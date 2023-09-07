use crate::errors::KVError;
use crate::wal::KVWAL;
use bytes::Bytes;
use parking_lot::RwLock;
use std::net::SocketAddr;
use std::{collections::HashMap, sync::Arc};
use tokio::sync::broadcast;

#[derive(Clone)]
pub struct KVStore {
    db: Arc<RwLock<db>>,
    wal_tx: broadcast::Sender<KVWAL>,
}

#[derive(Clone, Default)]
struct db {
    // Hash map chosed fort the in memory data storage
    // it is easy to work with, and has O(1) read and write on average
    data: HashMap<String, Bytes>,
}

impl KVStore {
    pub fn new() -> KVStore {
        return KVStore::default();
    }

    pub fn default() -> KVStore {
        let (wal_tx, _) = broadcast::channel::<KVWAL>(100);

        KVStore {
            db: Arc::new(RwLock::new(db {
                ..Default::default()
            })),
            wal_tx,
        }
    }

    pub fn subscribe(&self) -> broadcast::Receiver<KVWAL> {
        self.wal_tx.subscribe()
    }

    pub fn send_heartbeat(&self, addr: SocketAddr) -> Result<(), KVError> {
        self.wal_tx.send(KVWAL::Heartbeat { addr })?;

        Ok(())
        //todo add return
    }

    pub fn get(&self, key: &str) -> Option<Bytes> {
        self.wal_tx.send(KVWAL::Read {
            key: key.to_string(),
        });
        self.db.read().data.get(key).cloned()
    }

    pub fn insert(
        &self,
        key: &str,
        value: Bytes,
    ) -> Option<Bytes> {
        self.wal_tx.send(KVWAL::Insert {
            key: key.to_string(),
            value: value.clone(),
        });
        self.db.write().data.insert(key.to_string(), value)
    }

    pub fn remove(&self, key: &str) -> Option<Bytes> {
        self.wal_tx.send(KVWAL::Delete {
            key: key.to_string(),
        });
        self.db.write().data.remove(key)
    }

    pub fn remove_with_prefix(&self, key: &str) -> Vec<String> {
        self.wal_tx.send(KVWAL::DeletePrefix {
            key: key.to_string(),
        });
        let mut removed_keys = Vec::new();
        self.db.write().data.retain(|k, _| {
            if k.starts_with(key) {
                removed_keys.push(k.to_string());
                false
            } else {
                true
            }
        });
        removed_keys
    }

    pub fn remove_all(&self) {
        self.wal_tx.send(KVWAL::DeleteAll);
        self.db.write().data.clear();
    }

    pub fn list_keys(&self) -> Vec<String> {
        self.wal_tx.send(KVWAL::List);
        self.db
            .read()
            .data
            .keys()
            .map(|key| key.to_string())
            .collect::<Vec<String>>()
    }

    pub fn list_keys_with_prefix(&self, prefix: &str) -> Vec<String> {
        self.wal_tx.send(KVWAL::ListPrefix {
            prefix: prefix.to_string(),
        });
        self.db
            .read()
            .data
            .keys()
            .filter(|key| key.starts_with(prefix))
            .map(|key| key.to_string())
            .collect::<Vec<String>>()
    }
}
