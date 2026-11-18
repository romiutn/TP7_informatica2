#include "ReceptorIR.h"
#include <EEPROM.h>
#include <IRremote.hpp>
#include <string.h>

static const uint16_t IR_MAGIC = 0x1838;
ReceptorIR::ReceptorIR() : esperando(0), ultimaAprendida(0) {
  memset(teclas, 0, sizeof(teclas));
}

void ReceptorIR::comenzar(uint8_t pin) {
  cargar();
  IrReceiver.begin(pin, DISABLE_LED_FEEDBACK);
}

bool ReceptorIR::aprender(uint8_t accion) {
  if (accion < 1 || accion > NUM_TECLAS) return false;
  esperando = accion;
  return true;
}

void ReceptorIR::borrar() {
  memset(teclas, 0, sizeof(teclas));
  esperando = 0;
  guardar();
}
void ReceptorIR::cancelarAprendizaje() { esperando=0; }
bool ReceptorIR::aprendiendo() const { return esperando != 0; }
uint8_t ReceptorIR::ultimaTeclaAprendida() const { return ultimaAprendida; }
uint8_t ReceptorIR::cantidadConfigurada() const {
  uint8_t n = 0;
  for (uint8_t i = 0; i < NUM_TECLAS; ++i) if (teclas[i].protocolo) ++n;
  return n;
}

ReceptorIR::Accion ReceptorIR::procesar() {
  ultimaAprendida = 0;
  if (!IrReceiver.decode()) return NINGUNA;
  IRData data = IrReceiver.decodedIRData;
  IrReceiver.resume();
  if (data.protocol == UNKNOWN || (data.flags & IRDATA_FLAGS_IS_REPEAT)) return NINGUNA;
  TeclaIR codigo = { (uint16_t)data.protocol, (uint16_t)data.address, (uint16_t)data.command };
  if (esperando) {
    teclas[esperando - 1] = codigo;
    ultimaAprendida = esperando;
    esperando = 0;
    guardar();
    return NINGUNA;
  }
  for (uint8_t i = 0; i < NUM_TECLAS; ++i) {
    if (teclas[i].protocolo == codigo.protocolo &&
        teclas[i].direccion == codigo.direccion &&
        teclas[i].comando == codigo.comando) return (Accion)(i+1);
  }
  return NINGUNA;
}

void ReceptorIR::cargar() {
  uint16_t firma = 0;
  EEPROM.get(0, firma);
  if (firma == IR_MAGIC) EEPROM.get(sizeof(firma), teclas);
  else memset(teclas, 0, sizeof(teclas));
}

void ReceptorIR::guardar() {
  EEPROM.put(0, IR_MAGIC);
  EEPROM.put(sizeof(IR_MAGIC), teclas);
}
