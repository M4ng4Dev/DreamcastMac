# DreamcastMac — Estado actual y próximos pasos (Handoff)

## 📍 Dónde estamos
El proyecto tiene la arquitectura base del **Recompilador Estático (`dc-recomp`)** y el **Runtime Metal 3 (`dc-runtime`)** compilando y pasando las pruebas de ejecución nativa de SH-4:

- **Runtime (`runtime/`)**:
  - `sh4_context.h`: Estado completo de la CPU SH-4 (R0–R15, bancos, SR, FPSCR, FPU, XF0–XF15).
  - `sh4_instructions.h`: Operaciones SH-4 con aceleración **ARM NEON** para `FIPR` y `FTRV` en Apple Silicon.
  - `memory.h` / `memory.cpp`: Mapa de memoria de Dreamcast (16 MB Main RAM, 8 MB VRAM, 2 MB AICA RAM, stubs MMIO).
  - `powervr_metal.h` / `powervr_metal.cpp` / `metal/powervr.metal`: Interfaz y shaders Metal 3 TBDR (`raster_order_group(0)` para transparencia independiente del orden).
- **Recompilador (`tools/recompiler/`)**:
  - `sh4_opcodes.h` / `sh4_disasm.h` / `sh4_disasm.cpp`: Decodificador SH-4 y analizador de bloques básicos/funciones.
  - `ir_emitter.h` / `ir_emitter.cpp`: Generador de código C++ con manejo de *delay slots*.
  - `dc_disc.h` / `dc_disc.cpp`: Extractor directo de `1ST_READ.BIN` desde `.gdi`, `.iso` o `.cdi` + des-scrambler automático de Dreamcast.
  - `main.cpp`: CLI `dc-recomp` con informe de cobertura de opcodes y lista de opcodes desconocidos más frecuentes.
- **Verificación (`samples/sh4_test/`)**:
  - `generate_sample`, `verify_execution` (probado al 100% con subrutina `bsr`, *delay slot* y aritmética).

---

## 🎯 Próximo paso inmediato (en el Mac del usuario)
Nuestro **juego piloto** elegido es **Sonic Adventure**.

1. **Actualizar y compilar en `Documents/DreamcastMac`**:
   ```bash
   git pull
   mkdir -p build && cd build
   cmake -G Ninja ..
   ninja
   ./verify_execution
   ```

2. **Extraer y analizar `Sonic Adventure`**:
   Ejecutar `dc-recomp` apuntando a la imagen del disco (`.gdi`, `.cdi` o `.iso`) o directamente a `1ST_READ.BIN`:
   ```bash
   ./dc-recomp -i "/ruta/a/SonicAdventure.gdi" -o sonic_recomp.cpp --dump-bin sonic_1st_read.bin
   ```

3. **Siguientes hitos a implementar a partir de ese análisis**:
   - **Completar el set de instrucciones SH-4** en `sh4_opcodes.h`, `sh4_disasm.cpp`, `ir_emitter.cpp` y `sh4_instructions.h` basándonos en el reporte de *Top unknown opcodes* de `sonic_1st_read.bin` (por ejemplo: `mov.w`, `mov.b`, `mov.l @(disp,PC), Rn` para *literal pools*, `sts.l pr,@-r15`, `lds.l @r15+,pr`, `shll`/`shlr`, `fmov.s`, `mac.l`, etc.).
   - **Descubrimiento de funciones en tablas de punteros / literal pools**: en binarios reales compilados por GCC/Hitachi SHC, muchas funciones se llaman vía `mov.l @(disp,PC), Rn` + `jsr @Rn`. Hay que detectar esas cargas de direcciones en `0x8Cxxxxxx` para descubrir todo el grafo de llamadas de *Sonic Adventure*.
   - **Ventana macOS + MTKView + PowerVR Tile Accelerator**: conectar `PowerVRMetalRenderer` a una app Cocoa/SwiftUI en macOS para renderizar los primeros polígonos y menús reales con Metal 3 y MetalFX.
