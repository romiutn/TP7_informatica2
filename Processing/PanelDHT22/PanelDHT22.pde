import processing.serial.*;
import info2ambiente.*;
import java.io.File;
import java.io.IOException;
import java.util.ArrayList;

Serial arduino;
Historial historial;
String[] puertos = new String[0];
int seleccionado=0;
boolean conectado=false, pausadoPC=false;
String aviso="Seleccione puerto en la seccion CONEXION";
String ultimoEvento="Sin eventos recientes";
Medicion actual=null;
int limiteT=30, limiteH=75, totalSesion=0;
String ultimoEstado="";
long ultimoDatoMs=0, ultimoPingPC=0, ultimoPongPC=0;
ArrayList<Medicion> muestras = new ArrayList<Medicion>();
boolean datoHistorico=true;
long ultimaSecuenciaAceptada=-1;
String fechaUltima="";
int reiniciosObservados=0;
final int MAX_MUESTRAS=100;
String[] filasHistorial=new String[0];
int entradaD7=-1, salidaR=-1, salidaG=-1, salidaB=-1;
boolean modoPruebaES=false;
long ultimaOrdenIR=0;

final int PC_DESCONECTADO=0, PC_ESPERANDO=1, PC_NORMAL=2, PC_ALERTA=3,
          PC_ERROR=4, PC_PAUSADO=5, PC_ADVERTENCIA=6;
int estadoPC=PC_DESCONECTADO;

final String[] SECCIONES={"PANEL", "GRAFICOS", "ALARMAS", "CONTROL IR", "HISTORIAL", "CONEXION", "MANTENIMIENTO"};
int pagina=0;
boolean confirmarBorradoIR=false;
long horaConfirmacionIR=0;

final int FONDO=0xff101827, SUPERFICIE=0xff1c293b, SUPERFICIE2=0xff28374c;
final int TEXTO=0xffedf4ff, SECUNDARIO=0xffb0c5df;
final int ACENTO=0xff49b0ee, VERDE=0xff42c58b, ROJO=0xfff2747b, AMARILLO=0xffffcf75;

void setup() {
  size(900,620);
  surface.setTitle("AMBIENT | Centro de supervision - Informática II UTN");
  surface.setResizable(true);
  textFont(createFont("Arial",15));
  try { historial = new Historial(new File(sketchPath("historial_mediciones.csv")),150); }
  catch(IOException ex) { aviso="Error abriendo historial: "+ex.getMessage(); }
  recuperarCurvaDelCSV();
  refrescarPuertos();
}

void recuperarCurvaDelCSV() {
  if(historial==null)return;
  String[] registros=historial.ultimos();
  for(String fila:registros) {
    String[] p=split(fila,',');
    if(p==null || p.length!=7) continue;
    String trama="DATA,"+p[1]+","+p[2]+","+p[3]+","+p[4]+","+p[5]+","+p[6];
    Medicion m=Medicion.desdeSerial(trama);
    if(m==null)continue;
    muestras.add(m);
    if(muestras.size()>MAX_MUESTRAS)muestras.remove(0);
    actual=m;
    fechaUltima=p[0];
    limiteT=m.limiteTemperatura;limiteH=m.limiteHumedad;
  }
  if(actual!=null)aviso="Historial recuperado. Valores historicos hasta conectar el Arduino";
}

