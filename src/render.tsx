import satori from "satori";
import { Resvg } from "@resvg/resvg-js";
import type { SceneData } from "./data";

const WIDTH = 480;
const HEIGHT = 800;

let fontData: ArrayBuffer | null = null;
async function getFont(): Promise<ArrayBuffer> {
  if (fontData) return fontData;
  const res = await fetch(
    "https://cdn.jsdelivr.net/fontsource/fonts/inter@latest/latin-400-normal.woff"
  );
  fontData = await res.arrayBuffer();
  return fontData;
}

let fontBoldData: ArrayBuffer | null = null;
async function getBoldFont(): Promise<ArrayBuffer> {
  if (fontBoldData) return fontBoldData;
  const res = await fetch(
    "https://cdn.jsdelivr.net/fontsource/fonts/inter@latest/latin-700-normal.woff"
  );
  fontBoldData = await res.arrayBuffer();
  return fontBoldData;
}

function DeviceRow({ name, status, lastSeen }: { name: string; status: string; lastSeen: string }) {
  return (
    <div style={{ display: "flex", justifyContent: "space-between", padding: "6px 0" }}>
      <div style={{ display: "flex", alignItems: "center", fontSize: "15px" }}>
        <div
          style={{
            display: "flex",
            width: "8px",
            height: "8px",
            borderRadius: "50%",
            backgroundColor: status === "online" ? "#22c55e" : status === "standby" ? "#eab308" : "#ef4444",
            marginRight: "8px",
          }}
        />
        <span>{name}</span>
      </div>
      <div style={{ display: "flex", fontSize: "13px", color: "#888" }}>{lastSeen}</div>
    </div>
  );
}

function Card({ label, children }: { label: string; children: any }) {
  return (
    <div
      style={{
        display: "flex",
        flexDirection: "column",
        backgroundColor: "#ebe6dc",
        borderRadius: "12px",
        padding: "16px",
        marginBottom: "12px",
      }}
    >
      <div style={{ display: "flex", fontSize: "12px", color: "#999", marginBottom: "8px" }}>
        {label}
      </div>
      {children}
    </div>
  );
}

function DashboardLayout(data: SceneData) {
  return (
    <div
      style={{
        display: "flex",
        flexDirection: "column",
        width: "100%",
        height: "100%",
        backgroundColor: "#f5f0e8",
        color: "#1a1a1a",
        padding: "24px",
        fontFamily: "Inter",
      }}
    >
      {/* Header */}
      <div style={{ display: "flex", justifyContent: "space-between", alignItems: "flex-start", marginBottom: "20px" }}>
        <div style={{ display: "flex", flexDirection: "column" }}>
          <div style={{ display: "flex", fontSize: "56px", fontWeight: 700, lineHeight: "1" }}>{data.time}</div>
          <div style={{ display: "flex", fontSize: "18px", color: "#666", marginTop: "4px" }}>{data.date}</div>
        </div>
        <div style={{ display: "flex", flexDirection: "column", alignItems: "flex-end" }}>
          <div style={{ display: "flex", fontSize: "32px" }}>{data.weather.icon}</div>
          <div style={{ display: "flex", fontSize: "20px", fontWeight: 700 }}>{data.weather.temp}</div>
          <div style={{ display: "flex", fontSize: "13px", color: "#888" }}>{data.weather.condition}</div>
        </div>
      </div>

      {/* Divider */}
      <div style={{ display: "flex", width: "100%", height: "2px", backgroundColor: "#d0c8b8", marginBottom: "20px" }} />

      {/* Scene Label */}
      <div style={{ display: "flex", fontSize: "14px", color: "#999", letterSpacing: "2px", marginBottom: "8px" }}>
        DASLAB SCENE
      </div>
      <div style={{ display: "flex", fontSize: "28px", fontWeight: 700, marginBottom: "24px" }}>
        {data.sceneName}
      </div>

      {/* GitHub Card */}
      <Card label="GITHUB">
        <div style={{ display: "flex", justifyContent: "space-between", alignItems: "center" }}>
          <div style={{ display: "flex", fontSize: "18px", fontWeight: 700 }}>{data.github.repo}</div>
          <div style={{ display: "flex", fontSize: "22px", fontWeight: 700 }}>{data.github.stars}</div>
        </div>
        <div style={{ display: "flex", fontSize: "13px", color: "#888", marginTop: "4px" }}>
          Last commit: {data.github.lastCommit}
        </div>
      </Card>

      {/* Deploy Card */}
      <Card label="DEPLOY">
        <div style={{ display: "flex", justifyContent: "space-between", alignItems: "center" }}>
          <div style={{ display: "flex", fontSize: "18px", fontWeight: 700 }}>{data.deploy.service}</div>
          <div
            style={{
              display: "flex",
              backgroundColor: data.deploy.status === "live" ? "#22c55e" : "#ef4444",
              color: "white",
              padding: "4px 12px",
              borderRadius: "20px",
              fontSize: "13px",
              fontWeight: 700,
            }}
          >
            {data.deploy.status === "live" ? "LIVE" : "DOWN"}
          </div>
        </div>
        <div style={{ display: "flex", fontSize: "13px", color: "#888", marginTop: "4px" }}>
          Deployed: {data.deploy.lastDeploy}
        </div>
      </Card>

      {/* Devices Card */}
      <Card label="DEVICES">
        <div style={{ display: "flex", flexDirection: "column" }}>
          {data.devices.map((d, i) => (
            <DeviceRow key={i} name={d.name} status={d.status} lastSeen={d.lastSeen} />
          ))}
        </div>
      </Card>

      {/* Quote Footer */}
      <div
        style={{
          display: "flex",
          marginTop: "auto",
          paddingTop: "16px",
          borderTop: "1px solid #d0c8b8",
          fontSize: "13px",
          color: "#888",
          fontStyle: "italic",
          lineHeight: "1.4",
        }}
      >
        {data.quote}
      </div>
    </div>
  );
}

