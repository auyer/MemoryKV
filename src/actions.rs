use crate::errors::KVError;
use bytes::Bytes;
use std::net::SocketAddr;
use tokio::sync::broadcast;

#[derive(Clone)]
pub struct ActionBroadcaster {
    status_tx: broadcast::Sender<Actions>,
}

impl ActionBroadcaster {
    pub fn new() -> ActionBroadcaster {
        ActionBroadcaster::default()
    }

    pub fn default() -> ActionBroadcaster {
        let (status_tx, _) = broadcast::channel::<Actions>(10);

        ActionBroadcaster { status_tx }
    }

    pub fn subscribe(&self) -> broadcast::Receiver<Actions> {
        tracing::debug!("Subscribing to actions");
        self.status_tx.subscribe()
    }

    pub fn send_heartbeat(&self, addr: SocketAddr) -> Result<(), KVError> {
        tracing::debug!("Sending Heartbeat");
        self.status_tx.send(Actions::Heartbeat { addr })?;

        Ok(())
    }

    pub fn get(&self, key: &str) -> Result<(), KVError> {
        tracing::debug!("Sending a Get action");
        self.status_tx.send(Actions::Read {
            key: key.to_string(),
        })?;

        Ok(())
    }

    pub fn insert(&self, key: &str, value: Bytes) -> Result<(), KVError> {
        tracing::debug!("Sending a Insert action");
        self.status_tx.send(Actions::Insert {
            key: key.to_string(),
            value: value.clone(),
        })?;

        Ok(())
    }

    pub fn remove(&self, key: &str) -> Result<(), KVError> {
        tracing::debug!("Sending a Remove action");
        self.status_tx.send(Actions::Delete {
            key: key.to_string(),
        })?;
        Ok(())
    }

    pub fn remove_with_prefix(&self, key: &str) -> Result<(), KVError> {
        tracing::debug!("Sending a Remove with Prefix action");
        self.status_tx.send(Actions::DeletePrefix {
            key: key.to_string(),
        })?;
        Ok(())
    }

    pub fn remove_all(&self) -> Result<(), KVError> {
        tracing::debug!("Sending a Remove All action");
        self.status_tx.send(Actions::DeleteAll)?;
        Ok(())
    }

    pub fn list_keys(&self) -> Result<(), KVError> {
        tracing::debug!("Sending a List Keys action");
        self.status_tx.send(Actions::List)?;
        Ok(())
    }

    pub fn list_keys_with_prefix(&self, prefix: &str) -> Result<(), KVError> {
        tracing::debug!("Sending a List Keys With prefix action");
        self.status_tx.send(Actions::ListPrefix {
            prefix: prefix.to_string(),
        })?;
        Ok(())
    }
}

#[derive(Clone, Debug)]
pub enum Actions {
    Insert { key: String, value: Bytes },
    Delete { key: String },
    DeletePrefix { key: String },
    DeleteAll,
    Read { key: String },
    List,
    ListPrefix { prefix: String },
    Heartbeat { addr: SocketAddr },
}

impl std::fmt::Display for Actions {
    fn fmt(&self, f: &mut std::fmt::Formatter) -> std::fmt::Result {
        match self {
            Actions::Insert { key, value } => write!(
                f,
                "Insert {{ key: {}, value: {} }}",
                key,
                String::from_utf8_lossy(value)
            ),
            Actions::Delete { key } => write!(f, "Delete {{ key: {} }}", key),
            Actions::DeletePrefix { key } => write!(f, "DeletePrefix {{ key: {} }}", key),
            Actions::DeleteAll => write!(f, "DeleteAll"),
            Actions::Read { key } => write!(f, "Read {{ key: {}}}", key),
            Actions::List => write!(f, "List"),
            Actions::ListPrefix { prefix } => write!(f, "ListPrefix {{ prefix: {}}}", prefix),
            Actions::Heartbeat { addr } => write!(f, "Heartbeat from {}", addr),
        }
    }
}
impl Actions {
    pub fn to_bytes(&self) -> Bytes {
        match self {
            Actions::Insert { key, value } => Bytes::from(format!(
                "Insert {{ key: {}, value: {} }}",
                key,
                String::from_utf8_lossy(value)
            )),
            Actions::Delete { key } => Bytes::from(format!("Delete {{ key: {} }}", key)),
            Actions::DeletePrefix { key } => {
                Bytes::from(format!("DeletePrefix {{ key: {} }}", key))
            }
            Actions::DeleteAll => Bytes::from("DeleteAll".to_string()),
            Actions::Read { key } => Bytes::from(format!("Read {{ key: {}}}", key)),
            Actions::List => Bytes::from("List"),
            Actions::ListPrefix { prefix } => {
                Bytes::from(format!("ListPrefix {{ prefix: {}}}", prefix))
            }
            Actions::Heartbeat { addr } => Bytes::from(format!("Heartbeat from {}", addr)),
        }
    }
}
