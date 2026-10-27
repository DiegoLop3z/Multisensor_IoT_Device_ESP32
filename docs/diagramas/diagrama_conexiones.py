import sys

W, H = 1480, 1220
F = "DejaVu Sans, Arial, sans-serif"
M = "DejaVu Sans Mono, Consolas, monospace"
C = {
    "3v3": "#d62728",
    "5v": "#ff6b35",
    "gnd": "#222222",
    "sda": "#1c71d8",
    "scl": "#e5a50a",
    "ldr": "#9141ac",
    "pir": "#008080",
    "rel": "#26a269",
    "rgb_r": "#e01b24",
    "rgb_g": "#2ec27e",
    "rgb_b": "#3584e4",
}

o = []
a = o.append
a(f'<svg xmlns="http://www.w3.org/2000/svg" width="{W}" height="{H}" viewBox="0 0 {W} {H}" font-family="{F}">')
a(f'<rect width="{W}" height="{H}" fill="#ffffff"/>')
a('<text x="40" y="44" font-size="24" font-weight="bold" fill="#1a1a1a">Diagrama Electrónico de Conexiones — Sistema Multisensor ESP32</text>')
a('<text x="40" y="70" font-size="14" fill="#555">Práctica 2 (Unidad 2) · SHT41 + OLED (I²C GPIO 21/22) · LDR (GPIO 34 ADC1) · PIR (GPIO 35) · LED RGB (GPIO 25/26/27) · Relé (GPIO 23)</text>')

def box(x, y, w, h, fill, stroke, rx=8):
    a(f'<rect x="{x}" y="{y}" width="{w}" height="{h}" rx="{rx}" fill="{fill}" stroke="{stroke}" stroke-width="2"/>')

def txt(x, y, s, size=12, weight="normal", fill="#222", anchor="start", font=None):
    ff = f' font-family="{font}"' if font else ""
    s_esc = s.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;")
    a(f'<text x="{x}" y="{y}" font-size="{size}" font-weight="{weight}" fill="{fill}" text-anchor="{anchor}"{ff}>{s_esc}</text>')

def wire(pts, col, w=3.2):
    d = "M " + " L ".join(f"{x} {y}" for x, y in pts)
    a(f'<path d="{d}" fill="none" stroke="{col}" stroke-width="{w}" stroke-linecap="round" stroke-linejoin="round"/>')

def dot(x, y, col):
    a(f'<circle cx="{x}" cy="{y}" r="5" fill="{col}"/>')

# ---- ESP32 DevKit V1 (Central) ----
bx, by, bw, bh = 580, 105, 300, 625
box(bx, by, bw, bh, "#1f3a5f", "#0f2238", rx=12)
a('<path d="M680 120 h20 v14 h20 v-14 h20 v14 h20 v-14 h20" fill="none" stroke="#c9a227" stroke-width="3"/>')
box(670, 148, 120, 230, "#c0c4c8", "#8a8f94", rx=4)
txt(730, 260, "ESP-WROOM-32", 11, "bold", "#333", "middle")
txt(730, 415, "ESP32 DevKit V1", 16, "bold", "#fff", "middle")
txt(730, 437, "(Hotspot Wi-Fi 192.168.4.1)", 12, "normal", "#cfd8e3", "middle")
a(f'<rect x="700" y="{by+bh-4}" width="60" height="20" rx="3" fill="#9aa0a6" stroke="#5f6368"/>')
txt(730, by+bh+34, "USB (Alimentación 5V + Datos)", 11, "normal", "#555", "middle")

left_pins  = ["EN","VP","VN","D34","D35","D32","D33","D25","D26","D27","D14","D12","D13","GND","VIN"]
right_pins = ["D23","D22","TX0","RX0","D21","D19","D18","D5","TX2","RX2","D4","D2","D15","GND","3V3"]

used_left  = {"D34": "ldr", "D35": "pir", "D25": "rgb_r", "D26": "rgb_g", "D27": "rgb_b", "GND": "gnd", "VIN": "5v"}
used_right = {"D23": "rel", "D22": "scl", "D21": "sda", "GND": "gnd", "3V3": "3v3"}

