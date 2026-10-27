import sys

W, H = 1480, 900
F = "DejaVu Sans, Arial, sans-serif"
M = "DejaVu Sans Mono, Consolas, monospace"
TEL, CTL, MOD = "#1c71d8", "#e66100", "#16a34a"

o = []
a = o.append
a(f'<svg xmlns="http://www.w3.org/2000/svg" width="{W}" height="{H}" viewBox="0 0 {W} {H}" font-family="{F}">')
a('<defs>')
for n, c in (("tel", TEL), ("ctl", CTL), ("mod", MOD), ("gry", "#555")):
    a(f'<marker id="m{n}" viewBox="0 0 10 10" refX="9" refY="5" markerWidth="7" markerHeight="7" orient="auto-start-reverse"><path d="M0 0 L10 5 L0 10 z" fill="{c}"/></marker>')
a('</defs>')
a(f'<rect width="{W}" height="{H}" fill="#ffffff"/>')
a('<text x="40" y="44" font-size="24" font-weight="bold" fill="#1a1a1a">Arquitectura del Sistema Multisensor con Servidor Web ESP32</text>')
a('<text x="40" y="70" font-size="14" fill="#555">Práctica 2 (Unidad 2) · Hotspot Wi-Fi Propio (192.168.4.1) · Servidor HTTP Nativo (GET / POST) · 4 Modos con LED RGB y Relé</text>')

def box(x, y, w, h, fill, stroke, dash=False, rx=10):
    d = ' stroke-dasharray="8 6"' if dash else ""
    a(f'<rect x="{x}" y="{y}" width="{w}" height="{h}" rx="{rx}" fill="{fill}" stroke="{stroke}" stroke-width="2"{d}/>')

def txt(x, y, s, size=13, w="normal", fill="#222", anchor="start", font=None):
    ff = f' font-family="{font}"' if font else ""
    s_esc = s.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;")
    a(f'<text x="{x}" y="{y}" font-size="{size}" font-weight="{w}" fill="{fill}" text-anchor="{anchor}"{ff}>{s_esc}</text>')

def arrow(x1, y1, x2, y2, col, mk):
    a(f'<line x1="{x1}" y1="{y1}" x2="{x2}" y2="{y2}" stroke="{col}" stroke-width="2.6" marker-end="url(#m{mk})"/>')

# ---------- 1. Capa de Sensores y Actuadores (Izquierda) ----------
sx, sy, sw, sh = 35, 105, 340, 620
box(sx, sy, sw, sh, "#f8fafc", "#475569")
txt(sx+sw/2, sy+30, "1. Sensores y Actuadores Físicos", 16, "bold", "#1e293b", "middle")
txt(sx+sw/2, sy+50, "Conectados en Protoboard al ESP32", 12, "normal", "#64748b", "middle")

cards_hw = [
    ("GY-SHT41 (I²C 0x44) / DHT11", ["GPIO 21 (SDA) / GPIO 22 (SCL)", "Temperatura (°C) y Humedad (%)", "Muestreo cada 2000 ms"], TEL, 75),
    ("Fotocelda LDR + R 10kΩ", ["GPIO 34 (ADC1_CH6 · 12 bits)", "Media móvil 8 muestras → 0..100%", "Estados: OSCURO / MEDIO / ILUMINADO"], TEL, 185),
    ("Sensor de Movimiento PIR", ["GPIO 35 (Entrada digital 3.3V)", "Detección + ventana de retención 15 s"], TEL, 295),
    ("LED RGB Indicador de Modo", ["GPIO 25 (R), GPIO 26 (G), GPIO 27 (B)", "Azul: Manual · Verde: Auto LDR", "Amarillo: PIR · Rojo: Clima"], MOD, 390),
    ("Módulo Relé + OLED SSD1306", ["GPIO 23 (Relé) + I²C 0x3C (OLED)", "Conmutación con histéresis y", "confirmación física del estado real"], CTL, 500),
]

for title, lines, col, dy in cards_hw:
    box(sx+16, sy+dy, sw-32, 95 if len(lines)==3 else 80, "#ffffff", col, rx=7)
    txt(sx+28, sy+dy+22, title, 13, "bold", col)
    for i, ln in enumerate(lines):
        txt(sx+28, sy+dy+42+i*18, ln, 11.5, "normal", "#333")

# ---------- 2. ESP32 Firmware + Hotspot + Servidor Web (Centro) ----------
ex, ey, ew, eh = 435, 105, 455, 620
box(ex, ey, ew, eh, "#eef3fb", "#1f3a5f")
txt(ex+ew/2, ey+30, "2. Microcontrolador ESP32 DevKit V1", 17, "bold", "#1f3a5f", "middle")
txt(ex+ew/2, ey+50, "Firmware No Bloqueante (millis) + Hotspot + WebServer", 12, "normal", "#555", "middle")

# Hotspot block
box(ex+18, ey+70, ew-36, 95, "#ffffff", "#1f3a5f", rx=7)
txt(ex+32, ey+94, "Punto de Acceso Wi-Fi Propio (WiFi.softAP)", 13.5, "bold", "#1f3a5f")
txt(ex+32, ey+115, "• Red (SSID): ESP32-Multisensor (WPA2)", 12, "normal", "#333", font=M)
txt(ex+32, ey+135, "• Dirección IP fija: 192.168.4.1", 12, "normal", "#333", font=M)
txt(ex+32, ey+153, "• Sin dependencia de routers ni brokers externos", 11.5, "normal", "#555")