void draw() {
  background(FONDO);

  if (conectado && arduino!=null && millis()-ultimoPingPC>=3000) {
    ultimoPingPC=millis();
    try { arduino.write("PING\n"); }
    catch(Exception ex) { aviso="Fallo PING: "+ex.getMessage(); }
  }
  evaluarEstadoPC();
  if (frameCount%30==1 && historial!=null) filasHistorial=historial.ultimos();
  if (confirmarBorradoIR && millis()-horaConfirmacionIR>5000) confirmarBorradoIR=false;

  noStroke(); fill(SUPERFICIE); rect(0,0,width,81);
  fill(TEXTO); textSize(26); text("AMBIENT",24,36);
  fill(SECUNDARIO); textSize(13); text("CENTRO DE SUPERVISION AMBIENTAL  |  INFO II  |  UTN",26,60);
  String estado = nombreEstadoPC();
  fill(colorEstado(estado)); rect(width-194,22,170,32,9);
  fill(FONDO); textAlign(CENTER,CENTER); textSize(13); text(estado,width-109,38); textAlign(LEFT,BASELINE);

  float anchoTab=(width-48-(SECCIONES.length-1)*8)/(float)SECCIONES.length;
  for(int i=0;i<SECCIONES.length;i++) {
    float x=24+i*(anchoTab+8);
    fill(i==pagina?ACENTO:SUPERFICIE2); rect(x,95,anchoTab,44,9);
    fill(i==pagina?FONDO:TEXTO); textAlign(CENTER,CENTER);
    textSize(width<860?11:13); text(SECCIONES[i],x+anchoTab/2,117);
    textAlign(LEFT,BASELINE);
  }

  switch(pagina) {
    case 0: dibujarPanel(); break;
    case 1: dibujarGraficos(); break;
    case 2: dibujarAlarmas(); break;
    case 3: dibujarIR(); break;
    case 4: dibujarHistorial(); break;
    case 5: dibujarConexion(); break;
    case 6: dibujarES(); break;
  }

  noStroke(); fill(0xff0a111f); rect(0,height-47,width,47);
  fill(SECUNDARIO); textSize(12);
  text("MENSAJE: "+acortar(aviso,max(38,(width-110)/7)),24,height-19);
}

void panel(float x,float y,float w,float h,String titulo) {
  fill(SUPERFICIE); noStroke(); rect(x,y,w,h,12);
  fill(SECUNDARIO); textSize(14); text(titulo,x+16,y+28);
}

void boton(float x,float y,float w,float h,String etiqueta,int estilo) {
  fill(estilo==1?ACENTO:estilo==2?0xff753d4c:SUPERFICIE2);
  noStroke(); rect(x,y,w,h,8);
  fill(estilo==1?FONDO:TEXTO); textSize(13);
  textAlign(CENTER,CENTER); text(etiqueta,x+w/2,y+h/2); textAlign(LEFT,BASELINE);
}

void indicador(float x,float y,float w,float h,String titulo,String valor,String subtitulo,int acento) {
  panel(x,y,w,h,titulo);
  fill(acento); textSize(41); text(valor,x+18,y+88);
  fill(SECUNDARIO); textSize(14); text(subtitulo,x+18,y+h-20);
}

void dibujarPanel() {
  float mitad=(width-72)/2.0f;
  String t=actual==null||Float.isNaN(actual.temperatura)?"--":nf(actual.temperatura,0,1)+" C";
  String h=actual==null||Float.isNaN(actual.humedad)?"--":nf(actual.humedad,0,1)+" %";
  indicador(24,161,mitad,150,"TEMPERATURA",t,"Umbral: "+limiteT+" C",AMARILLO);
  indicador(48+mitad,161,mitad,150,"HUMEDAD",h,"Umbral: "+limiteH+" %",ACENTO);
  panel(24,326,width-48,111,"ESTADO DE SUPERVISION");
  String lecturaEstado=actual==null?"SIN MEDICIONES":actual.estado;
  fill(colorEstado(lecturaEstado)); textSize(25); text(lecturaEstado,43,385);
  fill(SECUNDARIO); textSize(13);
  text("Nuevos: "+totalSesion+"   |   Grafico: "+muestras.size()+
       "   |   CSV: "+(historial==null?"--":historial.cantidadEnMemoria())+
       (datoHistorico?"  |  HISTORICO (esperando datos)":"  |  EN VIVO"),43,415);
  panel(24,450,width-48,108,"ACCIONES RAPIDAS");
  float w=(width-88)/3.0f;
  boton(42,494,w,42,"PAUSAR",0);
  boton(50+w,494,w,42,"REANUDAR",1);
  boton(58+2*w,494,w,42,"RECONOCER ALARMA",2);
}

void dibujarGraficos() {
  panel(24,161,width-48,384,"EVOLUCION REAL   |   ESCALA AUTOMATICA POR SENSOR");
  fill(SECUNDARIO);textSize(12);
  text("Cada grafico amplifica variaciones pequenas. No se alteran las mediciones.",40,207);
  dibujarSerie(0,222,141,"TEMPERATURA (C)",AMARILLO,1.0f);
  dibujarSerie(1,392,141,"HUMEDAD (%)",ACENTO,2.0f);
  fill(SECUNDARIO);textSize(12);
  text("Muestras mostradas: "+muestras.size()+" / "+MAX_MUESTRAS+
       (datoHistorico?"  |  Ultima lectura HISTORICA":"  |  Mediciones en tiempo real"),38,562);
}

