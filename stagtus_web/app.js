'use strict';

const STORAGE_KEY = 'stagtus_config_v1';

// ── State ─────────────────────────────────────────────────────────
let commands      = null;
let port          = null;
let writer        = null;
let activeCommand = null;   // { key, cmd } of the last selected status

// ── Background glow crossfade ─────────────────────────────────────
const bgA      = document.getElementById('bg-a');
const bgB      = document.getElementById('bg-b');
let   activeBg = 'a'; // which overlay is currently visible

function hexToRgb(hex) {
  const h = hex.replace('#', '');
  return {
    r: parseInt(h.slice(0, 2), 16),
    g: parseInt(h.slice(2, 4), 16),
    b: parseInt(h.slice(4, 6), 16),
  };
}

function setBackgroundGlow(hex) {
  const { r, g, b } = hexToRgb(hex);
  const gradient = `radial-gradient(ellipse at 50% 40%,
    rgba(${r},${g},${b},0.22) 0%,
    rgba(${r},${g},${b},0.07) 45%,
    transparent 72%)`;

  const next = activeBg === 'a' ? bgB : bgA;
  const prev = activeBg === 'a' ? bgA : bgB;

  next.style.background = gradient;
  next.classList.add('visible');
  prev.classList.remove('visible');
  activeBg = activeBg === 'a' ? 'b' : 'a';
}

// ── DOM refs ──────────────────────────────────────────────────────
const configLoadedState    = document.getElementById('config-loaded-state');
const configEmptyState     = document.getElementById('config-empty-state');
const configSummary        = document.getElementById('config-summary');
const reloadConfigBtn      = document.getElementById('reload-config-btn');
const configFileInput      = document.getElementById('config-file-input');
const connDot              = document.getElementById('conn-dot');
const connLabel            = document.getElementById('conn-label');
const connectBtn           = document.getElementById('connect-btn');
const statusGrid           = document.getElementById('status-grid');
const currentStatusDisplay = document.getElementById('current-status-display');
const currentStatusText    = document.getElementById('current-status-text');
const noSerialWarning      = document.getElementById('no-serial-warning');

// ── TOML parser ───────────────────────────────────────────────────
// Handles the Stagtus config format: [sections] with key = "string" pairs.
function parseToml(text) {
  const result  = {};
  let   section = null;

  for (const raw of text.split('\n')) {
    const line = raw.trim();
    if (!line || line.startsWith('#')) continue;

    const sec = line.match(/^\[([^\]]+)\]$/);
    if (sec) {
      section = sec[1].trim();
      result[section] = {};
      continue;
    }

    const kv = line.match(/^(\w+)\s*=\s*"([^"]*)"$/);
    if (kv && section) result[section][kv[1]] = kv[2];
  }

  return result;
}

// ── Config ────────────────────────────────────────────────────────
function loadFromStorage() {
  try {
    const raw = localStorage.getItem(STORAGE_KEY);
    if (!raw) return false;
    applyConfig(parseToml(raw), null); // null = already stored, no need to re-save
    return true;
  } catch {
    localStorage.removeItem(STORAGE_KEY);
    return false;
  }
}

function applyConfig(parsed, rawToml) {
  commands = {};
  for (const [key, value] of Object.entries(parsed)) {
    if (key !== 'general' && typeof value === 'object') {
      commands[key] = value;
    }
  }

  if (rawToml !== null) localStorage.setItem(STORAGE_KEY, rawToml);

  showConfigLoaded();
  renderStatusGrid();
}

function showConfigLoaded() {
  const keys  = Object.keys(commands);
  const names = keys.map(k => commands[k].text || k).join(', ');
  configSummary.textContent = `${keys.length} status${keys.length !== 1 ? 'es' : ''}: ${names}`;

  configLoadedState.classList.remove('hidden');
  configEmptyState.classList.add('hidden');

  if (navigator.serial) connectBtn.disabled = false;
}

function showConfigEmpty() {
  configLoadedState.classList.add('hidden');
  configEmptyState.classList.remove('hidden');
  connectBtn.disabled = true;
}

