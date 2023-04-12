use bytes::Bytes;
use std::{
    collections::HashMap,
    sync::{Arc, RwLock},
};

pub type KVStoreConn = Arc<RwLock<KVStore>>;

#[derive(Default)]
pub struct KVStore {
    db: HashMap<String, Bytes>,
}

impl KVStore {
    pub fn new() -> KVStoreConn {
        return KVStoreConn::default();
    }

    pub fn get(&self, key: &str) -> Option<Bytes> {
        self.db.get(key).cloned()
    }

    pub fn insert(&mut self, key: &str, value: Bytes) -> Option<Bytes> {
        self.db.insert(key.to_string(), value)
    }

    pub fn remove(&mut self, key: &str) -> Option<Bytes> {
        self.db.remove(key)
    }

    pub fn remove_all(&mut self) {
        self.db.clear()
    }

    pub fn list_keys(&self) -> Vec<&String> {
        self.db.keys().collect::<Vec<&String>>()
    }

    pub fn list_keys_with_prefix(&self, prefix: &str) -> Vec<&String> {
        self.db
            .keys()
            .filter(|key| key.starts_with(prefix))
            .collect::<Vec<&String>>()
    }
}
