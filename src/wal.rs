use bytes::Bytes;
use std::net::SocketAddr;

#[derive(Clone, Debug)]
pub enum KVWAL {
    Insert { key: String, value: Bytes },
    Delete { key: String },
    DeletePrefix { key: String },
    DeleteAll,
    Read { key: String },
    List,
    ListPrefix { prefix: String },
    Heartbeat { addr: SocketAddr },
}

impl std::fmt::Display for KVWAL {
    fn fmt(&self, f: &mut std::fmt::Formatter) -> std::fmt::Result {
        match self {
            KVWAL::Insert { key, value } => write!(
                f,
                "Insert {{ key: {}, value: {} }}",
                key,
                String::from_utf8_lossy(&value)
            ),
            KVWAL::Delete { key } => write!(f, "Delete {{ key: {} }}", key),
            KVWAL::DeletePrefix { key } => write!(f, "DeletePrefix {{ key: {} }}", key),
            KVWAL::DeleteAll => write!(f, "DeleteAll"),
            KVWAL::Read { key } => write!(f, "Read {{ key: {}}}", key),
            KVWAL::List => write!(f, "List"),
            KVWAL::ListPrefix { prefix } => write!(f, "ListPrefix {{ prefix: {}}}", prefix),
            KVWAL::Heartbeat { addr } => write!(f, "Heartbeat from {}", addr.to_string()),
        }
    }
}
impl KVWAL {
    pub fn to_bytes(&self) -> Bytes {
        match self {
            KVWAL::Insert { key, value } => Bytes::from(format!(
                "Insert {{ key: {}, value: {} }}",
                key,
                String::from_utf8_lossy(&value)
            )),
            KVWAL::Delete { key } => Bytes::from(format!("Delete {{ key: {} }}", key)),
            KVWAL::DeletePrefix { key } => Bytes::from(format!("DeletePrefix {{ key: {} }}", key)),
            KVWAL::DeleteAll => Bytes::from(format!("DeleteAll")),
            KVWAL::Read { key } => Bytes::from(format!("Read {{ key: {}}}", key)),
            KVWAL::List => Bytes::from("List"),
            KVWAL::ListPrefix { prefix } => {
                Bytes::from(format!("ListPrefix {{ prefix: {}}}", prefix))
            }
            KVWAL::Heartbeat { addr } => {
                Bytes::from(format!("Heartbeat from {}", addr.to_string()))
            }
        }
    }
}
