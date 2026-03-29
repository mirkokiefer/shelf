import { renderRawFramebuffer } from "./raw-endpoint";
import { Hono } from "hono";
import { serve } from "@hono/node-server";
import { renderEinkImage } from "./render";
import { getData, setData, SceneData } from "./data";

const app = new Hono();

// ─── Health ───
app.get("/health", (c) => c.json({ status: "ok", uptime: process.uptime() }));

// ─── Scene data API ───
app.get("/api/data", (c) => c.json(getData()));

app.post("/api/data", async (c) => {
  const body = await c.req.json();
  setData(body as Partial<SceneData>);
  return c.json({ ok: true });
});

// ─── E-ink image endpoint (PNG, 480×800, grayscale) ───
app.get("/eink.png", async (c) => {
  const data = getData();
  const png = await renderEinkImage(data);
  return new Response(png, {
    headers: {
      "Content-Type": "image/png",
      "Cache-Control": "no-cache",
    },
  });
});

// ─── E-ink image as BMP (for stock firmware compat) ───
app.get("/eink.bmp", async (c) => {
  const data = getData();
  const png = await renderEinkImage(data);
  // Convert PNG to BMP using sharp
  const sharp = (await import("sharp")).default;
  const bmp = await sharp(png).toFormat("png").toBuffer(); // X4 can read PNG via custom firmware
  return new Response(bmp, {
    headers: {
      "Content-Type": "image/png",
      "Cache-Control": "no-cache",
    },
  });
});


// ─── E-ink raw 1-bit framebuffer (for ESP32-C3 direct display) ───
app.get("/eink.raw", async (c) => {
  const data = getData();
  const raw = await renderRawFramebuffer(data);
  return new Response(raw, {
    headers: {
      "Content-Type": "application/octet-stream",
      "Content-Length": String(raw.length),
      "Cache-Control": "no-cache",
    },
  });
});

// ─── Web dashboard (preview of e-ink + controls) ───
app.get("/", (c) => {
  return c.html(`<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Shelf — Daslab E-Ink Dashboard</title>
  <style>
    * { margin: 0; padding: 0; box-sizing: border-box; }
    body { 
      font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', sans-serif;
      background: #0a0a0a; color: #e0e0e0; 
      display: flex; flex-direction: column; align-items: center;
      min-height: 100vh; padding: 40px 20px;
    }
    h1 { font-size: 24px; margin-bottom: 8px; color: #fff; }
    .subtitle { font-size: 14px; color: #888; margin-bottom: 32px; }
    .eink-frame {
      background: #1a1a1a; border-radius: 16px; padding: 16px;
      box-shadow: 0 8px 32px rgba(0,0,0,0.5);
      display: flex; flex-direction: column; align-items: center;
    }
    .eink-label { font-size: 11px; color: #555; margin-bottom: 8px; text-transform: uppercase; letter-spacing: 2px; }
    .eink-preview {
      border: 2px solid #333; border-radius: 4px;
      width: 288px; height: 480px; /* 0.6x scale of 480x800 */
      background: #f5f0e8;
    }
    .eink-preview img { width: 100%; height: 100%; object-fit: contain; }
    .controls { margin-top: 24px; display: flex; gap: 12px; flex-wrap: wrap; justify-content: center; }
    button {
      background: #222; border: 1px solid #333; color: #ccc; padding: 8px 16px;
      border-radius: 8px; cursor: pointer; font-size: 13px;
      transition: all 0.2s;
    }
    button:hover { background: #333; color: #fff; }
    .status { margin-top: 16px; font-size: 12px; color: #555; }
    .data-panel {
      margin-top: 24px; background: #111; border-radius: 12px; padding: 16px;
      width: 100%; max-width: 400px;
    }
    .data-panel h3 { font-size: 14px; color: #888; margin-bottom: 12px; }
    .data-row { display: flex; justify-content: space-between; padding: 6px 0; border-bottom: 1px solid #1a1a1a; }
    .data-key { color: #666; font-size: 13px; }
    .data-val { color: #ccc; font-size: 13px; font-family: monospace; }
  </style>
</head>
<body>
  <h1>📋 Shelf</h1>
  <p class="subtitle">Physical Daslab scene widget — live on Xteink X4</p>
  
  <div class="eink-frame">
    <div class="eink-label">Xteink X4 • 480×800 • E-Ink</div>
    <div class="eink-preview">
      <img id="preview" src="/eink.png" alt="E-Ink Preview" />
    </div>
  </div>

  <div class="controls">
    <button onclick="refresh()">⟳ Refresh Preview</button>
    <button onclick="setQuote()">💬 New Quote</button>
    <button onclick="toggleMode()">🔄 Toggle Mode</button>
  </div>
  <div class="status" id="status">Last refresh: just now</div>

  <div class="data-panel">
    <h3>Scene Data</h3>
    <div id="data-rows"></div>
  </div>

  <script>
    async function refresh() {
      document.getElementById('preview').src = '/eink.png?' + Date.now();
      document.getElementById('status').textContent = 'Last refresh: ' + new Date().toLocaleTimeString();
      const res = await fetch('/api/data');
      const data = await res.json();
      const rows = document.getElementById('data-rows');
      rows.innerHTML = Object.entries(data).map(([k, v]) => 
        '<div class="data-row"><span class="data-key">' + k + '</span><span class="data-val">' + 
        (typeof v === 'object' ? JSON.stringify(v) : v) + '</span></div>'
      ).join('');
    }
    
    async function setQuote() {
      const quotes = [
        "The best way to predict the future is to invent it. — Alan Kay",
        "Hardware is the new software. — Mirko Kiefer",
        "Any sufficiently advanced technology is indistinguishable from magic. — Arthur C. Clarke",
        "The only way to do great work is to love what you do. — Steve Jobs",
        "Move fast and build things. — Daslab",
        "Intelligence at the edge is the next frontier.",
        "Scenes are where work lives. — Daslab",
      ];
      const q = quotes[Math.floor(Math.random() * quotes.length)];
      await fetch('/api/data', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ quote: q })
      });
      setTimeout(refresh, 500);
    }

    async function toggleMode() {
      const res = await fetch('/api/data');
      const data = await res.json();
      const modes = ['dashboard', 'quote', 'status'];
      const idx = modes.indexOf(data.mode || 'dashboard');
      const next = modes[(idx + 1) % modes.length];
      await fetch('/api/data', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ mode: next })
      });
      setTimeout(refresh, 500);
    }

    refresh();
    setInterval(refresh, 30000);
  </script>
</body>
</html>`);
});

export default app;

const port = 3001;
console.log(`🗂️  Shelf server running on http://localhost:${port}`);
console.log(`📱 E-ink image: http://localhost:${port}/eink.png`);
console.log(`🌐 Dashboard:   http://localhost:${port}/`);

export { port };
