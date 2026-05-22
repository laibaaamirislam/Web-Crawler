/**
 * script.js — Frontend Logic for Parallel Web Crawler
 * PDC Project: Handles UI, API calls, and real-time log rendering
 */

/* ── Load predefined sites on page load ── */
document.addEventListener("DOMContentLoaded", () => {
  loadSites();
});

/**
 * Fetch predefined sites from backend and render as badges.
 */
async function loadSites() {
  try {
    const resp = await fetch("/sites");
    const data = await resp.json();
    const container = document.getElementById("sites-list");
    container.innerHTML = "";
    data.sites.forEach((url, i) => {
      const badge = document.createElement("span");
      badge.className = "site-badge";
      badge.textContent = `Thread-${i + 1} → ${url}`;
      container.appendChild(badge);
    });
  } catch (err) {
    document.getElementById("sites-list").innerHTML =
      `<span style="color: var(--red); font-family: var(--mono); font-size:12px;">Could not load sites — is the server running?</span>`;
  }
}

/**
 * Main function: Validates inputs, calls backend, renders results.
 */
async function startCrawl() {
  /* ── Gather inputs ── */
  const keyword  = document.getElementById("keyword").value.trim();
  const maxDepth = parseInt(document.getElementById("max_depth").value) || 1;
  const maxPages = parseInt(document.getElementById("max_pages").value) || 6;

  if (!keyword) {
    flashInput("keyword", "Please enter a keyword!");
    return;
  }

  /* ── UI: enter loading state ── */
  setLoading(true);
  showSection("logs-section");
  hideSection("results-section");
  clearLogs();
  addLog("🕷  Sending crawl request to backend…", "log-start");
  addLog(`🔑 Keyword: "${keyword}" | Depth: ${maxDepth} | Pages/site: ${maxPages}`, "log-info");
  animateProgress();

  try {
    /* ── POST to C++ backend ── */
    const response = await fetch("/crawl", {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ keyword, max_depth: maxDepth, max_pages: maxPages }),
    });

    if (!response.ok) {
      const err = await response.json();
      throw new Error(err.error || `HTTP ${response.status}`);
    }

    const data = await response.json();

    /* ── Render logs returned from backend ── */
    clearLogs();
    if (data.logs && data.logs.length) {
      // Stream-style rendering: show logs with a tiny delay for effect
      await renderLogsAnimated(data.logs);
    }

    /* ── Render results ── */
    renderResults(data, keyword);

  } catch (err) {
    addLog(`❌ Error: ${err.message}`, "log-error");
    addLog("Make sure the C++ backend is running: ./build/parallel_web_crawler", "log-info");
  } finally {
    setLoading(false);
    stopProgress();
  }
}

/**
 * Render all log lines with a short animated delay between each.
 */
async function renderLogsAnimated(logs) {
  for (const line of logs) {
    addLog(line);
    await sleep(18);  // slight delay for terminal "streaming" feel
  }
  // Auto-scroll to bottom
  const console_ = document.getElementById("log-console");
  console_.scrollTop = console_.scrollHeight;
}

/**
 * Classify a log line and return the appropriate CSS class.
 */
function classifyLog(line) {
  if (line.includes("✅") || line.includes("KEYWORD FOUND")) return "log-found";
  if (line.includes("❌"))                                     return "log-miss";
  if (line.includes("🔍"))                                     return "log-crawl";
  if (line.includes("🧵") || line.includes("Worker"))         return "log-thread";
  if (line.includes("🔗"))                                     return "log-link";
  if (line.includes("🚀") || line.includes("🌐"))             return "log-start";
  if (line.includes("🏁") || line.includes("✔"))              return "log-done";
  if (line.includes("⚠") || line.includes("Error"))           return "log-error";
  if (line.includes("==="))                                    return "log-start";
  return "log-info";
}

/**
 * Add a single line to the log console.
 */
function addLog(message, cssClass = null) {
  const console_ = document.getElementById("log-console");
  const div = document.createElement("div");
  div.className = `log-entry ${cssClass || classifyLog(message)}`;
  div.textContent = message;
  console_.appendChild(div);
  // Keep scrolled to bottom
  console_.scrollTop = console_.scrollHeight;
}

/** Clear the log console. */
function clearLogs() {
  document.getElementById("log-console").innerHTML = "";
}

/**
 * Render results panel with stats and matched URL cards.
 */
