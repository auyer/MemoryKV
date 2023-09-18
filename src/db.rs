use bytes::Bytes;
use parking_lot::RwLock;

use std::{collections::HashMap, sync::Arc};

#[derive(Clone)]
pub struct KVStore {
    db: Arc<RwLock<db>>,
}

#[derive(Clone, Default)]
struct db {
    // Hash map chosen for the in memory data storage
    // it is easy to work with, and has O(1) read and write on average
    data: HashMap<String, Bytes>,
}

impl KVStore {
    pub fn new() -> KVStore {
        KVStore::default()
    }

    pub fn default() -> KVStore {
        KVStore {
            db: Arc::new(RwLock::new(db {
                ..Default::default()
            })),
        }
    }

    pub fn get(&self, key: &str) -> Option<Bytes> {
        self.db.read().data.get(key).cloned()
    }

    pub fn insert(&self, key: &str, value: Bytes) -> Option<Bytes> {
        self.db.write().data.insert(key.to_string(), value)
    }

    pub fn remove(&self, key: &str) -> Option<Bytes> {
        self.db.write().data.remove(key)
    }

    pub fn remove_with_prefix(&self, key: &str) -> Vec<String> {
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
        self.db.write().data.clear();
    }

    pub fn list_keys(&self) -> Vec<String> {
        self.db
            .read()
            .data
            .keys()
            .map(|key| key.to_string())
            .collect::<Vec<String>>()
    }

    pub fn list_keys_with_prefix(&self, prefix: &str) -> Vec<String> {
        self.db
            .read()
            .data
            .keys()
            .filter(|key| key.starts_with(prefix))
            .map(|key| key.to_string())
            .collect::<Vec<String>>()
    }
}