void dibujarSerie(int tipo,float y,float h,String titulo,int tinta,float amplitudMinima) {
  float izq=92, ancho=width-139;
  float minimo=Float.POSITIVE_INFINITY, maximo=Float.NEGATIVE_INFINITY;
  int validos=0;
  for(Medicion m:muestras) {
    float v=tipo==0?m.temperatura:m.humedad;
    if(Float.isNaN(v)||Float.isInfinite(v))continue;
    minimo=min(minimo,v);maximo=max(maximo,v);validos++;
  }
  if(validos==0) {
    fill(SECUNDARIO);textSize(14);text("Sin mediciones disponibles",izq+15,y+55);return;
  }
  float rango=max(maximo-minimo,amplitudMinima);
  float centro=(minimo+maximo)/2.0f;
  float inferior=centro-rango*0.64f, superior=centro+rango*0.64f;

  fill(TEXTO);textSize(13);text(titulo,izq,y-8);
  fill(SECUNDARIO);textSize(12);
  text("MIN "+nf(minimo,0,1)+"   MAX "+nf(maximo,0,1)+
       "   CAMBIO "+nf(maximo-minimo,0,1),izq+190,y-8);
  stroke(0xff3d536c);strokeWeight(1);
  for(int k=0;k<=2;k++) {
    float yy=y+k*h/2.0f;
    line(izq,yy,izq+ancho,yy);
  }
  noStroke();fill(SECUNDARIO);textSize(11);
  text(nf(superior,0,1),30,y+3); text(nf(inferior,0,1),30,y+h+3);
  stroke(tinta);strokeWeight(2.5f);noFill();
  boolean dibujando=false;
  for(int i=0;i<muestras.size();i++) {
    Medicion m=muestras.get(i);
    float v=tipo==0?m.temperatura:m.humedad;
    if(Float.isNaN(v)||Float.isInfinite(v)) {
      if(dibujando){endShape();dibujando=false;}
      continue;
    }
    if(!dibujando){beginShape();dibujando=true;}
    float xx=izq+(muestras.size()==1?ancho/2.0f:i*ancho/(muestras.size()-1));
    float yy=y+h-(v-inferior)*h/(superior-inferior);
    vertex(xx,yy);
  }
  if(dibujando)endShape();
  noStroke();fill(SECUNDARIO);textSize(11);
  if(maximo==minimo)text("Sin variacion detectable en estas muestras",izq,y+h+17);
}

void dibujarAlarmas() {
  float mitad=(width-72)/2.0f;
  panel(24,161,mitad,250,"LIMITE DE TEMPERATURA");
  panel(48+mitad,161,mitad,250,"LIMITE DE HUMEDAD");
  fill(TEXTO); textSize(42);
  text(limiteT+" C",47,256); text(limiteH+" %",70+mitad,256);
  boton(44,301,(mitad-70)/2,52,"- 1 C",0);
  boton(55+(mitad-70)/2,301,(mitad-70)/2,52,"+ 1 C",1);
  boton(68+mitad,301,(mitad-70)/2,52,"- 1 %",0);
  boton(79+mitad+(mitad-70)/2,301,(mitad-70)/2,52,"+ 1 %",1);
  fill(SECUNDARIO); textSize(13);
  text("Temperatura de aviso: limite - 2 C",42,386);
  text("Humedad de aviso: limite - 10 %",66+mitad,386);
  panel(24,427,width-48,129,"CONTROL DE ALARMA Y MUESTREO");
  float w=(width-88)/3.0f;
  boton(42,469,w,48,"PAUSAR",0);
  boton(50+w,469,w,48,"REANUDAR",1);
  boton(58+2*w,469,w,48,"RECONOCER ALARMA",2);
  fill(SECUNDARIO); textSize(12);
  text("Reconocer no cancela una alarma que sigue activa.",42,544);
}

