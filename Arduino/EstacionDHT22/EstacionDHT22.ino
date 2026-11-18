#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <avr/pgmspace.h>
#include "ControlAmbiental.h"
#include "ReceptorIR.h"
#include "Configuracion.h"

const uint8_t PIN_DHT=2, LED_VERDE=4, LED_AMARILLO=5, LED_ROJO=6;
const uint8_t PIN_BOTON=7, PIN_IR=8, RGB_ROJO=9, RGB_VERDE=10, RGB_AZUL=11;
const uint8_t LED_LOCAL=LED_BUILTIN;
const unsigned long INTERVALO=2500UL;
DHT sensor(PIN_DHT,DHT22);
LiquidCrystal_I2C lcd(0x27,16,2);
ControlAmbiental control;
ReceptorIR receptor;
MedicionAmbiental lectura={0,0,false};
unsigned long tLectura=0, secuencia=0, tPing=0, tIR=0, tBoton=0, inicioBoton=0, tLCD=0;
char comando[30];
uint8_t nComando=0;
bool descartarLinea=false;
bool botonAnterior=HIGH, botonEstable=HIGH, largaHecha=false;
char cacheLCD[2][17]={{0},{0}};

enum VistaLCD { V_RESUMEN, V_MENU, V_EDITAR, V_DETALLE, V_APRENDER };
VistaLCD vista=V_RESUMEN;

const uint8_t N_MENU=7;
uint8_t opcion=0, pasoIR=1;
char tipoEdicion='T';
int valorEdicion=30;
bool pruebaES=false;
bool secuenciaVisual=false;
unsigned long inicioVisual=0;
char colorPrueba='0';
unsigned long tPruebaES=0, tReporteES=0;

void fila(uint8_t f,const char *txt) {
  char relleno[17];
  memset(relleno,' ',16); relleno[16]='\0';
  for(uint8_t i=0;i<16 && txt[i];i++) relleno[i]=txt[i];
  if(memcmp(cacheLCD[f],relleno,17)==0) return;
  memcpy(cacheLCD[f],relleno,17);
  lcd.setCursor(0,f); lcd.print(relleno);
}

void tituloMenu(uint8_t n,char *dst) {
  switch(n) {
    case 0: strcpy_P(dst,PSTR("AJUSTAR TEMP")); break;
    case 1: strcpy_P(dst,PSTR("AJUSTAR HUM")); break;
    case 2: strcpy_P(dst,PSTR("ALARMA / ACK")); break;
    case 3: strcpy_P(dst,PSTR("PAUSA / REANUD")); break;
    case 4: strcpy_P(dst,PSTR("CONFIG CONTROL")); break;
    case 5: strcpy_P(dst,PSTR("DIAGNOSTICO")); break;
    default: strcpy_P(dst,PSTR("SALIR")); break;
  }
}

void nombreIR(uint8_t indice, char *dst) {
  switch(indice) {
    case 1: strcpy_P(dst,PSTR("LCD SIGUIENTE")); break;
    case 2: strcpy_P(dst,PSTR("PAUSAR")); break;
    case 3: strcpy_P(dst,PSTR("REANUDAR")); break;
    case 4: strcpy_P(dst,PSTR("ACK / ENTRAR")); break;
    case 5: strcpy_P(dst,PSTR("TEMP +")); break;
    case 6: strcpy_P(dst,PSTR("TEMP -")); break;
    case 7: strcpy_P(dst,PSTR("HUMEDAD +")); break;
    default: strcpy_P(dst,PSTR("HUMEDAD -")); break;
  }
}