py_l, py_r = {}, {}
for i, (lp, rp) in enumerate(zip(left_pins, right_pins)):
    y = 185 + i * 36
    ul = lp in used_left
    cl = C[used_left[lp]] if ul else "#d4af37"
    a(f'<circle cx="{bx+16}" cy="{y}" r="{6.5 if ul else 4.5}" fill="{cl}" stroke="#fff" stroke-width="{1.5 if ul else 0}"/>')
    txt(bx+28, y+4, lp, 12, "bold" if ul else "normal", "#fff" if ul else "#cfd8e3")
    py_l[lp] = y

    ur = rp in used_right
    cr = C[used_right[rp]] if ur else "#d4af37"
    a(f'<circle cx="{bx+bw-16}" cy="{y}" r="{6.5 if ur else 4.5}" fill="{cr}" stroke="#fff" stroke-width="{1.5 if ur else 0}"/>')
    txt(bx+bw-28, y+4, rp, 12, "bold" if ur else "normal", "#fff" if ur else "#cfd8e3", "end")
    py_r[rp] = y

# ==============================================================================
# LADO IZQUIERDO: LDR (GPIO 34), PIR (GPIO 35), LED RGB (GPIO 25, 26, 27)
# ==============================================================================

# 1. Modulo / Circuito LDR (Divisor de tension en GPIO 34)
lx, ly, lw, lh = 40, 105, 420, 145
box(lx, ly, lw, lh, "#f8f0fc", "#9141ac")
txt(lx+16, ly+26, "Sensor de Iluminación — LDR (ADC1)", 14, "bold", "#612078")
txt(lx+16, ly+46, "Divisor: 3.3V → LDR → GPIO 34 → R 10kΩ → GND", 11.5, "normal", "#444")
txt(lx+16, ly+68, "• Pin ADC1_CH6 (GPIO 34): compatible con Wi-Fi activo", 11.5, "normal", "#333")
txt(lx+16, ly+88, "• Rango ADC 12 bits: 0..4095 → 0..100%", 11.5, "normal", "#333")
txt(lx+lw-14, ly+124, "Nodo Divisor (OUT) → GPIO 34", 11.5, "bold", "#612078", "end")
wire([(lx+lw, ly+120), (505, ly+120), (505, py_l["D34"]), (bx+16, py_l["D34"])], C["ldr"])

# 2. Sensor PIR (GPIO 35)
px, py, pw, ph = 40, 270, 420, 130
box(px, py, pw, ph, "#eefafa", "#008080")
txt(px+16, py+26, "Sensor de Movimiento — PIR (HC-SR501 / AM312)", 14, "bold", "#005959")
txt(px+16, py+46, "• Salida digital 3.3V con ventana de retención de 15 s", 11.5, "normal", "#333")
txt(px+pw-14, py_l["D35"]+4, "Señal OUT → GPIO 35", 11.5, "bold", "#005959", "end")
txt(px+pw-14, py+112, "Alimentación VCC ← 5V (VIN)", 11.5, "bold", C["5v"], "end")
wire([(px+pw, py_l["D35"]), (bx+16, py_l["D35"])], C["pir"])

# 3. LED RGB Indicador de Modo (GPIO 25, 26, 27)
rx, ry, rw, rh = 40, 420, 420, 270
box(rx, ry, rw, rh, "#fff8f0", "#c05621")
txt(rx+16, ry+26, "LED RGB — Indicador de Modo (Cátodo Común)", 14, "bold", "#9c4221")
txt(rx+16, ry+46, "3× Resistores de 220 Ω en serie · Cátodo (-) a GND", 11.5, "normal", "#444")

rgb_rows = [
    ("D25", "rgb_r", 500, "R (220 Ω) ← GPIO 25 (Rojo: Clima / Amarillo: PIR)"),
    ("D26", "rgb_g", 545, "G (220 Ω) ← GPIO 26 (Verde: LDR / Amarillo: PIR)"),
    ("D27", "rgb_b", 590, "B (220 Ω) ← GPIO 27 (Azul: Modo Manual)"),
]
for idx, (pin_name, col_key, box_y, label_txt) in enumerate(rgb_rows):
    pin_y = py_l[pin_name]
    mid_x = 485 + idx * 18
    wire([(rx+rw, box_y), (mid_x, box_y), (mid_x, pin_y), (bx+16, pin_y)], C[col_key])
    txt(rx+18, box_y+4, label_txt, 11.5, "bold", C[col_key])

