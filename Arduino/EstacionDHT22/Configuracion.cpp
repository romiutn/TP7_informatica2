#include "Configuracion.h"
#include <EEPROM.h>

static const int SLOT_A = 80, SLOT_B = 96;
static const uint16_t FIRMA = 0xA617;
static const uint8_t VERSION = 1;
struct DatosConfig {
  uint16_t firma;
  uint8_t version;
  uint8_t t;
  uint8_t h;
  uint8_t secuencia;
  uint8_t comprobacion;
} __attribute__((packed));

static uint8_t checksum(const DatosConfig &d) {
  return (uint8_t)(0xA5u ^ d.version ^ d.t ^ d.h ^ d.secuencia);
}
static bool valido(const DatosConfig &d) {
  return d.firma==FIRMA && d.version==VERSION && d.comprobacion==checksum(d)
      && d.t<=60 && d.h<=100;
}
static int vigente(DatosConfig &act) {
  DatosConfig a,b;
  EEPROM.get(SLOT_A,a);EEPROM.get(SLOT_B,b);
  bool av=valido(a),bv=valido(b);
  if(av && bv) {
    if((int8_t)(a.secuencia-b.secuencia)>0) {act=a;return SLOT_A;}
    act=b;return SLOT_B;
  }
  if(av){act=a;return SLOT_A;}
  if(bv){act=b;return SLOT_B;}
  return -1;
}

void Configuracion::cargar(uint8_t &temperatura,uint8_t &humedad) {
  DatosConfig actual;
  if(vigente(actual)>=0) {temperatura=actual.t;humedad=actual.h;}
  else {temperatura=30;humedad=75;}
}

void Configuracion::guardar(uint8_t temperatura,uint8_t humedad) {
  DatosConfig anterior;
  int slotPrevio=vigente(anterior);
  if(slotPrevio>=0 && anterior.t==temperatura && anterior.h==humedad)return;
  DatosConfig nuevo={FIRMA,VERSION,temperatura,humedad,
                     (uint8_t)(slotPrevio>=0?(uint8_t)(anterior.secuencia+1):0),0};
  nuevo.comprobacion=checksum(nuevo);
  int destino=(slotPrevio==SLOT_A)?SLOT_B:SLOT_A;

  EEPROM.update(destino,0);
  EEPROM.update(destino+1,0);
  const uint8_t *p=(const uint8_t*)&nuevo;
  for(uint8_t i=2;i<sizeof(DatosConfig);++i) EEPROM.update(destino+i,p[i]);
  EEPROM.update(destino,(uint8_t)(FIRMA & 0xFF));
  EEPROM.update(destino+1,(uint8_t)(FIRMA >> 8));
}
