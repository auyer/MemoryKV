use bytes::Bytes;

#[derive(Clone, Debug, PartialEq, Eq)]
pub enum Operations {
    Insert { key: String, value: Bytes },
    Delete { key: String },
    DeletePrefix { prefix: String },
    DeleteAll,
}

#[derive(Clone, Default)]
pub struct WAL {
    // WAL is a vector of operations
    // this is used to keep track of the changes that are made to the data store
    // A vector was used to use a continuous block of memory
    data: Vec<Operations>,

    // position stores the current position in the WAL
    position: usize,

    // maximum_size stores the maximum size of the WAL
    maximum_size: usize,
}

impl WAL {
    pub fn new() -> WAL {
        WAL::default()
    }

    pub fn new_with_size(size: usize) -> WAL {
        WAL {
            data: Vec::with_capacity(size),
            maximum_size: size,
            // position starts at maximum_size because it will roll over to 0 on the first insert
            position: size,
            ..Default::default()
        }
    }

    pub fn default() -> WAL {
        WAL {
            data: Vec::with_capacity(10_00),
            maximum_size: 10_00,
            position: 10_00,
            ..Default::default()
        }
    }

    fn advance_position(&mut self) {
        self.position += 1;
        // position can neve be greater than maximum_size
        // when it gets there, it should start overwriting the oldest data
        if self.position >= self.maximum_size {
            self.position = 0;
        }
    }

    fn save_operation(&mut self, operation: Operations) {
        // the vec has a capacity but no len at the start.
        // to set items in position, we need to first push to create the first elements
        // and only after start inserting into a position
        if self.data.len() < self.maximum_size {
            self.data.push(operation);
        } else {
            self.data[self.position] = operation;
        }
    }

    // insert returns the position of the inserted operation
    pub fn insert(&mut self, key: &str, value: Bytes) -> usize {
        self.advance_position();
        self.save_operation(Operations::Insert {
            key: key.to_string(),
            value,
        });

        self.position
    }

    pub fn delete(&mut self, key: &str) -> usize {
        self.advance_position();
        self.save_operation(Operations::Delete {
            key: key.to_string(),
        });

        self.position
    }

    pub fn delete_prefix(&mut self, prefix: &str) -> usize {
        self.advance_position();
        self.save_operation(Operations::DeletePrefix {
            prefix: prefix.to_string(),
        });

        self.position
    }

    pub fn delete_all(&mut self) -> usize {
        self.advance_position();
        self.save_operation(Operations::DeleteAll);

        self.position
    }
}

mod tests {
    use super::*;

    #[test]
    fn test_wal_insert() {
        let mut wal = WAL::new();
        let key = "key";
        let value = Bytes::from("value");
        let pos = wal.insert(key, value);
        assert_eq!(
            wal.data[pos],
            Operations::Insert {
                key: "key".to_string(),
                value: Bytes::from("value")
            }
        );
    }

    #[test]
    fn test_wal_insert_with_rollover() {
        let test_len = 1;
        let mut wal = WAL::new_with_size(test_len);
        for i in 0..test_len * 2 {
            let key = format!("key{}", i);
            let value = Bytes::from(format!("value{}", i));
            let pos = wal.insert(&key, value);
            assert!(pos < test_len, "position should be less than 'test_len'");
            assert_eq!(
                wal.data[pos],
                Operations::Insert {
                    key: key.to_string(),
                    value: Bytes::from(format!("value{}", i))
                }
            );
        }
        assert_eq!(wal.data.len(), test_len, "wal should have 'test_len' items");
    }

    #[test]
    fn test_wal_all_operations() {
        let mut wal = WAL::new();
        let key = "key";
        let value = Bytes::from("value");
        let pos = wal.insert(key, value);
        assert_eq!(
            wal.data[pos],
            Operations::Insert {
                key: "key".to_string(),
                value: Bytes::from("value")
            }
        );

        let pos = wal.delete(key);
        assert_eq!(
            wal.data[pos],
            Operations::Delete {
                key: "key".to_string()
            }
        );
        let pos = wal.delete_prefix(key);
        assert_eq!(
            wal.data[pos],
            Operations::DeletePrefix {
                prefix: "key".to_string()
            }
        );
        let pos = wal.delete_all();
        assert_eq!(wal.data[pos], Operations::DeleteAll);
    }
}
