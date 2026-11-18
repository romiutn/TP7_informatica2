package info2ambiente;

public final class Medicion {
  public final long secuencia;
  public final float temperatura;
  public final float humedad;
  public final String estado;
  public final int limiteTemperatura;
  public final int limiteHumedad;

  public Medicion(long secuencia, float temperatura, float humedad,
                  String estado, int limiteTemperatura, int limiteHumedad) {
    this.secuencia = secuencia;
    this.temperatura = temperatura;
    this.humedad = humedad;
    this.estado = estado;
    this.limiteTemperatura = limiteTemperatura;
    this.limiteHumedad = limiteHumedad;
  }

  public static Medicion desdeSerial(String linea) {
    if (linea == null) return null;
    String[] p = linea.trim().split(",", -1);
    if (p.length != 7 || !p[0].equals("DATA")) return null;
    try {
      long id = Long.parseLong(p[1]);
      float t = Float.parseFloat(p[2]);
      float h = Float.parseFloat(p[3]);
      int lt = Integer.parseInt(p[5]);
      int lh = Integer.parseInt(p[6]);
      if (id < 0 || !Float.isNaN(h) && (h < 0 || h > 100)) return null;
      if (!p[4].equals("NORMAL") && !p[4].equals("ALERTA") &&
          !p[4].equals("ERROR") && !p[4].equals("ADVERTENCIA") && !p[4].equals("PAUSADO") &&
          !p[4].equals("INICIANDO")) return null;
      return new Medicion(id, t, h, p[4], lt, lh);
    } catch (NumberFormatException ex) { return null; }
  }
}