void dibujarIR() {
  panel(24,161,width-48,88,"CONTROL REMOTO INFRARROJO  |  CAR MP3");
  fill(SECUNDARIO); textSize(13);
  text("Selecciona una accion, apunta el control al receptor y pulsa la tecla deseada.",42,214);
  String[] acciones={"LCD / SIGUIENTE", "PAUSAR", "REANUDAR", "RECONOCER ALARMA",
                     "TEMPERATURA +", "TEMPERATURA -", "HUMEDAD +", "HUMEDAD -"};
  float w=(width-72)/2.0f;
  for(int i=0;i<8;i++) {
    float x=i%2==0?24:48+w;
    float y=266+(i/2)*55;
    boton(x,y,w,45,(i+1)+"  |  "+acciones[i],0);
  }
  boton(24,499,width-48,40,confirmarBorradoIR?"CONFIRMAR BORRADO DE TECLAS":"BORRAR ASIGNACIONES IR",2);
  fill(SECUNDARIO); textSize(12);
  text("Las teclas IR se conservan en EEPROM. Tambien hay asistente en la LCD.",26,557);
}

void dibujarHistorial() {
  int recuperados=historial==null?0:historial.cantidadEnMemoria();
  panel(24,161,width-48,71,"REGISTRO PERSISTENTE DE MEDICIONES");
  fill(TEXTO); textSize(15);
  text("Nuevos en esta ejecucion: "+totalSesion+"   |   Memoria de historial: "+recuperados,42,212);
  panel(24,242,width-48,305,"ULTIMAS FILAS DEL HISTORIAL CSV");
  fill(SECUNDARIO); textSize(12);
  text("FECHA / HORA                     ID        T (C)      H (%)      ESTADO",42,292);
  stroke(0xff3c516c); line(42,301,width-42,301); noStroke();
  if(filasHistorial.length==0) {
    fill(SECUNDARIO); text("Todavia no se guardaron registros.",43,337);
  } else {
    int n=min(8,filasHistorial.length);
    for(int i=0;i<n;i++) {
      String fila=filasHistorial[filasHistorial.length-n+i];
      fill(i%2==0?SUPERFICIE2:SUPERFICIE); rect(39,310+i*27,width-79,26,4);
      fill(TEXTO); textSize(12);
      text(acortar(fila,max(55,(width-100)/7)),45,328+i*27);
    }
  }
  fill(SECUNDARIO); textSize(12);
  text("Los CSV no se sobrescriben: al volver a ejecutar se recuperan los datos anteriores.",26,562);
}

void dibujarConexion() {
  panel(24,161,width-48,73,"ESTADO DEL ENLACE USB / SERIAL");
  fill(conectado?VERDE:AMARILLO); textSize(16);
  text(conectado?"CONECTADO":"DESCONECTADO",42,206);
  fill(SECUNDARIO); textSize(13);
  text("9600 baudios | Reinicios detectados: "+reiniciosObservados+" | Datos: "+(datoHistorico?"históricos":"en vivo"),242,205);
  panel(24,246,width-48,207,"SELECCIONAR PUERTO DEL ARDUINO");
  float w=(width-90)/3.0f;
  for(int i=0;i<min(6,puertos.length);i++) {
    float x=42+(i%3)*(w+9), y=294+(i/3)*49;
    boton(x,y,w,41,(seleccionado==i?"● ":"")+(i+1)+". "+acortar(puertos[i],15),seleccionado==i?1:0);
  }
  boton(42,398,(width-100)/2,42,"REFRESCAR PUERTOS",0);
  boton(52+(width-100)/2,398,(width-100)/2,42,conectado?"DESCONECTAR":"CONECTAR",1);
  panel(24,465,width-48,94,"CONTROL DE PANTALLA LCD DESDE PROCESSING");
  float t=(width-104)/4.0f;
  boton(40,503,t,42,"MENU LCD",0);
  boton(48+t,503,t,42,"SIGUIENTE",0);
  boton(56+2*t,503,t,42,"SELECCIONAR",1);
  boton(64+3*t,503,t,42,"INICIO LCD",0);
}