void actualizarLCD() {
  char a[17]={0},b[17]={0};
  if(vista==V_MENU) {
    snprintf_P(a,sizeof(a),PSTR("MENU %u/%u"),(unsigned)(opcion+1),(unsigned)N_MENU);
    char nombre[16]; tituloMenu(opcion,nombre);
    snprintf_P(b,sizeof(b),PSTR("> %.14s"),nombre);
  } else if(vista==V_EDITAR) {
    snprintf_P(a,sizeof(a),PSTR("%s MAX: %d"),tipoEdicion=='T'?"TEMP":"HUM",valorEdicion);
    strcpy_P(b,PSTR("CORTO+ 1S- 3S:OK"));
  } else if(vista==V_APRENDER) {
    snprintf_P(a,sizeof(a),PSTR("IR TECLA %u/8"),(unsigned)pasoIR);
    nombreIR(pasoIR,b);
  } else if(vista==V_DETALLE) {
    if(opcion==2) {
      snprintf_P(a,sizeof(a),PSTR("EST:%s"),control.nombreEstado());
      if(control.estado()==ALERTA) strcpy_P(b,PSTR("LARGO: ACK"));
      else strcpy_P(b,PSTR("NO HAY ALARMA"));
    } else if(opcion==5) {
      if(lectura.valida) strcpy_P(a,PSTR("SENSOR: OK"));
      else strcpy_P(a,PSTR("SENSOR: ERROR"));
      snprintf_P(b,sizeof(b),PSTR("PC:%s IR:%u"),(tPing && millis()-tPing<8000UL)?"SI":"NO",(unsigned)receptor.cantidadConfigurada());
    }
  } else {
    if(lectura.valida) {
      char t[8],h[8];
      dtostrf(lectura.temperatura,4,1,t); dtostrf(lectura.humedad,3,0,h);
      snprintf_P(a,sizeof(a),PSTR("T:%.5sC H:%.4s%%"),t,h);
    } else strcpy_P(a,PSTR("DHT22 SIN DATOS"));
    snprintf_P(b,sizeof(b),PSTR("%.16s"),control.nombreEstado());
  }
  fila(0,a); fila(1,b);
}

void enviarMedicion() {
  Serial.print(F("DATA,")); Serial.print(++secuencia);Serial.print(',');
  if(lectura.valida) {
    Serial.print(lectura.temperatura,1);Serial.print(',');Serial.print(lectura.humedad,1);
  } else Serial.print(F("NaN,NaN"));
  Serial.print(',');Serial.print(control.nombreEstado());Serial.print(',');
  Serial.print(control.limiteT());Serial.print(',');Serial.println(control.limiteH());
}

void cambiarLimite(char tipo,int valor) {
  if(!control.fijarLimite(tipo,valor)) {Serial.println(F("ERR,LIMITE"));return;}
  Configuracion::guardar((uint8_t)control.limiteT(),(uint8_t)control.limiteH());
  if(lectura.valida && !control.estaPausado()) control.actualizar(lectura);
  actualizarLCD();
  Serial.print(F("ACK,SET,"));Serial.print(tipo);Serial.print(',');Serial.println(valor);
  Serial.println(F("EVENT,CONFIG,GUARDADA"));
  enviarMedicion();
}

void reconocer() {
  if(control.estado()!=ALERTA) {Serial.println(F("ERR,SIN_ALARMA"));return;}
  control.reconocerAlarma();
  Serial.println(F("EVENT,ALARMA,RECONOCIDA"));actualizarLCD();
}

void empezarIR() {
  vista=V_APRENDER; pasoIR=1; receptor.aprender(pasoIR);
  Serial.println(F("EVENT,IR,ASISTENTE_INICIADO")); actualizarLCD();
}

void proximaIR() {
  if(pasoIR>=8) {
    receptor.cancelarAprendizaje(); vista=V_MENU;
    Serial.println(F("EVENT,IR,ASISTENTE_TERMINADO"));
  } else {
    pasoIR++; receptor.aprender(pasoIR);
  }
  actualizarLCD();
}

void toqueCorto() {
  if(vista==V_RESUMEN) {vista=V_MENU;opcion=0;}
  else if(vista==V_MENU) opcion=(opcion+1)%N_MENU;
  else if(vista==V_EDITAR) valorEdicion=(valorEdicion+1)%((tipoEdicion=='T')?61:101);
  else if(vista==V_APRENDER) proximaIR();
  else vista=V_MENU;
  actualizarLCD();
}

