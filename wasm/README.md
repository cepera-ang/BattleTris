# BattleTris wasm fork

This fork runs the original BattleTris code in browser with a wasm shim.

## Build

1. Install emsdk.
2. From `wasm/`:

```bash
source /path/to/emsdk/emsdk_env.sh
make all
```

Output is in `wasm/dist/`.

## Run (single player)

```bash
python -m http.server 8088 --directory wasm/dist
```

Open:

`http://127.0.0.1:8088/`

## Run (multiplayer relay)

Start relay:

```bash
cd wasm/relay
npm install
npm start
```

Then open on both sides:

`http://127.0.0.1:8088/?mp=1&room=test&ws=ws://127.0.0.1:8099`

Use same `room` value on both sides.
