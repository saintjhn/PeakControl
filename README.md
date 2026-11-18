# PeakControl

Clipper + limiter con oversampling y lookahead, en formato **VST3** para Windows (x64).
Hecho con JUCE y C++.

**Estado:** prototipo v0.2 para pruebas tecnicas. Todavia no tiene interfaz propia: usa la interfaz generica de JUCE.

## Cadena de senal

```
Entrada -> Input Gain -> [ Oversampling (2x/4x/8x) -> Clipper (tanh o hard) -> vuelta a la frecuencia original ]
        -> Limiter con lookahead (opcional) -> Mix con la senal seca alineada -> Output Gain -> Salida
```

## Parametros

| Parametro | Rango | Por defecto | Notas |
|---|---|---|---|
| Input Gain | 0 a +24 dB | 0 dB | Se aplica antes del clipper (suavizado 20 ms) |
| Ceiling | -12 a 0 dB | -0.3 dB | Techo del clipper y del limiter (suavizado 20 ms) |
| Clip Mode | Soft (tanh) / Hard | Soft | Curva de recorte |
| Oversampling | Off / 2x / 4x / 8x | 2x | Reduce el aliasing; anade latencia y CPU |
| Limiter | On / Off | On | Limiter con lookahead tras el clipper |
| Lookahead | 0.5 a 5 ms | 1.5 ms | Anade latencia cuando el limiter esta activo |
| Release | 5 a 500 ms | 80 ms | Recuperacion de la ganancia del limiter |
| Output Gain | -24 a 0 dB | 0 dB | Solo baja el nivel, asi el Ceiling sigue siendo el techo |
| Mix | 0 a 100 % | 100 % | La senal seca es la entrada original, sin Input Gain, alineada en latencia |
| Bypass | On / Off | Off | Fundido de 20 ms a la senal original, manteniendo la latencia (sin desfase) |

## Latencia

El plugin informa al host de su latencia (oversampling + lookahead del limiter). El aviso se hace desde el hilo principal. Cambiar Oversampling, Limiter o Lookahead durante la reproduccion cambia la latencia y puede producir un clic.

## Cambios en v0.2

- Input Gain y Ceiling suavizados (sin zipper noise al automatizar).
- Parametro Bypass que mantiene la latencia.
- El estado interno (limiter, senal seca, oversampling) se reinicia al arrancar la reproduccion.
- La latencia se comunica al host desde el hilo principal.
- Proteccion contra NaN e infinitos en la entrada.

## Limitaciones conocidas

- El limiter detecta picos de muestra (sample peak), **no true peak**. Con Ceiling en 0 dB pueden aparecer inter-sample peaks por encima. Para entregas, usar -1 dB o menos.
- Con el Limiter desactivado y el Oversampling activo, el Ceiling no esta garantizado: el filtro de bajada puede dejar pasar unas decimas de dB.
- Con Mix por debajo del 100 % la senal seca se suma tal cual y puede superar el Ceiling.
- Clic al cambiar Oversampling, Lookahead o Limiter mientras suena.
- Solo VST3, solo Windows x64, solo mono y estereo, solo procesado en 32 bits float.
- El calculo del minimo de la ventana del limiter es directo: el coste de CPU crece con el lookahead y la frecuencia de muestreo.

## Compilar

Requisitos: JUCE (indicar aqui la version usada), Projucer y Visual Studio. El proyecto necesita el modulo `juce_dsp`.

1. Abrir el archivo `.jucer` con Projucer.
2. Comprobar las rutas en **File > Global Paths** (Path to JUCE).
3. Pulsar **Save Project and Open in IDE**.
4. En Visual Studio, compilar en **Release x64**.
5. El resultado queda en `Builds\VisualStudio2026\x64\Release\VST3\`.

Las carpetas `Builds/` y `JuceLibraryCode/` no estan en el repositorio: Projucer las regenera al abrir el `.jucer`.

## Instalar

1. Copiar la carpeta **entera** del plugin (`.vst3`) a `C:\Program Files\Common Files\VST3` (hacen falta permisos de administrador).
2. En FL Studio: **Options > Manage plugins > Find plugins**.
3. Si habia una version anterior instalada, sustituirla por la nueva.

## Licencia

Pendiente de definir. JUCE tiene doble licencia (AGPLv3 o licencias comerciales); la licencia de este proyecto debe ser compatible con la que se use de JUCE. Ver https://juce.com
