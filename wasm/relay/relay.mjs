import { WebSocketServer } from "ws";

const port = Number(process.env.PORT || 8099);
const wss = new WebSocketServer({ port });
const rooms = new Map();

function roomFromUrl(url) {
  const q = new URL(url || "/", "ws://localhost");
  return q.searchParams.get("room") || "battletris";
}

wss.on("connection", (ws, req) => {
  const room = roomFromUrl(req.url);
  if (!rooms.has(room)) rooms.set(room, new Set());
  const set = rooms.get(room);
  set.add(ws);

  ws.on("message", (data, isBinary) => {
    for (const peer of set) {
      if (peer === ws || peer.readyState !== peer.OPEN) continue;
      peer.send(data, { binary: isBinary });
    }
  });

  ws.on("close", () => {
    set.delete(ws);
    if (set.size === 0) rooms.delete(room);
  });
});

console.log(`battletris relay listening ws://127.0.0.1:${port}`);
