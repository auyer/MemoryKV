use axum::{
    body::Bytes,
    extract::{Path, State},
    http::StatusCode,
    response::IntoResponse,
};
use std::{borrow::Cow, sync::Arc};
use tower::BoxError;

use crate::{db::KVStore, errors::KVError};

pub async fn ping() -> Bytes {
    Bytes::from("pong")
}

pub async fn kv_get(
    Path(key): Path<String>,
    State(kvstore): State<Arc<KVStore>>,
) -> Result<Bytes, StatusCode> {
    match kvstore.get(&key) {
        Some(value) => Ok(value),
        None => Ok(bytes::Bytes::new()),
    }
}

pub async fn kv_set(
    Path(key): Path<String>,
    State(kvstore): State<Arc<KVStore>>,
    bytes: Bytes,
) -> Result<Bytes, StatusCode> {
    match kvstore.insert(&key, bytes) {
        Some(value) => Ok(value),
        None => Ok(bytes::Bytes::new()),
    }
}

pub async fn remove_key(
    Path(key): Path<String>,
    State(kvstore): State<Arc<KVStore>>,
) -> Result<Bytes, StatusCode> {
    match kvstore.remove(&key) {
        Some(value) => Ok(value),
        None => Err(StatusCode::NOT_FOUND),
    }
}

pub async fn remove_prefix(
    Path(key): Path<String>,
    State(kvstore): State<Arc<KVStore>>,
) -> axum::Json<Vec<String>> {
    kvstore
        .remove_with_prefix(&key)
        .into_iter()
        .collect::<Vec<String>>()
        .into()
}

pub async fn remove_all_keys(State(kvstore): State<Arc<KVStore>>) -> Result<(), StatusCode> {
    Ok(kvstore.remove_all())
}

pub async fn list_keys(State(kvstore): State<Arc<KVStore>>) -> axum::Json<Vec<String>> {
    kvstore
        .list_keys()
        .into_iter()
        .collect::<Vec<String>>()
        .into()
}

pub async fn list_keys_with_prefix(
    Path(prefix): Path<String>,
    State(kvstore): State<Arc<KVStore>>,
) -> axum::Json<Vec<String>> {
    kvstore
        .list_keys_with_prefix(&prefix)
        .into_iter()
        .collect::<Vec<String>>()
        .into()
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
