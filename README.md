# 01-Mixer

Mixer de audio con 3 canales, basado en el ejemplo `01-AudioFilePlayer` del curso (módulo 2).

## Objetivo

- 3 canales, cada uno carga un archivo de audio (como el reproductor del ejemplo base)
- Controles de mezcla por canal: ganancia y paneo (basado en el ejemplo `03-GainPan`)
- Efecto de delay en la cadena (basado en el ejemplo `05-DelayApp`)
## Etapa de separación en stems (`stems/`)

`stems/stem_separation.ipynb` es un notebook interactivo que separa un tema en stems con **BS-RoFormer**: elegís el audio con un popup, instala `bs-roformer-infer` solo si falta, y exporta los WAV con nombres normalizados (`vocals.wav`, `drums.wav`, `bass.wav`, `guitar.wav`, `piano.wav`, `other.wav`, `instrumental.wav`) **en la raíz del proyecto**, que es de donde la app los levanta.

**Requerimientos mínimos de hardware**: 8 GB de RAM (16 recomendados); CPU x86_64 moderno (en CPU tarda varios minutos por tema); GPU opcional; ~3 GB de disco libre; Python ≥ 3.10. La primera corrida descarga el modelo (~700 MB).

Uso: abrir el notebook en Jupyter, ejecutar las celdas de arriba hacia abajo (`Shift+Enter`).

## Estructura

```
01-Mixer/
├── CMakeLists.txt    # fuente de verdad del proyecto (flujo CMake, sin Projucer)
├── Source/           # código de la app
├── stems/            # notebook de separación en stems; sus salidas van a la raíz
└── build/            # build dir (no commitear)
```

## Compilar y correr

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug   # una sola vez por proyecto
cmake --build build
./build/01-Mixer_artefacts/Debug/01-Mixer
```

## Notas

- El flujo de build completo está definido en `../README.md` (raíz de pas1dir).
- `AudioFilePlayer.jucer` es un archivo heredado del ejemplo original; no se usa.