# Máquina de Estados (4 Modos)
box(ex+18, ey+180, ew-36, 215, "#ffffff", MOD, rx=7)
txt(ex+32, ey+204, "Máquina de Estados — 4 Modos de Operación", 13.5, "bold", MOD)
modes_info = [
    ("Modo 0 — MANUAL (LED Azul):", "El usuario enciende/apaga el relé desde la web."),
    ("Modo 1 — AUTO_LDR (LED Verde):", "Relé ON si Luz < 35% · OFF si Luz > 45%."),
    ("Modo 2 — AUTO_PIR (LED Amarillo):", "Relé ON con movimiento PIR (retención 15 s)."),
    ("Modo 3 — AUTO_CLIMA (LED Rojo):", "Relé ON si Temp >= 28°C o Humedad >= 75%."),
]
for i, (m_title, m_desc) in enumerate(modes_info):
    y_m = ey + 228 + i * 40
    txt(ex+32, y_m, m_title, 12, "bold", "#1e293b")
    txt(ex+32, y_m+17, m_desc, 11.5, "normal", "#475569")

# Servidor Web HTTP Nativo
box(ex+18, ey+410, ew-36, 195, "#ffffff", CTL, rx=7)
txt(ex+32, ey+434, "Servidor Web Nativo (WebServer.h · Puerto 80)", 13.5, "bold", CTL)
endpoints = [
    ("GET  /", "Sirve Dashboard HTML5 + CSS3 (PROGMEM)"),
    ("GET  /status", "Entrega JSON con sensores, modo y relé"),
    ("POST /mode", "Recibe mode=0..3 → cambia modo y RGB"),
    ("POST /relay", "Recibe state=ON|OFF → relé en Manual"),
]
for i, (ep, desc) in enumerate(endpoints):
    y_e = ey + 460 + i * 34
    txt(ex+30, y_e, ep, 11.5, "bold", CTL, font=M)
    txt(ex+142, y_e, desc, 11.5, "normal", "#333")

# Arrows Hardware <-> ESP32
arrow(sx+sw, 240, ex, 240, TEL, "tel")
arrow(ex, 520, sx+sw, 520, CTL, "ctl")

# ---------- 3. Cliente Web / Navegador (Derecha) ----------
cx, cy, cw, ch = 1075, 105, 370, 620
box(cx, cy, cw, ch, "#fdf8f3", "#8f3b0b")
txt(cx+cw/2, cy+30, "3. Interfaz Web (Navegador)", 17, "bold", "#8f3b0b", "middle")
txt(cx+cw/2, cy+50, "http://192.168.4.1 (Laptop o Smartphone)", 12, "normal", "#555", "middle", font=M)

ui_boxes = [
    ("Tarjetas de Telemetría (HTML + CSS)", [
        "• Temperatura (°C) y Humedad (%)",
        "• Iluminación LDR (% + barra + estado)",
        "• Estado del PIR (MOVIMIENTO / LIBRE)",
    ], TEL, 80, 115),
    ("Selector de Modo e Indicador RGB", [
        "• Vista previa del color del LED RGB activo",
        "• 4 Botones interactivos (POST /mode)",
        "• Actualiza regla activa en pantalla",
    ], MOD, 215, 115),
    ("Control del Módulo Relé (Lazo Cerrado)", [
        "• Muestra confirmación real: ON / OFF",
        "• Botones ENCENDER / APAGAR (POST /relay)",
        "• Activos en Modo Manual; bloqueados en Auto",
    ], CTL, 350, 120),
    ("Sincronización Ligera sin Recarga", [
        "• Consulta GET /status cada 1.5 s",
        "• Refleja cambios físicos confirmados por",
        "  la ESP32 sin parpadeo de página",
    ], "#475569", 490, 110),
]

for title, lines, col, dy, bh_card in ui_boxes:
    box(cx+18, cy+dy, cw-36, bh_card, "#ffffff", col, rx=7)
    txt(cx+32, cy+dy+24, title, 13, "bold", col)
    for i, ln in enumerate(lines):
        txt(cx+32, cy+dy+46+i*20, ln, 11.5, "normal", "#333")

# Arrows ESP32 <-> Navegador
mid_col_x = (ex + ew + cx) / 2
arrow(ex+ew+4, 285, cx-4, 285, TEL, "tel")
txt(mid_col_x, 256, "GET / y GET /status", 11, "bold", TEL, "middle", font=M)
txt(mid_col_x, 273, "HTML/CSS + JSON", 11, "normal", "#444", "middle")

arrow(cx-4, 445, ex+ew+4, 445, CTL, "ctl")
txt(mid_col_x, 416, "POST /mode y /relay", 11, "bold", CTL, "middle", font=M)
txt(mid_col_x, 433, "Comandos + Confirmación", 11, "normal", "#444", "middle")

# ---------- Resumen Inferior del Ciclo de Operacion ----------
box(35, 750, 1410, 120, "#f8fafc", "#cbd5e1", rx=8)
txt(55, 778, "Flujo de Operación y Confirmación de Estado:", 14, "bold", "#1e293b")
steps = [
    "1. La ESP32 inicia su Hotspot Wi-Fi (192.168.4.1) y lee continuamente SHT41/DHT11, LDR (GPIO 34) y PIR (GPIO 35) mediante millis().",
    "2. El usuario abre http://192.168.4.1 en su navegador (GET /) y visualiza todas las variables actualizadas vía GET /status.",
    "3. Al elegir un modo (POST /mode) o conmutar el relé en Modo Manual (POST /relay), la ESP32 actúa físicamente sobre el LED RGB y el Relé y responde con el estado real confirmado.",
]
for i, st in enumerate(steps):
    txt(55, 804 + i*23, st, 12.5, "normal", "#334155")

a('</svg>')
open(sys.argv[1], "w").write("\n".join(o))

