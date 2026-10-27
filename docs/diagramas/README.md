# Generadores de Diagramas (SVG y PNG)

Scripts en Python puro (sin dependencias externas) para generar los diagramas del proyecto.

| Script | Salida SVG | Salida PNG |
|---|---|---|
| `diagrama_conexiones.py` | `hardware/diagrama_conexiones.svg` | `hardware/diagrama_conexiones.png` |
| `diagrama_arquitectura.py` | `docs/diagrama_arquitectura.svg` | `docs/diagrama_arquitectura.png` |

## Regenerar Diagramas

Desde la raíz del repositorio:

```bash
python3 docs/diagramas/diagrama_conexiones.py hardware/diagrama_conexiones.svg
python3 docs/diagramas/diagrama_arquitectura.py docs/diagrama_arquitectura.svg

rsvg-convert -z 2 hardware/diagrama_conexiones.svg -o hardware/diagrama_conexiones.png
rsvg-convert -z 2 docs/diagrama_arquitectura.svg -o docs/diagrama_arquitectura.png
```

