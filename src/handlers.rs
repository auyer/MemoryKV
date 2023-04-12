use axum::{
    body::Bytes,
    extract::{Path, State},
    http::StatusCode,
    response::IntoResponse,
};
use std::borrow::Cow;
use tower::BoxError;

use crate::db::KVStoreConn;

pub async fn kv_get(
    Path(key): Path<String>,
    State(kvstore): State<KVStoreConn>,
) -> Result<Bytes, StatusCode> {
    let db = &kvstore.read().unwrap();

    if let Some(value) = db.get(&key) {
        Ok(value.clone())
    } else {
        Err(StatusCode::NOT_FOUND)
    }
}

pub async fn kv_set(Path(key): Path<String>, State(kvstore): State<KVStoreConn>, bytes: Bytes) {
    kvstore.write().unwrap().insert(&key, bytes);
}

pub async fn list_keys(State(kvstore): State<KVStoreConn>) -> String {
    let db = &kvstore.read().unwrap();

    db.list_keys()
        .into_iter()
        .map(|key| key.to_string())
        .collect::<Vec<String>>()
        .join("\n")
}

pub async fn list_keys_with_prefix(
    Path(prefix): Path<String>,
    State(kvstore): State<KVStoreConn>,
) -> String {
    let db = &kvstore.read().unwrap();

    db.list_keys_with_prefix(&prefix)
        .into_iter()
        .map(|key| key.to_string())
        .collect::<Vec<String>>()
        .join("\n")
}

pub async fn delete_all_keys(State(kvstore): State<KVStoreConn>) {
    kvstore.write().unwrap().remove_all();
}

pub async fn remove_key(Path(key): Path<String>, State(kvstore): State<KVStoreConn>) {
    kvstore.write().unwrap().remove(&key);
}

pub async fn handle_error(error: BoxError) -> impl IntoResponse {
    if error.is::<tower::timeout::error::Elapsed>() {
        return (StatusCode::REQUEST_TIMEOUT, Cow::from("request timed out"));
    }

    if error.is::<tower::load_shed::error::Overloaded>() {
        return (
            StatusCode::SERVICE_UNAVAILABLE,
            Cow::from("service is overloaded, try again later"),
        );
    }

    (
        StatusCode::INTERNAL_SERVER_ERROR,
        Cow::from(format!("Unhandled internal error: {}", error)),
    )
}
