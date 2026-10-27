#ifndef WEB_DASHBOARD_H
#define WEB_DASHBOARD_H

#include <Arduino.h>

// ============================================================================
//  Interfaz Web HTML5 + CSS3 almacenada en memoria Flash (PROGMEM)
//  - Diseno profesional inspirado en el tema IoT (#2c3e50 / #e66100)
//  - Peticiones HTTP estandar:
//      * GET  /        -> Carga esta interfaz web
//      * GET  /status  -> Consulta JSON con valores reales de sensores y estado
//      * POST /mode    -> Cambia el modo de operacion (mode=0..3)
//      * POST /relay   -> Enciende/apaga el rele en modo Manual (state=ON|OFF)
// ============================================================================

const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="es">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>ESP32 Multisensor IoT — Panel de Control</title>
  <style>
    :root {
      --header-bg: #2c3e50;
      --accent: #e66100;
      --bg-page: #eef1f5;
      --card-bg: #ffffff;
      --text-main: #1e293b;
      --text-muted: #64748b;
      --border: #dce3ec;
      --blue: #0066ff;
      --green: #16a34a;
      --yellow: #eab308;
      --red: #dc2626;
    }
    * { box-sizing: border-box; margin: 0; padding: 0; }
    body {
      font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Arial, sans-serif;
      background-color: var(--bg-page);
      color: var(--text-main);
      line-height: 1.45;
      padding-bottom: 32px;
    }
    header {
      background-color: var(--header-bg);
      color: #ffffff;
      padding: 16px 24px;
      display: flex;
      flex-wrap: wrap;
      justify-content: space-between;
      align-items: center;
      gap: 12px;
      box-shadow: 0 2px 8px rgba(0,0,0,0.15);
    }
    .brand h1 { font-size: 1.25rem; font-weight: 700; letter-spacing: 0.2px; }
    .brand p { font-size: 0.82rem; color: #cbd5e1; margin-top: 2px; }
    .status-pill {
      display: inline-flex;
      align-items: center;
      gap: 8px;
      background: rgba(255,255,255,0.12);
      padding: 6px 14px;
      border-radius: 999px;
      font-size: 0.85rem;
      font-weight: 600;
    }
    .dot {
      width: 10px;
      height: 10px;
      border-radius: 50%;
      background-color: #22c55e;
      box-shadow: 0 0 6px #22c55e;
    }
    .dot.offline { background-color: #ef4444; box-shadow: 0 0 6px #ef4444; }

    .container {
      max-width: 1080px;
      margin: 22px auto;
      padding: 0 16px;
    }
    .section-title {
      font-size: 0.9rem;
      text-transform: uppercase;
      letter-spacing: 0.8px;
      color: var(--text-muted);
      font-weight: 700;
      margin: 18px 0 10px;
    }
    .grid-sensors {
      display: grid;
      grid-template-columns: repeat(auto-fit, minmax(230px, 1fr));
      gap: 16px;
    }
    .grid-control {
      display: grid;
      grid-template-columns: repeat(auto-fit, minmax(320px, 1fr));
      gap: 16px;
    }
    .card {
      background: var(--card-bg);
      border: 1px solid var(--border);
      border-radius: 10px;
      padding: 18px;
      box-shadow: 0 1px 4px rgba(15, 23, 42, 0.05);
    }
    .card-header {
      display: flex;
      justify-content: space-between;
      align-items: center;
      margin-bottom: 10px;
    }
    .card-title {
      font-size: 0.92rem;
      font-weight: 600;
      color: var(--text-muted);
    }
    .sensor-tag {
      font-size: 0.72rem;
      font-weight: 700;
      padding: 2px 8px;
      border-radius: 4px;
      background: #f1f5f9;
      color: #475569;
    }
    .metric {
      font-size: 2.1rem;
      font-weight: 700;
      color: var(--text-main);
      margin: 4px 0 8px;
    }
    .metric small {
      font-size: 1rem;
      font-weight: 500;
      color: var(--text-muted);
    }
    .progress-bar {
      width: 100%;
      height: 8px;
      background: #e2e8f0;
      border-radius: 999px;
      overflow: hidden;
      margin: 8px 0 6px;
    }
    .progress-fill {
      height: 100%;
      width: 0%;
      background: var(--accent);
      transition: width 0.35s ease, background-color 0.35s ease;
    }
    .subtext {
      font-size: 0.82rem;
      color: var(--text-muted);
    }

    /* Indicadores de estado (PIR / Rele) */
    .state-badge {
      display: inline-block;
      padding: 8px 14px;
      border-radius: 8px;
      font-weight: 700;
      font-size: 0.95rem;
      margin: 6px 0 8px;
    }
    .badge-active {
      background-color: #dcfce7;
      color: #15803d;
      border: 1px solid #86efac;
    }
    .badge-inactive {
      background-color: #f1f5f9;
      color: #475569;
      border: 1px solid #cbd5e1;
    }
    .badge-alert {
      background-color: #fef3c7;
      color: #b45309;
      border: 1px solid #fde68a;
    }

    /* Indicador LED RGB */
    .rgb-box {
      display: flex;
      align-items: center;
      gap: 14px;
      padding: 12px 14px;
      background: #f8fafc;
      border: 1px solid var(--border);
      border-radius: 8px;
      margin-bottom: 14px;
    }
    .rgb-led {
      width: 28px;
      height: 28px;
      border-radius: 50%;
      background-color: var(--blue);
      box-shadow: 0 0 12px var(--blue);
      border: 2px solid #ffffff;
      flex-shrink: 0;
      transition: background-color 0.25s ease, box-shadow 0.25s ease;
    }
    .rgb-info strong { display: block; font-size: 0.95rem; }
    .rgb-info span { font-size: 0.82rem; color: var(--text-muted); }

    /* Botones de Modo (POST /mode) */
    .mode-grid {
      display: grid;
      grid-template-columns: 1fr 1fr;
      gap: 10px;
    }
    .mode-btn {
      display: flex;
      align-items: center;
      gap: 8px;
      width: 100%;
      padding: 11px 12px;
      border: 2px solid var(--border);
      border-radius: 8px;
      background: #ffffff;
      color: var(--text-main);
      font-size: 0.88rem;
      font-weight: 600;
      cursor: pointer;
      text-align: left;
      transition: all 0.2s ease;
    }
    .mode-btn:hover { border-color: #94a3b8; background: #f8fafc; }
    .mode-btn.active {
      border-color: var(--header-bg);
      background: #f1f5f9;
      box-shadow: inset 0 0 0 1px var(--header-bg);
    }
    .color-dot {
      width: 12px;
      height: 12px;
      border-radius: 50%;
      flex-shrink: 0;
    }

    /* Control de Rele (POST /relay) */
    .relay-controls {
      display: flex;
      gap: 12px;
      margin-top: 14px;
    }
    .btn-relay {
      flex: 1;
      padding: 12px 16px;
      border: none;
      border-radius: 8px;
      font-size: 0.92rem;
      font-weight: 700;
      cursor: pointer;
      transition: opacity 0.2s ease, transform 0.1s ease;
    }
    .btn-relay:active { transform: scale(0.98); }
    .btn-on { background-color: var(--green); color: #ffffff; }
    .btn-off { background-color: #475569; color: #ffffff; }
    .btn-relay:disabled {
      opacity: 0.4;
      cursor: not-allowed;
    }
    .notice-box {
      margin-top: 12px;
      padding: 10px 12px;
      border-radius: 6px;
      background: #fff7ed;
      border-left: 4px solid var(--accent);
      font-size: 0.82rem;
      color: #9a3412;
    }
    footer {
      text-align: center;
      margin-top: 28px;
      font-size: 0.8rem;
      color: var(--text-muted);
    }
  </style>
</head>
<body>

  <header>
    <div class="brand">
      <h1>Sistema Multisensor IoT — ESP32 Web Server</h1>
      <p>Práctica 2 · Unidad 2 · Monitoreo y Control Multimodo con Confirmación</p>
    </div>
    <div class="status-pill">
      <span id="connDot" class="dot"></span>
      <span id="connText">ESP32 En Línea</span>
    </div>
  </header>

  <main class="container">

    <div class="section-title">1. Telemetría de Sensores en Tiempo Real</div>
    <div class="grid-sensors">

      <!-- Temperatura -->
      <div class="card">
        <div class="card-header">
          <span class="card-title">Temperatura</span>
          <span id="tempSensorTag" class="sensor-tag">SHT41 / DHT11</span>
        </div>
        <div class="metric"><span id="valTemp">--.-</span> <small>°C</small></div>
        <div class="progress-bar">
          <div id="barTemp" class="progress-fill" style="background:#3584e4;"></div>
        </div>
        <div id="descTemp" class="subtext">Rango confortable: 18 – 28 °C</div>
      </div>

      <!-- Humedad -->
      <div class="card">
        <div class="card-header">
          <span class="card-title">Humedad Relativa</span>
          <span class="sensor-tag">Ambiental</span>
        </div>
        <div class="metric"><span id="valHum">--.-</span> <small>%</small></div>
        <div class="progress-bar">
          <div id="barHum" class="progress-fill" style="background:#0ea5e9;"></div>
        </div>
        <div id="descHum" class="subtext">Rango confortable: 30 – 65 %</div>
      </div>

      <!-- Iluminacion LDR -->
      <div class="card">
        <div class="card-header">
          <span class="card-title">Iluminación (LDR)</span>
          <span class="sensor-tag">GPIO 34 · ADC1</span>
        </div>
        <div class="metric"><span id="valLdr">--</span> <small>%</small></div>
        <div class="progress-bar">
          <div id="barLdr" class="progress-fill" style="background:#eab308;"></div>
        </div>
        <div class="subtext">
          Estado: <strong id="stateLdr">---</strong> (ADC: <span id="rawLdr">0</span>)
        </div>
      </div>

      <!-- Sensor PIR -->
      <div class="card">
        <div class="card-header">
          <span class="card-title">Movimiento (PIR)</span>
          <span class="sensor-tag">GPIO 35</span>
        </div>
        <div>
          <span id="badgePir" class="state-badge badge-inactive">SIN MOVIMIENTO</span>
        </div>
        <div id="descPir" class="subtext">Retención tras detección: 15 s</div>
      </div>

    </div>

    <div class="section-title">2. Modos de Operación (LED RGB) y Control del Módulo Relé</div>
    <div class="grid-control">

      <!-- Seleccion de Modo + Indicador LED RGB -->
      <div class="card">
        <div class="card-header">
          <span class="card-title">Modo de Funcionamiento e Indicador LED RGB</span>
          <span class="sensor-tag">GPIO 25, 26, 27</span>
        </div>

        <div class="rgb-box">
          <div id="rgbPreview" class="rgb-led"></div>
          <div class="rgb-info">
            <strong id="modeTitle">Modo Actual: MANUAL</strong>
            <span id="modeRule">LED RGB: Azul · Control directo desde botones web</span>
          </div>
        </div>

        <form action="/mode" method="POST" onsubmit="return false;">
          <div class="mode-grid">
            <button type="button" id="btnMode0" class="mode-btn active" onclick="cambiarModo(0)">
              <span class="color-dot" style="background:#0066ff;"></span>
              <span>0. Manual (Azul)</span>
            </button>
            <button type="button" id="btnMode1" class="mode-btn" onclick="cambiarModo(1)">
              <span class="color-dot" style="background:#16a34a;"></span>
              <span>1. Auto Luz LDR (Verde)</span>
            </button>
            <button type="button" id="btnMode2" class="mode-btn" onclick="cambiarModo(2)">
              <span class="color-dot" style="background:#eab308;"></span>
              <span>2. Auto PIR (Amarillo)</span>
            </button>
            <button type="button" id="btnMode3" class="mode-btn" onclick="cambiarModo(3)">
              <span class="color-dot" style="background:#dc2626;"></span>
              <span>3. Auto Clima (Rojo)</span>
            </button>
          </div>
        </form>
      </div>

      <!-- Estado y Control del Modulo Rele -->
      <div class="card">
        <div class="card-header">
          <span class="card-title">Módulo Relé (Salida de Potencia)</span>
          <span class="sensor-tag">GPIO 23</span>
        </div>

        <div>
          <span id="badgeRelay" class="state-badge badge-inactive">RELÉ APAGADO (OFF)</span>
        </div>
        <div id="relayReason" class="subtext">Confirmación física en tiempo real desde la ESP32.</div>

        <form action="/relay" method="POST" onsubmit="return false;">
          <div class="relay-controls">
            <button type="button" id="btnRelayOn" class="btn-relay btn-on" onclick="controlarRele('ON')">
              ENCENDER RELÉ
            </button>
            <button type="button" id="btnRelayOff" class="btn-relay btn-off" onclick="controlarRele('OFF')">
              APAGAR RELÉ
            </button>
          </div>
        </form>

        <div id="manualNotice" class="notice-box">
          En <strong>Modo Manual (Azul)</strong> puedes encender o apagar el relé directamente con estos botones.
        </div>
      </div>

    </div>

  </main>

  <footer>
    Universidad Politécnica de Yucatán (UPY) · Ciclo Escolar 2026 · Servidor Web ESP32 (HTTP GET &amp; POST)
  </footer>

  <script>
    const REGLAS_MODO = [
      "LED RGB: Azul · El usuario controla el relé manualmente con los botones.",
      "LED RGB: Verde · Relé ON si Luz < 35% (Oscuro) · OFF si Luz > 45%.",
      "LED RGB: Amarillo · Relé ON al detectar movimiento PIR (retención 15 s).",
      "LED RGB: Rojo · Relé ON si Temp >= 28 °C o Humedad >= 75%."
    ];

    function aplicarEstadoUI(d) {
      document.getElementById('connDot').className = 'dot';
      document.getElementById('connText').textContent = 'ESP32 En Línea (' + d.uptime + ' s)';

      // Temperatura y Humedad
      document.getElementById('valTemp').textContent = Number(d.temp).toFixed(1);
      document.getElementById('barTemp').style.width = Math.min(100, Math.max(0, (d.temp / 50) * 100)) + '%';
      document.getElementById('tempSensorTag').textContent = d.sensor || 'SHT41';

      document.getElementById('valHum').textContent = Number(d.hum).toFixed(1);
      document.getElementById('barHum').style.width = Math.min(100, Math.max(0, d.hum)) + '%';

      // LDR
      document.getElementById('valLdr').textContent = d.ldr_pct;
      document.getElementById('rawLdr').textContent = d.ldr_raw;
      document.getElementById('stateLdr').textContent = d.ldr_state;
      document.getElementById('barLdr').style.width = d.ldr_pct + '%';

      // PIR
      const badgePir = document.getElementById('badgePir');
      if (d.pir) {
        badgePir.textContent = 'MOVIMIENTO DETECTADO';
        badgePir.className = 'state-badge badge-alert';
      } else {
        badgePir.textContent = 'SIN MOVIMIENTO';
        badgePir.className = 'state-badge badge-inactive';
      }

      // Modo y LED RGB
      document.getElementById('modeTitle').textContent = 'Modo Actual: ' + d.mode_name;
      document.getElementById('modeRule').textContent = REGLAS_MODO[d.mode] || '';
      const rgbEl = document.getElementById('rgbPreview');
      rgbEl.style.backgroundColor = d.rgb;
      rgbEl.style.boxShadow = '0 0 12px ' + d.rgb;

      for (let i = 0; i < 4; i++) {
        const b = document.getElementById('btnMode' + i);
        if (b) b.className = (i === d.mode) ? 'mode-btn active' : 'mode-btn';
      }

      // Estado confirmado del Rele
      const badgeRelay = document.getElementById('badgeRelay');
      if (d.relay) {
        badgeRelay.textContent = '🟢 RELÉ ENCENDIDO (ON)';
        badgeRelay.className = 'state-badge badge-active';
      } else {
        badgeRelay.textContent = '⚫ RELÉ APAGADO (OFF)';
        badgeRelay.className = 'state-badge badge-inactive';
      }

      // Habilitar botones manuales solo en Modo 0 (MANUAL)
      const esManual = (d.mode === 0);
      document.getElementById('btnRelayOn').disabled = !esManual;
      document.getElementById('btnRelayOff').disabled = !esManual;
      document.getElementById('manualNotice').innerHTML = esManual
        ? 'En <strong>Modo Manual (Azul)</strong> puedes encender o apagar el relé directamente con estos botones.'
        : 'Control automático activo (<strong>' + d.mode_name + '</strong>). Cambia a <strong>Modo 0 (Manual)</strong> para usar los botones del relé.';
    }

    // Peticion GET /status para consultar el estado actual
    async function consultarEstado() {
      try {
        const res = await fetch('/status', { method: 'GET', cache: 'no-store' });
        if (!res.ok) throw new Error('HTTP ' + res.status);
        const data = await res.json();
        aplicarEstadoUI(data);
      } catch (e) {
        document.getElementById('connDot').className = 'dot offline';
        document.getElementById('connText').textContent = 'Sin conexión con ESP32';
      }
    }

    // Peticion POST /mode para cambiar el modo de operacion
    async function cambiarModo(nuevoModo) {
      try {
        const res = await fetch('/mode', {
          method: 'POST',
          headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
          body: 'mode=' + encodeURIComponent(nuevoModo)
        });
        if (res.ok) {
          const data = await res.json();
          aplicarEstadoUI(data);
        }
      } catch (e) {
        console.error(e);
      }
    }

    // Peticion POST /relay para conmutar el rele en Modo Manual
    async function controlarRele(estado) {
      try {
        const res = await fetch('/relay', {
          method: 'POST',
          headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
          body: 'state=' + encodeURIComponent(estado)
        });
        if (res.ok) {
          const data = await res.json();
          aplicarEstadoUI(data);
        }
      } catch (e) {
        console.error(e);
      }
    }

    consultarEstado();
    setInterval(consultarEstado, 1500);
  </script>
</body>
</html>
)rawliteral";

#endif

