#ifndef RECEPTOR_IR_H
#define RECEPTOR_IR_H
#include <Arduino.h>

struct TeclaIR { uint16_t protocolo; uint16_t direccion; uint16_t comando; };
class ReceptorIR {
 public:
  enum Accion { NINGUNA=0, PANTALLA=1, PAUSA=2, REANUDAR=3,
                RECONOCER=4, TEMP_MAS=5, TEMP_MENOS=6,
                HUM_MAS=7, HUM_MENOS=8 };
  ReceptorIR();
  void comenzar(uint8_t pin);
  bool aprender(uint8_t accion);
  void borrar();
  void cancelarAprendizaje();
  Accion procesar();
  bool aprendiendo() const;
  uint8_t cantidadConfigurada() const;
  uint8_t ultimaTeclaAprendida() const;
 private:
  static const uint8_t NUM_TECLAS = 8;
  TeclaIR teclas[NUM_TECLAS];
  uint8_t esperando, ultimaAprendida;
  void cargar();
  void guardar();
};
#endif
