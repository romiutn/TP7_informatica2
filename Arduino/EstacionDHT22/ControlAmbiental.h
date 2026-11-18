#ifndef CONTROL_AMBIENTAL_H
#define CONTROL_AMBIENTAL_H
#include <Arduino.h>

enum EstadoAmbiental { INICIANDO, NORMAL, ADVERTENCIA, ALERTA, ERROR_SENSOR, PAUSADO };
struct MedicionAmbiental {
  float temperatura;
  float humedad;
  bool valida;
};

class ControlAmbiental {
 public:
  ControlAmbiental();
  void actualizar(const MedicionAmbiental &medicion);
  void pausar(bool valor);
  bool estaPausado() const;
  bool fijarLimite(char tipo, int valor);
  int limiteT() const;
  int limiteH() const;
  EstadoAmbiental estado() const;
  const char *nombreEstado() const;
  void reconocerAlarma();
  bool alarmaReconocida() const;
 private:
  int _limT, _limH;
  bool _pausa, _reconocida;
  EstadoAmbiental _estado;
};
#endif