void dibujarES() {
  panel(24,161,width-48,80,"MANTENIMIENTO  |  VERIFICACION DE INDICADORES");
  fill(SECUNDARIO); textSize(13);
  text("Comproba el pulsador y el RGB sin interrumpir las alarmas ambientales.",42,211);

  panel(24,253,width-48,112,"ENTRADAS DEL EQUIPO");
  fill(TEXTO); textSize(16);
  String pulsador=entradaD7<0?"Sin lectura":entradaD7==0?"LOW - presionado":"HIGH - suelto";
  text("Pulsador (D7): "+pulsador,42,303);
  fill(SECUNDARIO); textSize(13);
  text("Control remoto: "+(ultimaOrdenIR>0&&millis()-ultimaOrdenIR<1500?"Orden reconocida":"Esperando orden"),42,340);

  panel(24,378,width-48,179,"VERIFICACION VISUAL DEL INDICADOR RGB");
  fill(TEXTO); textSize(13);
  String r=salidaR<0?"?":salidaR==1?"HIGH":"LOW";
  String g=salidaG<0?"?":salidaG==1?"HIGH":"LOW";
  String b=salidaB<0?"?":salidaB==1?"HIGH":"LOW";
  text("Ordenes digitales:  D9 "+r+"    D10 "+g+"    D11 "+b,42,420);
  float w=(width-100)/4.0f;
  boton(42,432,w,42,"TEST ROJO",0);
  boton(48+w,432,w,42,"TEST VERDE",0);
  boton(54+2*w,432,w,42,"TEST AZUL",0);
  boton(60+3*w,432,w,42,"APAGAR",0);
  float w2=(width-108)/3.0f;
  boton(42,485,w2,39,"SECUENCIA 4 S",0);
  boton(50+w2,485,w2,39,"LOCALIZAR EQUIPO",2);
  boton(58+2*w2,485,w2,39,"VOLVER A AUTO",1);
  fill(SECUNDARIO); textSize(12);
  text(modoPruebaES?"Verificacion activa: pulsador local recorre los canales RGB.":
       "Funcion normal: RGB indica enlace con PC y ordenes infrarrojas.",42,545);
}

int colorEstado(String e) {
  if(e.equals("NORMAL"))return VERDE;
  if(e.equals("ALERTA")||e.equals("ERROR"))return ROJO;
  if(e.equals("ADVERTENCIA")||e.equals("PAUSADO")||e.equals("INICIANDO"))return AMARILLO;
  return ACENTO;
}

String acortar(String s,int maximo) {
  if(s==null)return "";
  if(s.length()<=maximo)return s;
  return s.substring(0,max(0,maximo-3))+"...";
}

void refrescarPuertos() {
  puertos=Serial.list();
  if(seleccionado>=puertos.length) seleccionado=0;
  if(puertos.length==0) aviso="No hay puertos COM. Revise USB y controlador";
}

void conectar() {
  if(conectado) {
    if(arduino!=null) {
      try {arduino.write("RGB,A\n");} catch(Exception ex) { }
      arduino.stop();
    }
    arduino=null;conectado=false;datoHistorico=true;
    estadoPC=PC_DESCONECTADO;aviso="Desconectado: graficos e historial conservados";return;
  }
  if(puertos.length==0||seleccionado>=puertos.length) {aviso="Seleccione un puerto primero";return;}
  try {
    arduino=new Serial(this,puertos[seleccionado],9600);
    arduino.clear();arduino.bufferUntil('\n');
    conectado=true;pausadoPC=false;estadoPC=PC_ESPERANDO;
    ultimoDatoMs=millis();ultimoPingPC=0;ultimoPongPC=0;
    ultimaSecuenciaAceptada=-1; datoHistorico=true;

    aviso="Puerto abierto: Arduino UNO puede reiniciarse; ajustes en EEPROM";
  } catch(Exception ex) {
    conectado=false;arduino=null;estadoPC=PC_DESCONECTADO;
    aviso="Error de puerto: "+ex.getMessage();
  }
}

void enviar(String comando) {
  if(!conectado||arduino==null) { aviso="Primero conecte Arduino (pestana CONEXION)"; pagina=5; return; }
  try { arduino.write(comando+"\n"); aviso="Enviado: "+comando; }
  catch(Exception ex) { aviso="Error enviando: "+ex.getMessage(); }
}

