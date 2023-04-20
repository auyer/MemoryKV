# KV

I like to learn thing by doing them.
This is a in memory Key Value store in Rust. It also has a Write Ahead Log served with Websockets

A sample of the WAL received by a WebSocket client:
```prolog has the right colors
23:42 23.17 Heartbeat from 127.0.0.1:36460
23:42 14.02 Read { key: pre:fix:for_key_value:perfect}
23:42 03.70 List
23:42 03.17 Heartbeat from 127.0.0.1:53374
23:41 29.96 Read { key: pre:demo:key}
23:41 23.17 Heartbeat from 127.0.0.1:36460
23:41 17.34 ListPrefix { prefix: pre}
23:41 05.19 Insert { key: pre:demo:key, value: {"source": "CloudFlare Worker", "type": "WASM"} }
23:41 03.19 Heartbeat from 127.0.0.1:53374
23:41 03.18 Connected to ws://localhost:8080/wal
23:41 03.17 Connecting to ws://localhost:8080/wal
```