use crate::wal;
use thiserror::Error;
/// Errors that can happen when using this service

#[derive(Error, Debug)]
pub enum KVError {
    #[error("data store unavailable")]
    Unavailable { reason: String },
    #[error("data store unavailable")]
    WALChannelError {
        #[from]
        source: tokio::sync::broadcast::error::SendError<wal::KVWAL>,
    },
}