void toqueLargo() {
  if(vista==V_RESUMEN) {
    if(control.estado()==ALERTA) reconocer();
    else {vista=V_MENU;opcion=0;}
  } else if(vista==V_MENU) {
    if(opcion<=1) {
      tipoEdicion=opcion==0?'T':'H';
      valorEdicion=tipoEdicion=='T'?control.limiteT():control.limiteH();
      vista=V_EDITAR;
    } else if(opcion==2 || opcion==5) vista=V_DETALLE;
    else if(opcion==3) {
      bool pausa=!control.estaPausado();control.pausar(pausa);
      Serial.println(pausa?F("EVENT,LOCAL,PAUSA"):F("EVENT,LOCAL,REANUDAR"));
      if(!pausa) tLectura=millis()-INTERVALO;
      enviarMedicion();vista=V_RESUMEN;
    } else if(opcion==4) empezarIR();
    else vista=V_RESUMEN;
  } else if(vista==V_EDITAR) {
    cambiarLimite(tipoEdicion,valorEdicion);vista=V_MENU;
  } else if(vista==V_APRENDER) {
    receptor.cancelarAprendizaje();vista=V_MENU;
    Serial.println(F("EVENT,IR,ASISTENTE_CANCELADO"));
  } else if(vista==V_DETALLE) {
    if(opcion==2) reconocer();
    vista=V_MENU;
  }
  actualizarLCD();
}

void actualizarSemaforo() {
  bool parp=(millis()/400UL)%2UL==0UL;
  bool verde=false, amarillo=false, rojo=false;
  switch(control.estado()) {
    case NORMAL:verde=true;break;
    case ADVERTENCIA:amarillo=true;break;
    case ALERTA:rojo=control.alarmaReconocida()||parp;break;
    case ERROR_SENSOR:rojo=parp;break;
    case PAUSADO:case INICIANDO:amarillo=parp;break;
    default:break;
  }
  digitalWrite(LED_VERDE,verde?HIGH:LOW);
  digitalWrite(LED_AMARILLO,amarillo?HIGH:LOW);
  digitalWrite(LED_ROJO,rojo?HIGH:LOW);
  digitalWrite(LED_LOCAL,rojo?HIGH:LOW);
}

void actualizarRGB() {
  if(secuenciaVisual) {
    unsigned long paso=(millis()-inicioVisual)/1000UL;
    if(paso>=4UL) {secuenciaVisual=false;pruebaES=false;}
    else colorPrueba=paso==0?'R':paso==1?'G':paso==2?'B':'V';
  }
  if(pruebaES) {
    digitalWrite(RGB_ROJO,(colorPrueba=='R'||colorPrueba=='V')?HIGH:LOW);
    digitalWrite(RGB_VERDE,colorPrueba=='G'?HIGH:LOW);
    digitalWrite(RGB_AZUL,(colorPrueba=='B'||colorPrueba=='V')?HIGH:LOW);
    return;
  }
  bool pc=tPing && millis()-tPing<8000UL;
  bool ir=tIR && millis()-tIR<900UL;
  digitalWrite(RGB_ROJO,ir?HIGH:LOW);
  digitalWrite(RGB_VERDE,(!pc&&!ir)?HIGH:LOW);
  digitalWrite(RGB_AZUL,(pc||ir)?HIGH:LOW);
}

void informarES() {
  if(!tPing || millis()-tPing>=8000UL) return;
  Serial.print(F("IO,"));
  Serial.print((int)digitalRead(PIN_BOTON));Serial.print(',');
  Serial.print((int)digitalRead(RGB_ROJO));Serial.print(',');
  Serial.print((int)digitalRead(RGB_VERDE));Serial.print(',');
  Serial.print((int)digitalRead(RGB_AZUL));Serial.print(',');
  Serial.println(pruebaES?1:0);
}

void muestrear() {
  EstadoAmbiental anterior=control.estado();
  lectura.temperatura=sensor.readTemperature();lectura.humedad=sensor.readHumidity();
  lectura.valida=!isnan(lectura.temperatura)&&!isnan(lectura.humedad)
      &&lectura.humedad>=0&&lectura.humedad<=100;
  control.actualizar(lectura);
  actualizarLCD();enviarMedicion();
  if(anterior!=control.estado()) {
    Serial.print(F("EVENT,ESTADO,"));Serial.println(control.nombreEstado());
  }
}