# Alimentacion 5V (VIN) y GND del lado izquierdo
wire([(bx+16, py_l["GND"]), (540, py_l["GND"]), (540, 660), (rx+rw, 660)], C["gnd"])
txt(rx+rw-14, 664, "GND Común (LDR, PIR, Cátodo RGB)", 11.5, "bold", "#222", "end")
wire([(bx+16, py_l["VIN"]), (558, py_l["VIN"]), (558, py+108), (px+pw, py+108)], C["5v"])

# ==============================================================================
# LADO DERECHO: Modulo Rele (GPIO 23), SHT41 (I2C 0x44) y OLED SSD1306 (I2C 0x3C)
# ==============================================================================

# 1. Modulo Rele (GPIO 23)
rlx, rly, rlw, rlh = 1020, 105, 420, 155
box(rlx, rly, rlw, rlh, "#eefbf3", "#26a269")
txt(rlx+18, rly+26, "Módulo Relé de 1 Canal (Actuador de Potencia)", 14, "bold", "#1b7a4e")
txt(rlx+18, rly+48, "• Alimentación VCC: 5V (VIN) · Tierra: GND", 11.5, "normal", "#333")
txt(rlx+18, rly+68, "• Contactos de salida: COM / NO (Normalmente Abierto)", 11.5, "normal", "#333")
txt(rlx+18, rly+126, "IN ← GPIO 23 (Control con confirmación)", 12, "bold", "#1b7a4e")
wire([(bx+bw-16, py_r["D23"]), (980, py_r["D23"]), (980, rly+122), (rlx, rly+122)], C["rel"])

# Buses I2C + 3V3 + GND a la derecha (identicos a la practica anterior)
X_BUS = {"scl": 915, "sda": 940, "gnd": 965, "3v3": 990}
wire([(bx+bw-16, py_r["D22"]), (X_BUS["scl"], py_r["D22"]), (X_BUS["scl"], 635)], C["scl"])
wire([(bx+bw-16, py_r["D21"]), (X_BUS["sda"], py_r["D21"]), (X_BUS["sda"], 665)], C["sda"])
wire([(bx+bw-16, py_r["GND"]), (X_BUS["gnd"], py_r["GND"]), (X_BUS["gnd"], 380)], C["gnd"])
wire([(bx+bw-16, py_r["3V3"]), (X_BUS["3v3"], py_r["3V3"]), (X_BUS["3v3"], 350)], C["3v3"])

# 2. Sensor GY-SHT41 (I2C 0x44 - Mismos pines de la practica anterior)
sx, sy, sw, sh = 1020, 285, 420, 185
box(sx, sy, sw, sh, "#7b2fbe", "#4b1a75")
txt(sx+18, sy+26, "Sensor Ambiental GY-SHT41 (I²C 0x44)", 14, "bold", "#fff")
txt(sx+18, sy+46, "Temperatura y Humedad · Mismos pines Práctica 1", 11.5, "normal", "#efe6f7")
for name, yy, k in [("VIN (3.3V)", 350, "3v3"), ("GND", 380, "gnd"), ("SCL (GPIO 22)", 410, "scl"), ("SDA (GPIO 21)", 440, "sda")]:
    wire([(X_BUS[k], yy), (sx, yy)], C[k])
    dot(X_BUS[k], yy, C[k])
    txt(sx+16, yy+4, name, 11.5, "bold", "#fff")

