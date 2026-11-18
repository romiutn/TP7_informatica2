package info2ambiente;
import java.io.*;
import java.util.*;

public final class Historial {
  private final File archivo;
  private final int maximo;
  private final ArrayList<String> recientes = new ArrayList<String>();

  public Historial(File archivo, int maximo) throws IOException {
    this.archivo = archivo;
    this.maximo = maximo;
    if (!archivo.exists()) {
      File padre = archivo.getParentFile();
      if (padre != null && !padre.exists() && !padre.mkdirs())
        throw new IOException("No se puede crear carpeta");
      appendRaw("fecha_hora,secuencia,temperatura_c,humedad_pct,estado,limite_t,limite_h");
    }

    BufferedReader br = new BufferedReader(new FileReader(archivo));
    try {
      String linea;
      while ((linea = br.readLine()) != null) {
        if (!linea.startsWith("fecha_hora,")) agregarMemoria(linea);
      }
    } finally { br.close(); }
  }

  private void appendRaw(String texto) throws IOException {
    BufferedWriter bw = new BufferedWriter(new FileWriter(archivo, true));
    try { bw.write(texto); bw.newLine(); }
    finally { bw.close(); }
  }

  private void agregarMemoria(String texto) {
    recientes.add(texto);
    while (recientes.size() > maximo) recientes.remove(0);
  }

  public void guardar(String fechaHora, Medicion m) throws IOException {
    String fila = fechaHora + "," + m.secuencia + "," + m.temperatura + "," +
                  m.humedad + "," + m.estado + "," + m.limiteTemperatura + "," + m.limiteHumedad;
    appendRaw(fila);
    agregarMemoria(fila);
  }

  public void guardarEvento(File eventos, String fechaHora, String evento) throws IOException {
    BufferedWriter bw = new BufferedWriter(new FileWriter(eventos, true));
    try { bw.write(fechaHora + "," + evento); bw.newLine(); }
    finally { bw.close(); }
  }

  public int cantidadEnMemoria() { return recientes.size(); }

  public String[] ultimos() { return recientes.toArray(new String[0]); }

  public String ruta() { return archivo.getAbsolutePath(); }
}