void ejecutarComando(const char *cmd) {
  if(strncmp(cmd,"RGB,",4)==0) {
    char c=cmd[4];
    if(strlen(cmd)!=5 || (c!='A' && c!='0' && c!='R' && c!='G' && c!='B' && c!='V' && c!='T')) {
      Serial.println(F("ERR,RGB"));return;
    }
    secuenciaVisual=(c=='T');
    if(c=='A') pruebaES=false;
    else {pruebaES=true;colorPrueba=c;tPruebaES=millis();}
    if(secuenciaVisual) {inicioVisual=millis();colorPrueba='R';}
    actualizarRGB();informarES();
    if(c=='V') Serial.println(F("EVENT,MANTENIMIENTO,LOCALIZACION"));
    if(c=='T') Serial.println(F("EVENT,MANTENIMIENTO,VERIFICACION"));
    Serial.print(F("ACK,RGB,"));Serial.println(c);
    return;
  }
  if(strcmp(cmd,"PING")==0) {tPing=millis(); Serial.println(F("ACK,PONG"));return;}
  if(strcmp(cmd,"GET")==0) {
    Serial.print(F("ACK,CONFIG,"));Serial.print(control.limiteT());Serial.print(',');
    Serial.println(control.limiteH()); enviarMedicion();return;
  }
  if(strcmp(cmd,"PAUSE")==0) {
    control.pausar(true);actualizarLCD();Serial.println(F("ACK,PAUSE"));enviarMedicion();return;
  }
  if(strcmp(cmd,"RESUME")==0) {
    control.pausar(false);tLectura=millis()-INTERVALO;actualizarLCD();
    Serial.println(F("ACK,RESUME"));return;
  }
  if(strcmp(cmd,"ACK")==0) {reconocer();Serial.println(F("ACK,ALARMA"));return;}
  if(strcmp(cmd,"SCREEN")==0) {toqueCorto();Serial.println(F("ACK,SCREEN"));return;}
  if(strcmp(cmd,"MENU")==0) {vista=V_MENU;opcion=0;actualizarLCD();Serial.println(F("ACK,MENU"));return;}
  if(strcmp(cmd,"HOME")==0) {receptor.cancelarAprendizaje();vista=V_RESUMEN;actualizarLCD();Serial.println(F("ACK,HOME"));return;}
  if(strcmp(cmd,"SELECT")==0) {toqueLargo();Serial.println(F("ACK,SELECT"));return;}
  if(strcmp(cmd,"IR_CLEAR")==0) {receptor.borrar();Serial.println(F("ACK,IR_CLEAR"));actualizarLCD();return;}
  if(strncmp(cmd,"LEARN,",6)==0) {
    char *fin;long n=strtol(cmd+6,&fin,10);
    if(fin!=cmd+6 && *fin=='\0' && n>=1 && n<=8 && receptor.aprender((uint8_t)n)) {
      Serial.print(F("ACK,LEARN,"));Serial.println(n);
    } else Serial.println(F("ERR,IR_ACCION"));
    return;
  }
  if(strncmp(cmd,"SET,",4)==0 && strlen(cmd)>=7 && cmd[5]==',') {
    char *fin;long n=strtol(cmd+6,&fin,10);
    if(fin!=cmd+6 && *fin=='\0' && n>=0 && n<=100 && (cmd[4]=='T'||cmd[4]=='H'))
      cambiarLimite(cmd[4],(int)n);
    else Serial.println(F("ERR,LIMITE"));
    return;
  }
  Serial.println(F("ERR,COMANDO"));
}

void atenderSerial() {
  while(Serial.available()) {
    char c=(char)Serial.read();
    if(c=='\r') continue;
    if(c=='\n') {
      if(!descartarLinea) {comando[nComando]='\0';if(nComando)ejecutarComando(comando);}
      nComando=0;descartarLinea=false;
    } else if(descartarLinea) continue;
    else if(nComando<sizeof(comando)-1) comando[nComando++]=c;
    else {nComando=0;descartarLinea=true;Serial.println(F("ERR,LINEA_LARGA"));}
  }
}