# 3. Pantalla OLED SSD1306 (I2C 0x3C - Mismos pines de la practica anterior)
ox, oy, ow, oh = 1020, 495, 420, 200
box(ox, oy, ow, oh, "#0b3d91", "#06265c")
txt(ox+18, oy+26, "Pantalla OLED SSD1306 128×32 (I²C 0x3C)", 14, "bold", "#fff")
txt(ox+18, oy+46, "Muestra Modo, Relé y Sensores · Mismos pines P1", 11.5, "normal", "#cfe0ff")
box(ox+210, oy+68, 190, 74, "#000", "#444", rx=4)
txt(ox+220, oy+88, "M:LDR       REL:ON", 12, "bold", "#9fe7ff", font=M)
txt(ox+220, oy+126, "25.4 C", 24, "bold", "#9fe7ff", font=M)
for name, yy, k in [("VCC (3.3V)", 575, "3v3"), ("GND", 605, "gnd"), ("SCL (GPIO 22)", 635, "scl"), ("SDA (GPIO 21)", 665, "sda")]:
    wire([(X_BUS[k], yy), (ox, yy)], C[k])
    dot(X_BUS[k], yy, C[k])
    txt(ox+16, yy+4, name, 11.5, "bold", "#fff")

# ==============================================================================
# TABLA RESUMEN DE PINES (PARTE INFERIOR)
# ==============================================================================
rows = [
    ("3v3",   "Alimentación 3.3V",          "3V3",            "VIN del SHT41 · VCC de la OLED · Extremo superior del LDR"),
    ("5v",    "Alimentación 5V (USB)",      "VIN (5V)",       "VCC del Sensor PIR · VCC del Módulo Relé"),
    ("gnd",   "Tierra Común (GND)",         "GND",            "GND de SHT41, OLED, PIR, Relé, R 10kΩ (LDR) y Cátodo LED RGB"),
    ("sda",   "Datos I²C (Idéntico P1)",    "GPIO 21 (D21)",  "SDA del Sensor GY-SHT41 (0x44) y SDA de la OLED (0x3C)"),
    ("scl",   "Reloj I²C (Idéntico P1)",    "GPIO 22 (D22)",  "SCL del Sensor GY-SHT41 (0x44) y SCL de la OLED (0x3C)"),
    ("ldr",   "Entrada Analógica LDR",      "GPIO 34 (D34)",  "Nodo divisor de tensión (LDR + R 10kΩ) en canal ADC1_CH6"),
    ("pir",   "Entrada Digital PIR",        "GPIO 35 (D35)",  "Salida OUT del Sensor de Movimiento PIR"),
    ("rgb_r", "LED RGB — Canal Rojo",       "GPIO 25 (D25)",  "Resistor 220 Ω → Terminal R (Modo Auto Clima / Auto PIR)"),
    ("rgb_g", "LED RGB — Canal Verde",      "GPIO 26 (D26)",  "Resistor 220 Ω → Terminal G (Modo Auto LDR / Auto PIR)"),
    ("rgb_b", "LED RGB — Canal Azul",       "GPIO 27 (D27)",  "Resistor 220 Ω → Terminal B (Modo Manual)"),
    ("rel",   "Salida Digital Módulo Relé", "GPIO 23 (D23)",  "Entrada IN del Módulo Relé (control automático o manual)"),
]

tx, ty, rh = 40, 790, 33
cols = [tx, tx+65, tx+310, tx+500]
a(f'<rect x="{tx}" y="{ty}" width="1400" height="{rh}" fill="#2c3e50"/>')
for x, h_txt in zip(cols, ["Color", "Señal / Función", "Pin ESP32", "Conexión Física en Protoboard"]):
    txt(x+12, ty+22, h_txt, 13, "bold", "#fff")

for i, (k, sig, pin, dest) in enumerate(rows):
    y = ty + rh*(i+1)
    bg = "#f4f6f8" if i % 2 == 0 else "#ffffff"
    a(f'<rect x="{tx}" y="{y}" width="1400" height="{rh}" fill="{bg}" stroke="#e0e0e0"/>')
    a(f'<line x1="{tx+14}" y1="{y+16}" x2="{tx+50}" y2="{y+16}" stroke="{C[k]}" stroke-width="5" stroke-linecap="round"/>')
    txt(cols[1]+12, y+21, sig, 12.5, "normal", "#222")
    txt(cols[2]+12, y+21, pin, 12.5, "bold", "#222", font=M)
    txt(cols[3]+12, y+21, dest, 12.5, "normal", "#222")

a('</svg>')
open(sys.argv[1], "w").write("\n".join(o))

