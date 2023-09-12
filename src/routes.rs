use axum::extract::connect_info::IntoMakeServiceWithConnectInfo;
use axum::{
    error_handling::HandleErrorLayer,
    extract::{DefaultBodyLimit, FromRef},
    http::{header, Method},
    middleware,
    routing::get,
    Router,
};

use std::{net::SocketAddr, time::Duration};
use tower::ServiceBuilder;
use tower_http::cors::{Any, CorsLayer};
use tower_http::{
    compression::CompressionLayer,
    limit::RequestBodyLimitLayer,
    trace::TraceLayer,
    // validate_request::ValidateRequestHeaderLayer,
};

use crate::actions::ActionBroadcaster;
use crate::db::KVStore;
use crate::handlers;
use crate::metrics;

// the application state that encapsulates both KVStore and ActionBroadcaster
#[derive(Clone)]
struct AppState {
    kv_store: KVStore,
    actions: ActionBroadcaster,
}

// support converting an `AppState` in an `ApiState`
impl FromRef<AppState> for KVStore {
    fn from_ref(app_state: &AppState) -> KVStore {
        app_state.kv_store.clone()
    }
}

impl FromRef<AppState> for ActionBroadcaster {
    fn from_ref(app_state: &AppState) -> ActionBroadcaster {
        app_state.actions.clone()
    }
}

pub fn build_app() -> IntoMakeServiceWithConnectInfo<Router, std::net::SocketAddr> {
    let cors = CorsLayer::new()
        // allow `GET`, `PUT` and `DELETE` when accessing the resource
        .allow_methods([Method::GET, Method::PUT, Method::DELETE])
        .allow_headers(vec![header::ACCEPT, header::CONTENT_TYPE])
        // allow requests from any origin
        .allow_origin(Any);
    // move out
    let kv_store = KVStore::new();

    let actions_bc = ActionBroadcaster::new();

    let app_state = AppState {
        actions: actions_bc.clone(),
        kv_store: kv_store.clone(),
    };

    // Build our application by composing routes
    let rest = Router::new()
        .route(
            "/:key",
            get(handlers::kv_get)
                .put(handlers::kv_set)
                .delete(handlers::remove_key),
        )
        .route(
            "/keys",
            get(handlers::list_keys).delete(handlers::remove_all_keys),
        )
        .route(
            "/keys/:prefix",
            get(handlers::list_keys_with_prefix).delete(handlers::remove_prefix),
        );

    let stream = Router::new()
        .route("/wal", get(handlers::ws_handler))
        .route("/ping", get(handlers::ping));

    let app = Router::new()
        .layer((
            DefaultBodyLimit::disable(),
            RequestBodyLimitLayer::new(1024 * 5_000 /* ~5mb */),
        ))
        .layer(CompressionLayer::new())
        .layer(cors)
        .layer(
            ServiceBuilder::new()
                // Handle errors from middleware
                .layer(HandleErrorLayer::new(handlers::handle_error))
                .load_shed()
                .concurrency_limit(1024)
                .timeout(Duration::from_secs(10))
                .layer(TraceLayer::new_for_http()),
        )
        .layer(metrics::create_tracing_layer())
        .layer(middleware::from_fn(metrics::track_metrics))
        .merge(rest.with_state(app_state.clone()))
        .merge(stream.with_state(actions_bc.clone()));

    app.into_make_service_with_connect_info::<SocketAddr>()
}