void serialEvent(Serial puerto) {
  String s=puerto.readStringUntil('\n');
  if(s==null)return;
  s=trim(s);
  if(s.startsWith("READY,")) {
    reiniciosObservados++;
    ultimaSecuenciaAceptada=-1;
    datoHistorico=true;
    aviso="Arduino inicio/reinicio; restaurando limites guardados en EEPROM";
    try {puerto.write("GET\n");} catch(Exception ex) {aviso="Sin respuesta GET: "+ex.getMessage();}
    return;
  }
  if(s.startsWith("IO,")) {
    String[] p=split(s,',');
    if(p.length==6) {
      entradaD7=Integer.parseInt(p[1]);salidaR=Integer.parseInt(p[2]);
      salidaG=Integer.parseInt(p[3]);salidaB=Integer.parseInt(p[4]);
      modoPruebaES=p[5].equals("1");
    }
    return;
  }
  if(!s.startsWith("DATA,")) {
    if(s.equals("ACK,PONG")) {ultimoPongPC=millis();return;}
    if(s.startsWith("EVENT,IR,ACCION_"))ultimaOrdenIR=millis();
    if(s.startsWith("EVENT,")) ultimoEvento=s;
    aviso=s;
    if(s.startsWith("EVENT,")&&historial!=null) {
      try {historial.guardarEvento(new File(sketchPath("eventos.csv")),fecha(),s);}
      catch(IOException ex) {aviso="Error guardando evento: "+ex.getMessage();}
    }
    return;
  }
  Medicion m=Medicion.desdeSerial(s);
  if(m==null) {aviso="Mensaje DATA invalido: "+acortar(s,70);return;}
  limiteT=m.limiteTemperatura;limiteH=m.limiteHumedad;

  if(m.estado.equals("INICIANDO") && Float.isNaN(m.temperatura)) {
    aviso="Arduino iniciando; se muestran los ultimos datos guardados";
    return;
  }
  actual=m;ultimoDatoMs=millis();fechaUltima=fecha();
  datoHistorico=false;
  pausadoPC=m.estado.equals("PAUSADO");
  boolean nueva=(m.secuencia!=ultimaSecuenciaAceptada);
  if(nueva) {
    ultimaSecuenciaAceptada=m.secuencia;

    if(!m.estado.equals("PAUSADO") && !m.estado.equals("INICIANDO")) {
      muestras.add(m);
      if(muestras.size()>MAX_MUESTRAS)muestras.remove(0);
      totalSesion++;
      if(historial!=null) {
        try {
          historial.guardar(fechaUltima,m);
          if(!m.estado.equals(ultimoEstado)&&(m.estado.equals("ALERTA")||m.estado.equals("ERROR")))
            historial.guardarEvento(new File(sketchPath("eventos.csv")),fechaUltima,m.estado);
        } catch(IOException ex) {aviso="Error guardando historial: "+ex.getMessage();}
      }
    }
  }
  ultimoEstado=m.estado;
}

void evaluarEstadoPC() {
  if(!conectado) { estadoPC=PC_DESCONECTADO; return; }
  if(actual==null || datoHistorico) { estadoPC=PC_ESPERANDO; return; }
  if(millis()-ultimoDatoMs>9000 && !pausadoPC) { estadoPC=PC_ERROR; return; }
  if(pausadoPC) { estadoPC=PC_PAUSADO; return; }
  if(actual.estado.equals("ERROR")) { estadoPC=PC_ERROR; return; }
  estadoPC=actual.estado.equals("ALERTA")?PC_ALERTA:
           actual.estado.equals("ADVERTENCIA")?PC_ADVERTENCIA:PC_NORMAL;
}

String nombreEstadoPC() {
  switch(estadoPC) {
    case PC_DESCONECTADO:return "DESCONECTADO";
    case PC_ESPERANDO:return "ESPERANDO";
    case PC_NORMAL:return "NORMAL";
    case PC_ALERTA:return "ALERTA";
    case PC_ERROR:return "ERROR";
    case PC_PAUSADO:return "PAUSADO";
    case PC_ADVERTENCIA:return "ADVERTENCIA";
  }
  return "DESCONOCIDO";
}

String fecha() {
  return nf(year(),4)+"-"+nf(month(),2)+"-"+nf(day(),2)+" "+
         nf(hour(),2)+":"+nf(minute(),2)+":"+nf(second(),2);
}