function QuoteLayout(data: SceneData) {
  return (
    <div
      style={{
        display: "flex",
        flexDirection: "column",
        width: "100%",
        height: "100%",
        backgroundColor: "#f5f0e8",
        color: "#1a1a1a",
        padding: "40px 32px",
        fontFamily: "Inter",
        justifyContent: "center",
        alignItems: "center",
      }}
    >
      <div style={{ display: "flex", fontSize: "28px", fontWeight: 700, lineHeight: "1.5", textAlign: "center" }}>
        {data.quote}
      </div>
      <div style={{ display: "flex", marginTop: "40px", fontSize: "14px", color: "#999" }}>
        {data.time} — {data.date}
      </div>
    </div>
  );
}

function StatusLayout(data: SceneData) {
  return (
    <div
      style={{
        display: "flex",
        flexDirection: "column",
        width: "100%",
        height: "100%",
        backgroundColor: "#f5f0e8",
        color: "#1a1a1a",
        padding: "32px 24px",
        fontFamily: "Inter",
      }}
    >
      <div style={{ display: "flex", fontSize: "14px", color: "#999", letterSpacing: "2px", marginBottom: "16px" }}>
        SYSTEM STATUS
      </div>
      <div style={{ display: "flex", fontSize: "48px", fontWeight: 700, marginBottom: "32px" }}>
        {data.time}
      </div>

      <div style={{ display: "flex", flexDirection: "column" }}>
        {data.devices.map((d, i) => (
          <div
            key={i}
            style={{
              display: "flex",
              backgroundColor: "#ebe6dc",
              borderRadius: "12px",
              padding: "20px",
              marginBottom: "12px",
              justifyContent: "space-between",
              alignItems: "center",
            }}
          >
            <div style={{ display: "flex", flexDirection: "column" }}>
              <div style={{ display: "flex", fontSize: "20px", fontWeight: 700 }}>{d.name}</div>
              <div style={{ display: "flex", fontSize: "14px", color: "#888", marginTop: "4px" }}>
                Last seen: {d.lastSeen}
              </div>
            </div>
            <div
              style={{
                display: "flex",
                width: "16px",
                height: "16px",
                borderRadius: "50%",
                backgroundColor: d.status === "online" ? "#22c55e" : d.status === "standby" ? "#eab308" : "#ef4444",
              }}
            />
          </div>
        ))}
      </div>

      {data.lastCaption ? (
        <div
          style={{
            display: "flex",
            flexDirection: "column",
            backgroundColor: "#ebe6dc",
            borderRadius: "12px",
            padding: "20px",
            marginTop: "12px",
          }}
        >
          <div style={{ display: "flex", fontSize: "12px", color: "#999", marginBottom: "8px" }}>AI VISION</div>
          <div style={{ display: "flex", fontSize: "16px", lineHeight: "1.4" }}>{data.lastCaption}</div>
        </div>
      ) : null}

      <div style={{ display: "flex", marginTop: "auto", fontSize: "13px", color: "#aaa" }}>
        Daslab — Shelf — {data.sceneName}
      </div>
    </div>
  );
}

export async function renderEinkImage(data: SceneData): Promise<Buffer> {
  const [font, boldFont] = await Promise.all([getFont(), getBoldFont()]);

  let element: any;
  switch (data.mode) {
    case "quote":
      element = QuoteLayout(data);
      break;
    case "status":
      element = StatusLayout(data);
      break;
    default:
      element = DashboardLayout(data);
  }

  const svg = await satori(element, {
    width: WIDTH,
    height: HEIGHT,
    fonts: [
      { name: "Inter", data: font, weight: 400, style: "normal" },
      { name: "Inter", data: boldFont, weight: 700, style: "normal" },
    ],
  });

  const resvg = new Resvg(svg, {
    fitTo: { mode: "width", value: WIDTH },
  });

  const rendered = resvg.render();
  return Buffer.from(rendered.asPng());
}
