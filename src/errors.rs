use thiserror::Error;

/// Errors that can happen when using this service

#[derive(Error, Debug)]
pub enum KVError {
    #[error("data store unavailable")]
    Unavailable { reason: String },
}