void mousePressed() {
  if(mouseY>=95&&mouseY<=139) {
    float w=(width-48-(SECCIONES.length-1)*8)/(float)SECCIONES.length;
    for(int i=0;i<SECCIONES.length;i++) if(dentro(mouseX,24+i*(w+8),w)&&mouseY<=139) {
      pagina=i; return;
    }
  }
  if(mouseY>=height-47) return;
  if(pagina==0) {
    if(entreY(494,536)) {
      float w=(width-88)/3.0f;
      if(dentro(mouseX,42,w)) enviar("PAUSE");
      else if(dentro(mouseX,50+w,w)) enviar("RESUME");
      else if(dentro(mouseX,58+2*w,w)) enviar("ACK");
    }
  } else if(pagina==2) {
    float mitad=(width-72)/2.0f, w=(mitad-70)/2;
    if(entreY(301,353)) {
      if(dentro(mouseX,44,w)) enviar("SET,T,"+max(0,limiteT-1));
      else if(dentro(mouseX,55+w,w)) enviar("SET,T,"+min(60,limiteT+1));
      else if(dentro(mouseX,68+mitad,w)) enviar("SET,H,"+max(0,limiteH-1));
      else if(dentro(mouseX,79+mitad+w,w)) enviar("SET,H,"+min(100,limiteH+1));
    }
    if(entreY(469,517)) {
      float x=(width-88)/3.0f;
      if(dentro(mouseX,42,x)) enviar("PAUSE");
      else if(dentro(mouseX,50+x,x)) enviar("RESUME");
      else if(dentro(mouseX,58+2*x,x)) enviar("ACK");
    }
  } else if(pagina==3) {
    float w=(width-72)/2.0f;
    for(int i=0;i<8;i++) {
      float x=i%2==0?24:48+w, y=266+(i/2)*55;
      if(dentro(mouseX,x,w)&&mouseY>=y&&mouseY<=y+45) {
        enviar("LEARN,"+(i+1));
        aviso="Presione en el CAR MP3 la tecla elegida para la accion "+(i+1);
        return;
      }
    }
    if(entreY(499,539)) {
      if(!confirmarBorradoIR) {
        confirmarBorradoIR=true; horaConfirmacionIR=millis();
        aviso="Presione de nuevo para confirmar que borrara TODAS las teclas IR";
      } else { enviar("IR_CLEAR"); confirmarBorradoIR=false; }
    }
  } else if(pagina==6) {
    float w=(width-100)/4.0f;
    if(entreY(432,474)) {
      if(dentro(mouseX,42,w)) enviar("RGB,R");
      else if(dentro(mouseX,48+w,w)) enviar("RGB,G");
      else if(dentro(mouseX,54+2*w,w)) enviar("RGB,B");
      else if(dentro(mouseX,60+3*w,w)) enviar("RGB,0");
    } else if(entreY(485,524)) {
      float w2=(width-108)/3.0f;
      if(dentro(mouseX,42,w2)) enviar("RGB,T");
      else if(dentro(mouseX,50+w2,w2)) enviar("RGB,V");
      else if(dentro(mouseX,58+2*w2,w2)) enviar("RGB,A");
    }
  } else if(pagina==5) {
    float w=(width-90)/3.0f;
    for(int i=0;i<min(6,puertos.length);i++) {
      float x=42+(i%3)*(w+9), y=294+(i/3)*49;
      if(dentro(mouseX,x,w)&&mouseY>=y&&mouseY<=y+41) {
        seleccionado=i; aviso="Puerto seleccionado: "+puertos[i]; return;
      }
    }
    if(entreY(398,440)) {
      if(dentro(mouseX,42,(width-100)/2)) refrescarPuertos();
      else if(dentro(mouseX,52+(width-100)/2,(width-100)/2)) conectar();
    }
    if(entreY(503,545)) {
      float t=(width-104)/4.0f;
      if(dentro(mouseX,40,t)) enviar("MENU");
      else if(dentro(mouseX,48+t,t)) enviar("SCREEN");
      else if(dentro(mouseX,56+2*t,t)) enviar("SELECT");
      else if(dentro(mouseX,64+3*t,t)) enviar("HOME");
    }
  }
}

boolean dentro(float v,float origen,float ancho) { return v>=origen&&v<=origen+ancho; }

boolean entreY(float a,float b) { return mouseY>=a&&mouseY<=b; }

void keyPressed() {
  if(key>='1'&&key<='7') pagina=key-'1';
  else if(key=='i'||key=='I') pagina=3;
  else if(key=='o'||key=='O') pagina=0;
  else if(key=='c'||key=='C') {pagina=5; conectar();}
  else if(key=='p'||key=='P') enviar("PAUSE");
  else if(key=='r'||key=='R') enviar("RESUME");
  else if(key=='a'||key=='A') enviar("ACK");
}

void dispose() { if(arduino!=null) arduino.stop(); }
