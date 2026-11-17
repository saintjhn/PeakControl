# PeakControl

Clipper + limiter con oversampling y lookahead, en formato **VST3** para Windows (x64).
Hecho con JUCE y C++.

**Estado:** prototipo v0.1 para pruebas técnicas. Todavía no tiene interfaz propia: usa la interfaz genérica de JUCE.

## Cadena de señal

```
Entrada -> Input Gain -> [ Oversampling (2x/4x/8x) -> Clipper (tanh o hard) -> vuelta a la frecuencia original ]
        -> Limiter con lookahead (opcional) -> Mix con la señal seca alineada -> Output Gain -> Salida
```

## Parámetros

| Parámetro | Rango | Por defecto | Notas |
|---|---|---|---|
| Input Gain | 0 a +24 dB | 0 dB | Se aplica antes del clipper |
| Ceiling | -12 a 0 dB | -0.3 dB | Techo del clipper y del limiter |
| Clip Mode | Soft (tanh) / Hard | Soft | Curva de recorte |
| Oversampling | Off / 2x / 4x / 8x | 2x | Reduce el aliasing; añade latencia y CPU |
| Limiter | On / Off | On | Limiter con lookahead tras el clipper |
| Lookahead | 0.5 a 5 ms | 1.5 ms | Añade latencia cuando el limiter está activo |
| Release | 5 a 500 ms | 80 ms | Recuperación de la ganancia del limiter |
| Output Gain | -24 a 0 dB | 0 dB | Solo baja el nivel, así el Ceiling sigue siendo el techo |
| Mix | 0 a 100 % | 100 % | La señal seca es la entrada original, sin Input Gain, alineada en latencia |

## Latencia

El plugin informa al host de su latencia (oversampling + lookahead del limiter). Cambiar Oversampling, Limiter o Lookahead durante la reproducción cambia la latencia y puede producir un clic.

## Limitaciones conocidas (v0.1)

- El limiter detecta picos de muestra (sample peak), **no true peak**. Con Ceiling en 0 dB pueden aparecer inter-sample peaks por encima. Para entregas, usar -1 dB o menos.
- Con el Limiter desactivado y el Oversampling activo, el Ceiling no está garantizado: el filtro de bajada puede dejar pasar unas décimas de dB.
- Con Mix por debajo del 100 % la señal seca se suma tal cual y puede superar el Ceiling.
- Input Gain y Ceiling no están suavizados: posible ruido de "zipper" al automatizarlos.
- Clic al cambiar Oversampling, Lookahead o Limiter mientras suena.
- No hay bypass propio, y el estado interno no se reinicia al parar el transporte.
- Solo VST3, solo Windows x64, solo mono y estéreo, solo procesado en 32 bits float.
- El cálculo del mínimo de la ventana del limiter es directo: el coste de CPU crece con el lookahead y la frecuencia de muestreo.

## Compilar

Requisitos: JUCE (indicar aquí la versión usada), Projucer y Visual Studio. El proyecto necesita el módulo `juce_dsp`.

1. Abrir el archivo `.jucer` con Projucer.
2. Comprobar las rutas en **File > Global Paths** (Path to JUCE).
3. Pulsar **Save Project and Open in IDE**.
4. En Visual Studio, compilar en **Release x64**.
5. El resultado queda en `Builds\VisualStudio2026\x64\Release\VST3\`.

Las carpetas `Builds/` y `JuceLibraryCode/` no están en el repositorio: Projucer las regenera al abrir el `.jucer`.

## Instalar

1. Copiar la carpeta **entera** `PeakControl_VST3.vst3` a `C:\Program Files\Common Files\VST3` (hacen falta permisos de administrador).
2. En FL Studio: **Options > Manage plugins > Find plugins**.

## Licencia

Pendiente de definir. JUCE tiene doble licencia (AGPLv3 o licencias comerciales); la licencia de este proyecto debe ser compatible con la que se use de JUCE. Ver https://juce.com
