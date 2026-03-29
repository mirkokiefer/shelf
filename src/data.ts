export interface SceneData {
  mode: "dashboard" | "quote" | "status";
  sceneName: string;
  quote: string;
  // Dashboard widgets
  time: string;
  date: string;
  weather: { temp: string; condition: string; icon: string };
  github: { repo: string; stars: number; lastCommit: string };
  deploy: { service: string; status: string; lastDeploy: string };
  devices: { name: string; status: string; lastSeen: string }[];
  // Camera (Phase 2)
  lastCaption: string;
  faceCount: number;
}

const defaults: SceneData = {
  mode: "dashboard",
  sceneName: "Shelf",
  quote: "Intelligence at the edge is the next frontier. — Daslab",
  time: "",
  date: "",
  weather: { temp: "18°C", condition: "Partly Cloudy", icon: "⛅" },
  github: { repo: "mirkokiefer/daslab", stars: 42, lastCommit: "3h ago" },
  deploy: { service: "Daslab Server", status: "live", lastDeploy: "2h ago" },
  devices: [
    { name: "XIAO ESP32-S3", status: "online", lastSeen: "now" },
    { name: "Xteink X4", status: "online", lastSeen: "now" },
    { name: "Pi Zero 2W", status: "standby", lastSeen: "5m ago" },
  ],
  lastCaption: "",
  faceCount: 0,
};

let state: SceneData = { ...defaults };

export function getData(): SceneData {
  // Always update time
  const now = new Date();
  state.time = now.toLocaleTimeString("en-US", {
    hour: "2-digit",
    minute: "2-digit",
    hour12: false,
    timeZone: "Europe/Berlin",
  });
  state.date = now.toLocaleDateString("en-US", {
    weekday: "long",
    month: "short",
    day: "numeric",
    timeZone: "Europe/Berlin",
  });
  return state;
}

export function setData(partial: Partial<SceneData>) {
  state = { ...state, ...partial };
}
