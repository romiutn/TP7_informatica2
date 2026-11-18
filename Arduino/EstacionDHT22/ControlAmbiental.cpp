#include "ControlAmbiental.h"

ControlAmbiental::ControlAmbiental() :
    _limT(30), _limH(75), _pausa(false), _reconocida(false), _estado(INICIANDO) {}

void ControlAmbiental::actualizar(const MedicionAmbiental &m) {
  if (_pausa) { _estado = PAUSADO; return; }
  if (!m.valida) { _estado = ERROR_SENSOR; _reconocida = false; return; }
  const bool supera = m.temperatura > _limT || m.humedad > _limH;
  const bool mantenerAlarma = _estado == ALERTA &&
      (m.temperatura > _limT - 0.5f || m.humedad > _limH - 2.0f);
  if (supera || mantenerAlarma) { _estado = ALERTA; return; }
  _reconocida = false;
  const bool cerca = m.temperatura >= _limT - 2.0f || m.humedad >= _limH - 10.0f;
  _estado = cerca ? ADVERTENCIA : NORMAL;
}

void ControlAmbiental::pausar(bool valor) {
  _pausa = valor;
  _estado = valor ? PAUSADO : INICIANDO;
  if (valor) _reconocida = false;
}
bool ControlAmbiental::estaPausado() const { return _pausa; }

bool ControlAmbiental::fijarLimite(char tipo, int valor) {
  if (tipo == 'T' && valor >= 0 && valor <= 60) { _limT = valor; return true; }
  if (tipo == 'H' && valor >= 0 && valor <= 100) { _limH = valor; return true; }
  return false;
}
int ControlAmbiental::limiteT() const { return _limT; }
int ControlAmbiental::limiteH() const { return _limH; }
EstadoAmbiental ControlAmbiental::estado() const { return _estado; }
void ControlAmbiental::reconocerAlarma() {
  if (_estado == ALERTA) _reconocida = true;
}
bool ControlAmbiental::alarmaReconocida() const { return _reconocida; }

const char* ControlAmbiental::nombreEstado() const {
  switch (_estado) {
    case INICIANDO: return "INICIANDO";
    case NORMAL: return "NORMAL";
    case ADVERTENCIA: return "ADVERTENCIA";
    case ALERTA: return "ALERTA";
    case ERROR_SENSOR: return "ERROR";
    case PAUSADO: return "PAUSADO";
  }
  return "ERROR";
}