// ── Status grid ───────────────────────────────────────────────────
function renderStatusGrid() {
  if (!commands || !Object.keys(commands).length) {
    statusGrid.innerHTML = '<p class="muted hint-text">Load a configuration file to see available statuses.</p>';
    return;
  }

  statusGrid.innerHTML = '';

  const sorted = Object.entries(commands).sort(([a], [b]) => a.localeCompare(b));

  for (const [key, cmd] of sorted) {
    const btn = document.createElement('button');
    btn.className   = 'status-btn';
    btn.dataset.key = key;
    btn.title       = cmd.text || key;

    if (cmd.color) btn.style.setProperty('--btn-color', `#${cmd.color}`);

    // emoji field holds an emoji character; fall back to first letter of the key
    const icon = cmd.emoji || key.charAt(0).toUpperCase();

    btn.innerHTML = `
      <div class="status-btn-inner">
        <span class="status-emoji" aria-hidden="true">${icon}</span>
        <span class="status-btn-label">${cmd.text || key}</span>
      </div>`;

    btn.addEventListener('click', () => onStatusClick(key, cmd, btn));
    statusGrid.appendChild(btn);
  }
}

function onStatusClick(key, cmd, btn) {
  document.querySelectorAll('.status-btn').forEach(b => b.classList.remove('active'));
  btn.classList.add('active');

  currentStatusText.textContent = cmd.text || key;
  currentStatusDisplay.classList.remove('hidden');

  if (cmd.color) setBackgroundGlow(cmd.color);

  activeCommand = { key, cmd };
  sendSerialCommand(cmd.id);
}

// ── Serial ────────────────────────────────────────────────────────
async function toggleConnection() {
  port ? await disconnect() : await connect();
}

async function connect() {
  try {
    port   = await navigator.serial.requestPort();
    await port.open({ baudRate: 115200 });
    writer = port.writable.getWriter();
    setConnected(true);
    if (activeCommand) sendSerialCommand(activeCommand.cmd.id);
  } catch (err) {
    port   = null;
    writer = null;
    if (err.name !== 'NotFoundError') setConnected(false, `Error: ${err.message}`);
  }
}

async function disconnect() {
  try {
    if (writer) { writer.releaseLock(); writer = null; }
    if (port)   { await port.close();   port   = null; }
  } catch { /* ignore close errors */ }
  setConnected(false);
}

function setConnected(connected, errorMsg) {
  if (connected) {
    connDot.className      = 'dot dot-green';
    connLabel.textContent  = 'Connected';
    connectBtn.textContent = 'Disconnect';
    connectBtn.classList.replace('btn-primary', 'btn-danger');
  } else {
    connDot.className      = 'dot';
    connLabel.textContent  = errorMsg || 'Not connected';
    connectBtn.textContent = 'Connect';
    connectBtn.classList.replace('btn-danger', 'btn-primary');
  }
}

async function sendSerialCommand(id) {
  if (!writer) return;
  try {
    await writer.write(new TextEncoder().encode(id + '\n'));
  } catch (err) {
    console.error('Send failed:', err);
    await disconnect();
  }
}

// ── Event listeners ───────────────────────────────────────────────
configFileInput.addEventListener('change', async (e) => {
  const file = e.target.files[0];
  if (!file) return;

  try {
    const text   = await file.text();
    const parsed = parseToml(text);
    applyConfig(parsed, text);
  } catch (err) {
    alert(`Could not parse config.toml:\n${err.message}`);
  }

  configFileInput.value = ''; // allow re-selecting the same file next time
});

reloadConfigBtn.addEventListener('click', () => configFileInput.click());
connectBtn.addEventListener('click', toggleConnection);

// ── Init ──────────────────────────────────────────────────────────
function init() {
  if (!navigator.serial) {
    noSerialWarning.classList.remove('hidden');
    connectBtn.disabled    = true;
    connectBtn.textContent = 'Not supported';
  }

  if (!loadFromStorage()) showConfigEmpty();
}

init();
