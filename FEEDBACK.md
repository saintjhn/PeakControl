# PeakControl v0.1: guía de pruebas y feedback

Gracias por probarlo. Es un prototipo: la idea es que rompáis cosas y anotéis lo que oigáis y midáis.
**Bajad el volumen de monitoreo al empezar**: es un clipper/limiter y con Input Gain alto puede sonar fuerte.

## 1. Datos del tester

- Nombre:
- DAW y versión:
- Sistema operativo:
- Interfaz de audio, frecuencia de muestreo y tamaño de buffer:

## 2. Pruebas técnicas (marcad y anotad resultados)

- [ ] **Carga y estado:** cargar el plugin, mover parámetros, guardar el proyecto, cerrarlo y reabrirlo. ¿Se conservan los valores?
- [ ] **Techo con limiter:** Limiter On, Ceiling -1 dB, Input Gain +12 dB y material denso. Anotar pico de muestra y **true peak** medidos a la salida.
  - Resultado:
- [ ] **Techo sin limiter:** igual, con Limiter Off y Oversampling en 2x/4x/8x. Anotar el exceso sobre el Ceiling.
  - Resultado:
- [ ] **Latencia y alineación:** duplicar la pista, invertir la polaridad de una y dejar la otra con el plugin (Hard, Input Gain 0, Ceiling 0 dB, material por debajo de -6 dBFS). Con Oversampling Off y Limiter Off el residuo debería ser nulo. Repetir con 2x/8x y con Limiter On. Anotar el residuo.
  - Resultado:
- [ ] **Aliasing:** seno agudo (8 a 16 kHz) o barrido, Hard, Input Gain +18 dB. Comparar en un analizador de espectro Off / 2x / 4x / 8x.
  - Resultado:
- [ ] **Mix paralelo:** Mix al 50 % con la señal sin tocar el techo: ¿suena igual que al 100 %? (comprueba la alineación de la seca).
- [ ] **Automatización:** automatizar Input Gain, Ceiling, Mix y Output Gain. ¿Hay clics o zipper noise?
- [ ] **Cambios en vivo:** cambiar Oversampling, Lookahead y Limiter mientras suena. ¿Clic? ¿El host recompensa bien la latencia?
- [ ] **Render:** el render offline, ¿coincide con lo que se oye en tiempo real?
- [ ] **Frecuencias y buffers:** probar 44.1 / 48 / 96 kHz y distintos tamaños de buffer. ¿Algún fallo?
- [ ] **Pista mono:** probar el plugin en una pista mono.
- [ ] **CPU:** anotar el consumo con 2x, 4x y 8x y con Lookahead al máximo.
  - Resultado:

## 3. Escucha (valoración subjetiva)

- ¿Soft (tanh) vs Hard: cuándo preferís cada uno y por qué?
- ¿El limiter es transparente? ¿Bombeo, distorsión en graves, aplastamiento? ¿Con qué Release y Lookahead?
- ¿Los rangos de los parámetros tienen sentido? ¿Qué cambiaríais?
- ¿Qué os falta para usarlo en una sesión real? (true peak, medidores, filtro en la detección, otras curvas de clip, modo mid/side...)

## 4. Registro de problemas

| Qué pasó | Cómo reproducirlo | Parámetros | DAW |
|---|---|---|---|
| | | | |
