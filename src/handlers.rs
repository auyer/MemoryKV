use axum::{
    headers::ContentType,
    extract::{
        ws::{Message, WebSocket, WebSocketUpgrade},
        TypedHeader,
    },
    body::Bytes,
    extract::{Path, State},
    http::StatusCode,
    response::IntoResponse,
};
use std::{borrow::Cow, sync::Arc};
use tower::BoxError;
use std::net::SocketAddr;
use axum::extract::connect_info::ConnectInfo;
use futures::{sink::SinkExt, stream::StreamExt};
use tokio::time::{sleep, Duration};


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


pub async fn ws_handler(
    ws: WebSocketUpgrade,
    content_type: Option<TypedHeader<ContentType>>,
    user_agent: Option<TypedHeader<headers::UserAgent>>,
    ConnectInfo(addr): ConnectInfo<SocketAddr>,
    State(kvstore): State<Arc<KVStore>>,
) -> impl IntoResponse {
    let user_agent = if let Some(TypedHeader(user_agent)) = user_agent {
        user_agent.to_string()
    } else {
        String::from("Unknown browser")
    };
    tracing::info!("`{user_agent}` at {addr} connected.");

    if let Some(content_type) = content_type {
        // if content_type is binary
        if content_type.0 == ContentType::octet_stream() {
            tracing::info!("Client {} requested Binary protocol", addr);
            return ws.on_upgrade(move |socket| wal_handler(socket,addr,  kvstore, WalType::Binary))
        } 
    }
    tracing::info!("Client {} requested Textual protocol", addr);
    ws.on_upgrade(move |socket| wal_handler(socket,addr,  kvstore, WalType::Textual))
    
}

enum WalType{
    Textual,
    Binary
}

// returns the WAL via websocket
async fn wal_handler(stream: WebSocket, addr: SocketAddr, state: Arc<KVStore>, mode: WalType) {
    // By splitting, we can send and receive at the same time.
    
    let (mut sender, mut receiver) = stream.split();

    if sender.send(Message::Ping(vec![1, 2, 3])).await.is_err(){
        tracing::info!("Failed to send ping to {}, disconected",addr);
        return
    };
    // We subscribe *before* sending the "joined" message, so that we will also
    // display it to our client.
    let mut rx = state.subscribe();

    // Spawn the first task that will receive messages from the Clients
    let mut client_messages = tokio::spawn(async move {

        while let Some(Ok(msg)) = receiver.next().await {
            match msg {
                Message::Close(_) => {
                    
                    return},
                Message::Ping(_) => {
                    tracing::info!("Ping");
                    // let _ = tx.send(format!("{}: {}", name, text));
                }
                Message::Pong(_) => {
                    tracing::info!("Pong");
                }
                _ => {tracing::info!("ANY");}
            }
            // Add username before message.
            
        }
    });


    // task to recieve WAL messages and send to the WS client
    let mut wal_task = tokio::spawn(async move {
        match mode {
            WalType::Binary => {
                while let Ok(msg) = rx.recv().await {
                    // In any websocket error, break loop.
                    if sender.send(Message::Binary(msg.to_bytes().to_vec())).await.is_err() {
                        break;
                    }
                }
            },
            WalType::Textual => {
                while let Ok(msg) = rx.recv().await {
                    // In any websocket error, break loop.
                    if sender.send(Message::Text(msg.to_string())).await.is_err() {
                        break;
                    }
                }
            },
        }
        
    });

    // task to send heatbeat to the other listeners
    let mut heartbeat_task = tokio::spawn(async move {
      loop {
            if state.send_heartbeat(addr).is_err(){
                break;
            };
            sleep(Duration::from_secs(15)).await;
        }
    });

    // If any one of the tasks run to completion, we abort the other.
    tokio::select! {
        _ = (&mut wal_task) => wal_task.abort(),
        _ = (&mut client_messages) => client_messages.abort(),
        _ = (&mut heartbeat_task) => heartbeat_task.abort(),
    };
    // this handler is missing a propper disconect
    
    tracing::info!("Client {} disconected",addr);
}
