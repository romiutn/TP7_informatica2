# Arduino UNO

El programa lee el DHT22, muestra las mediciones en la LCD, evalúa los estados ambientales y recibe órdenes por USB o infrarrojo.

## Cargar el programa

Abrir `EstacionDHT22/EstacionDHT22.ino` desde Arduino IDE, seleccionar Arduino Uno y el puerto COM. Mantener los archivos `.h` y `.cpp` junto al `.ino`.

Bibliotecas: DHT sensor library, Adafruit Unified Sensor, LiquidCrystal_I2C e IRremote 4.x. El puerto serie trabaja a 9600 baudios.

## Conexiones

| Componente | Pin |
|---|---|
| DHT22 (DATA) | D2 |
| LED verde, amarillo, rojo | D4, D5, D6 |
| Pulsador | D7 (con INPUT_PULLUP) |
| Receptor infrarrojo (OUT) | D8 |
| RGB rojo, verde, azul | D9, D10, D11 |
| LCD SDA, SCL | A4, A5 |

El DHT22 se alimenta a 3,3 V y la LCD y el receptor IR a 5 V en este montaje. Todos comparten GND. Cada LED y cada canal del RGB lleva su resistencia limitadora.

## Verificación del indicador

Processing envía órdenes `RGB,R`, `RGB,G`, `RGB,B`, `RGB,0`, `RGB,T`, `RGB,V` y `RGB,A`. Arduino informa por serie `IO,D7,R,G,B,modo`, con los estados lógicos de entrada y de las salidas configuradas. El pulsador puede recorrer los canales durante una prueba manual. `RGB,T` realiza una secuencia visual de cuatro segundos; `RGB,V` identifica el equipo con una señal violeta durante siete segundos. `RGB,A` devuelve el RGB a la función automática. Los LED de alarma ambiental no cambian con estas órdenes.