void atenderBoton() {
  bool valor=digitalRead(PIN_BOTON);
  if(valor!=botonAnterior)tBoton=millis();
  botonAnterior=valor;
  if(millis()-tBoton<35UL || valor==botonEstable) {

    if(botonEstable==LOW && !pruebaES && !largaHecha &&
       millis()-inicioBoton >= (vista==V_EDITAR?3000UL:1300UL)) {
      largaHecha=true;toqueLargo();
    }
    return;
  }
  botonEstable=valor;
  if(pruebaES) {
    if(valor==LOW) {inicioBoton=millis();largaHecha=false;}
    else {
      secuenciaVisual=false;
      colorPrueba=colorPrueba=='0'?'R':colorPrueba=='R'?'G':colorPrueba=='G'?'B':'0';
      tPruebaES=millis();actualizarRGB();
    }
    informarES();return;
  }
  if(valor==LOW){inicioBoton=millis();largaHecha=false;}
  else if(!largaHecha) {

    if(vista==V_EDITAR && millis()-inicioBoton>=900UL) {
      if(valorEdicion>0)--valorEdicion;
      actualizarLCD();
    } else toqueCorto();
  }
}

void atenderIR() {
  ReceptorIR::Accion accion=receptor.procesar();
  if(receptor.ultimaTeclaAprendida()) {
    uint8_t n=receptor.ultimaTeclaAprendida();
    Serial.print(F("EVENT,IR,APRENDIDA_"));Serial.println(n);
    if(vista==V_APRENDER && n==pasoIR) proximaIR();
    else actualizarLCD();
    return;
  }
  if(accion==ReceptorIR::NINGUNA) return;
  tIR=millis();
  Serial.print(F("EVENT,IR,ACCION_"));Serial.println((int)accion);
  switch(accion) {
    case ReceptorIR::PANTALLA:toqueCorto();break;
    case ReceptorIR::PAUSA:control.pausar(true);enviarMedicion();actualizarLCD();break;
    case ReceptorIR::REANUDAR:control.pausar(false);tLectura=millis()-INTERVALO;actualizarLCD();break;
    case ReceptorIR::RECONOCER:toqueLargo();break;
    case ReceptorIR::TEMP_MAS:
    case ReceptorIR::TEMP_MENOS:
    case ReceptorIR::HUM_MAS:
    case ReceptorIR::HUM_MENOS: {
      char tipo=(accion==ReceptorIR::TEMP_MAS||accion==ReceptorIR::TEMP_MENOS)?'T':'H';
      int cambio=(accion==ReceptorIR::TEMP_MAS||accion==ReceptorIR::HUM_MAS)?1:-1;
      if(vista==V_EDITAR && tipoEdicion==tipo) {
        valorEdicion=constrain(valorEdicion+cambio,0,tipo=='T'?60:100);actualizarLCD();
      } else cambiarLimite(tipo,(tipo=='T'?control.limiteT():control.limiteH())+cambio);
      break;
    }
    default:break;
  }
}

void setup() {
  Serial.begin(9600);
  pinMode(LED_VERDE,OUTPUT);pinMode(LED_AMARILLO,OUTPUT);pinMode(LED_ROJO,OUTPUT);
  pinMode(RGB_ROJO,OUTPUT);pinMode(RGB_VERDE,OUTPUT);pinMode(RGB_AZUL,OUTPUT);
  pinMode(LED_LOCAL,OUTPUT);pinMode(PIN_BOTON,INPUT_PULLUP);
  uint8_t lt=30,lh=75;Configuracion::cargar(lt,lh);
  control.fijarLimite('T',lt);control.fijarLimite('H',lh);
  sensor.begin();Wire.begin();lcd.init();lcd.backlight();
  receptor.comenzar(PIN_IR);
  fila(0,"AMBIENT 1.0");fila(1,"INICIANDO...");
  tLectura=millis();
  Serial.println(F("READY,AMBIENT,2"));
  Serial.print(F("ACK,CONFIG,"));Serial.print(control.limiteT());Serial.print(',');
  Serial.println(control.limiteH());
}

void loop() {
  atenderSerial();atenderBoton();atenderIR();
  if(pruebaES && !secuenciaVisual && millis()-tPruebaES>=(colorPrueba=='V'?7000UL:120000UL)) pruebaES=false;
  actualizarSemaforo();actualizarRGB();
  if(millis()-tReporteES>=500UL) {tReporteES=millis();informarES();}
  if(!control.estaPausado() && millis()-tLectura>=INTERVALO) {
    tLectura=millis();muestrear();
  }
  if(millis()-tLCD>=1200UL) {tLCD=millis();actualizarLCD();}
}