function renderResults(data, keyword) {
  showSection("results-section");

  /* Stats */
  const statsRow = document.getElementById("stats-row");
  statsRow.innerHTML = `
    <div class="stat-card">
      <span class="stat-value">${data.total_crawled || 0}</span>
      <div class="stat-label">Pages Crawled</div>
    </div>
    <div class="stat-card">
      <span class="stat-value" style="color: var(--green)">${data.total_matches || 0}</span>
      <div class="stat-label">Keyword Matches</div>
    </div>
    <div class="stat-card">
      <span class="stat-value" style="color: var(--yellow)">${data.logs ? data.logs.length : 0}</span>
      <div class="stat-label">Log Entries</div>
    </div>
  `;

  /* Matched URLs */
  const matchesList = document.getElementById("matches-list");
  const noResults   = document.getElementById("no-results");
  matchesList.innerHTML = "";

  if (!data.matched_urls || data.matched_urls.length === 0) {
    noResults.style.display = "block";
    return;
  }

  noResults.style.display = "none";

  data.matched_urls.forEach((item, idx) => {
    const url    = typeof item === "string" ? item : item.url;
    const site   = typeof item === "object" ? item.site : "";
    const card   = document.createElement("div");
    card.className = "match-item";
    card.style.animationDelay = `${idx * 60}ms`;
    
    // Create viewer URL with keyword highlight
    const viewerUrl = `/viewer?url=${encodeURIComponent(url)}&keyword=${encodeURIComponent(keyword)}`;
    
    card.innerHTML = `
      <span class="match-icon">✅</span>
      <div>
        <div class="match-url"><a href="${escapeHtml(viewerUrl)}" target="_blank" rel="noopener">${escapeHtml(url)}</a></div>
        ${site ? `<div class="match-domain">📌 Site: ${escapeHtml(site)}</div>` : ""}
      </div>
    `;
    matchesList.appendChild(card);
  });
}

/* ── Progress bar animation ── */
let progressInterval = null;
let progressVal = 0;

function animateProgress() {
  const wrap  = document.getElementById("progress-wrap");
  const bar   = document.getElementById("progress-bar");
  const label = document.getElementById("progress-label");
  wrap.style.display = "block";
  progressVal = 0;

  const msgs = [
    "Initializing threads…",
    "Launching site crawlers…",
    "BFS queue running…",
    "Workers processing pages…",
    "Searching for keyword…",
    "Almost done…",
  ];
  let msgIdx = 0;

  progressInterval = setInterval(() => {
    // Slow fake progress that never quite reaches 100%
    if (progressVal < 85) {
      progressVal += Math.random() * 3;
      bar.style.width = `${Math.min(progressVal, 90)}%`;
    }
    if (msgIdx < msgs.length) {
      label.textContent = msgs[Math.floor(progressVal / 16)] || msgs[msgs.length - 1];
    }
  }, 600);
}

function stopProgress() {
  if (progressInterval) clearInterval(progressInterval);
  const bar   = document.getElementById("progress-bar");
  const label = document.getElementById("progress-label");
  bar.style.width = "100%";
  label.textContent = "Complete!";
  setTimeout(() => {
    document.getElementById("progress-wrap").style.display = "none";
  }, 1200);
}

/* ── UI helpers ── */
function setLoading(state) {
  const btn = document.getElementById("crawl-btn");
  if (state) {
    btn.disabled = true;
    btn.querySelector(".btn-text").textContent = "Crawling…";
    btn.querySelector(".btn-icon").textContent = "⏳";
  } else {
    btn.disabled = false;
    btn.querySelector(".btn-text").textContent = "Start Crawling";
    btn.querySelector(".btn-icon").textContent = "▶";
  }
}

function showSection(id) {
  document.getElementById(id).style.display = "block";
}
function hideSection(id) {
  document.getElementById(id).style.display = "none";
}

function flashInput(id, msg) {
  const el = document.getElementById(id);
  el.style.borderColor = "var(--red)";
  el.placeholder = msg;
  el.focus();
  setTimeout(() => {
    el.style.borderColor = "";
    el.placeholder = "e.g. parallel, python, algorithm…";
  }, 2500);
}

function escapeHtml(str) {
  return String(str)
    .replace(/&/g, "&amp;")
    .replace(/</g, "&lt;")
    .replace(/>/g, "&gt;")
    .replace(/"/g, "&quot;");
}

function sleep(ms) {
  return new Promise((r) => setTimeout(r, ms));
}
