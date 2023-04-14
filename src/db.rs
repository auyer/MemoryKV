use crate::errors::KVError;
use bytes::Bytes;
use parking_lot::RwLock;
use std::{collections::HashMap, sync::Arc};

pub type KVStoreConn = Arc<KVStore>;

#[derive(Default)]
pub struct KVStore {
    db: RwLock<HashMap<String, Bytes>>,
}

impl KVStore {
    pub fn new() -> Arc<KVStore> {
        return Arc::new(KVStore::default());
    }
    pub fn get(&self, key: &str) -> Option<Bytes> {
        self.db.read().get(key).cloned()
    }

    pub fn insert(&self, key: &str, value: Bytes) -> Option<Bytes> {
        self.db.write().insert(key.to_string(), value)
    }

    pub fn remove(&self, key: &str) -> Option<Bytes> {
        self.db.write().remove(key)
    }

    pub fn remove_with_prefix(&self, key: &str) -> Vec<String> {
        let mut removed_keys = Vec::new();
        self.db.write().retain(|k, _| {
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
        self.db.write().clear();
    }

    pub fn list_keys(&self) -> Vec<String> {
        self.db
            .read()
            .keys()
            .map(|key| key.to_string())
            .collect::<Vec<String>>()
    }

    pub fn list_keys_with_prefix(&self, prefix: &str) -> Vec<String> {
        self.db
            .read()
            .keys()
            .filter(|key| key.starts_with(prefix))
            .map(|key| key.to_string())
            .collect::<Vec<String>>()
    }
}
