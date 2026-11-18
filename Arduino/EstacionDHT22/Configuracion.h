#ifndef CONFIGURACION_AMBIENT_H
#define CONFIGURACION_AMBIENT_H
#include <Arduino.h>

class Configuracion {
public:
  static void cargar(uint8_t &temperatura, uint8_t &humedad);
  static void guardar(uint8_t temperatura, uint8_t humedad);
};
#endif
