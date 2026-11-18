# Processing

La aplicación muestra las mediciones de Arduino, permite configurar alarmas, registra datos y ofrece herramientas de mantenimiento.

## Ejecutar

1. Abrir `PanelDHT22/PanelDHT22.pde` en Processing 4.
2. Conservar `PanelDHT22/code/Info2Ambiente.jar` en la carpeta `code` (Java 17).
3. Cerrar el Monitor Serie de Arduino IDE.
4. Abrir la sección CONEXION, elegir el puerto COM y presionar CONECTAR.

## Secciones

PANEL: valores actuales y estado del ambiente.  
GRAFICOS: evolución de temperatura y humedad.  
ALARMAS: límites y reconocimiento.  
CONTROL IR: asociación de teclas del CAR MP3.  
HISTORIAL: mediciones conservadas en CSV.  
CONEXION: estado del enlace y control del menú LCD.  
MANTENIMIENTO: prueba visual del RGB y lectura del pulsador.

En MANTENIMIENTO se pueden verificar individualmente los tres canales del RGB, ejecutar la secuencia de cuatro segundos, localizar la unidad o finalizar la prueba. La información HIGH/LOW representa niveles lógicos, no una medición de corriente ni una confirmación óptica del LED.
