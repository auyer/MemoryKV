use bytes::Bytes;
use std::net::SocketAddr;

#[derive(Clone, Debug)]
pub enum States {
    Insert { key: String, value: Bytes },
    Delete { key: String },
    DeletePrefix { key: String },
    DeleteAll,
    Read { key: String },
    List,
    ListPrefix { prefix: String },
    Heartbeat { addr: SocketAddr },
}

impl std::fmt::Display for States {
    fn fmt(&self, f: &mut std::fmt::Formatter) -> std::fmt::Result {
        match self {
            States::Insert { key, value } => write!(
                f,
                "Insert {{ key: {}, value: {} }}",
                key,
                String::from_utf8_lossy(value)
            ),
            States::Delete { key } => write!(f, "Delete {{ key: {} }}", key),
            States::DeletePrefix { key } => write!(f, "DeletePrefix {{ key: {} }}", key),
            States::DeleteAll => write!(f, "DeleteAll"),
            States::Read { key } => write!(f, "Read {{ key: {}}}", key),
            States::List => write!(f, "List"),
            States::ListPrefix { prefix } => write!(f, "ListPrefix {{ prefix: {}}}", prefix),
            States::Heartbeat { addr } => write!(f, "Heartbeat from {}", addr),
        }
    }
}
impl States {
    pub fn to_bytes(&self) -> Bytes {
        match self {
            States::Insert { key, value } => Bytes::from(format!(
                "Insert {{ key: {}, value: {} }}",
                key,
                String::from_utf8_lossy(value)
            )),
            States::Delete { key } => Bytes::from(format!("Delete {{ key: {} }}", key)),
            States::DeletePrefix { key } => Bytes::from(format!("DeletePrefix {{ key: {} }}", key)),
            States::DeleteAll => Bytes::from("DeleteAll".to_string()),
            States::Read { key } => Bytes::from(format!("Read {{ key: {}}}", key)),
            States::List => Bytes::from("List"),
            States::ListPrefix { prefix } => {
                Bytes::from(format!("ListPrefix {{ prefix: {}}}", prefix))
            }
            States::Heartbeat { addr } => Bytes::from(format!("Heartbeat from {}", addr)),
        }
    }
}
