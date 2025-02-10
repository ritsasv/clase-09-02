#include <Adafruit_Fingerprint.h>
#include <SoftwareSerial.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Configuración de SoftwareSerial para los pines 2 y 3
SoftwareSerial mySerial(2, 3); // RX, TX
Adafruit_Fingerprint finger(&mySerial);

// Configuración de la pantalla OLED
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire);

// Configuración de usuarios
const int MAX_USUARIOS = 3;
const int MAX_LONGITUD_NOMBRE = 20;  // Máximo de caracteres por nombre

char nombres[MAX_USUARIOS][MAX_LONGITUD_NOMBRE] = { "" };

void setup() {
  Serial.begin(9600);
  while (!Serial);
  Serial.println("S");

  Wire.begin();
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("E");
    while (1);
  }
    // *Mostrar mensaje inicial*
  display.clearDisplay();
  mostrarTexto("R", 20, 10);
  Serial.println("R");
  delay(3000); // Mantener el mensaje 3 segundos

  display.ssd1306_command(SSD1306_DISPLAYOFF);
  display.clearDisplay();
  display.display();

  // Iniciar el lector de huellas
  mySerial.begin(57600);
  finger.begin(57600);

  if (finger.verifyPassword()) {
    Serial.println("c");
  } else {
    Serial.println("E");
    while (1);
  }
}

void loop() {
  identificarHuella();  // Activar la identificación de huellas
  if (Serial.available()) {
    Serial.println("1");
    Serial.println("2");
    Serial.println("3");
    Serial.println("4");
    char opcion = '.';
    while (Serial.available()) {
      char c = Serial.read();
      if (c != '\n' && c != '\r') {  // Ignorar '\n' y '\r'
        opcion = c;
        break;  // Salir del bucle después de leer una opción válida
      }
    }
    Serial.println("------");
    Serial.println(opcion);
    switch (opcion) {
      case '1':
        registrarHuella();
        delay(3000);
        break;
      case '2':
        borrarHuella();
        break;
      case '3':
        mostrarHuellas();
        break;
      case '4':
        Serial.println("...");
        while (1);
        break;
      default:
        Serial.println("no válida");
        break;
    }
    
    while (Serial.available()) {
      Serial.read();  // Leer y descartar cualquier dato residual
    }

    delay(1000);

  }
}

void mostrarTexto(String texto, int x, int y) {
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(x, y);
  display.print(texto);
  display.display();
}

void registrarHuella() {
  int id = buscarIDDisponible();
  if (id == -1) {
    Serial.println("No");
    return;
  }

  Serial.print("Registrando nueva huella con ID: ");
  Serial.println(id);
  Serial.println("Introduce el nombre:");

  char nombre[MAX_LONGITUD_NOMBRE] = "";
  int indice = 0;

  unsigned long tiempoInicio = millis();  // Iniciar temporizador

  // Capturar el nombre desde el Serial
  while ((millis() - tiempoInicio) < 10000) {  // Esperar hasta 10 segundos para la entrada
    if (Serial.available()) {
      char c = Serial.read();
      
      // Filtrar saltos de línea y retorno de carro
      if (c == '\n' || c == '\r') {
        if (indice > 0) break;  // Finaliza si ya hay caracteres ingresados
        else continue;          // Si no hay caracteres, sigue esperando
      }
      
      // Almacenar el carácter si hay espacio
      if (indice < MAX_LONGITUD_NOMBRE - 1) {
        nombre[indice++] = c;
      }
      
      tiempoInicio = millis();  // Reiniciar temporizador al recibir un carácter
    }
  }
  
  Serial.println(nombre);
  nombre[indice] = '\0';  // Terminar la cadena con null

  strcpy(nombres[id], nombre);  // Copiar el nombre al arreglo
  if (capturarHuella(id)) {
    Serial.print("Huella registrada exitosamente como ");
    Serial.println(nombres[id]);
  } else {
    Serial.println("Error al registrar la huella.");
  }
}

bool capturarHuella(int id) {
  Serial.println("Coloca el dedo en el sensor para el primer escaneo.");
  if (!esperarHuella(1)) return false;

  Serial.println("Levanta el dedo.");
  delay(2000);

  Serial.println("Coloca el mismo dedo nuevamente para el segundo escaneo.");
  if (!esperarHuella(2)) return false;

  if (finger.createModel() == FINGERPRINT_OK) {
    if (finger.storeModel(id) == FINGERPRINT_OK) {
      return true;
    }
  }
  return false;
}

bool esperarHuella(uint8_t posicion) {
  for (int intentos = 0; intentos < 5; intentos++) {
    int resultado = finger.getImage();
    if (resultado == FINGERPRINT_OK) {
      if (finger.image2Tz(posicion) == FINGERPRINT_OK) {
        return true;
      }
    } else if (resultado == FINGERPRINT_NOFINGER) {
      delay(1000);
    }
  }
  return false;
}

void identificarHuella() {
  int resultado = finger.getImage();
  if (resultado != FINGERPRINT_OK) return;

  resultado = finger.image2Tz();
  if (resultado != FINGERPRINT_OK) return;

  if (finger.fingerFastSearch() == FINGERPRINT_OK) {
    int id = finger.fingerID;
    Serial.print("Huella reconocida: ");
    mySerial.begin(57600);
    display.ssd1306_command(SSD1306_DISPLAYON);
    display.clearDisplay();
    display.display();
    display.clearDisplay();
    mostrarTexto("Huella reconocida", 20, 10);
    delay(1000);
    if (strlen(nombres[id]) > 0) {
      Serial.println(nombres[id]);
      display.clearDisplay();
      mostrarTexto(nombres[id], 20, 10);
    } else {
      Serial.println("sin nombre asociado.");
    }
    delay(2000);
    mySerial.end();
    display.ssd1306_command(SSD1306_DISPLAYOFF);
    display.clearDisplay();
    display.display();
    display.flush();
    finger.begin(57600);

  }
}

void borrarHuella() {
  Serial.println("Introduce el ID de la huella que deseas borrar:");
  while (!Serial.available());
  int id = Serial.parseInt();
  Serial.read(); // Limpiar el buffer del salto de línea

  if (finger.deleteModel(id) == FINGERPRINT_OK) {
    Serial.print("Huella con ID ");
    Serial.print(id);
    Serial.println(" borrada exitosamente.");
    nombres[id][0] = '\0';  // Vaciar el nombre asociado
  } else {
    Serial.println("Error al borrar la huella.");
  }
}

void mostrarHuellas() {
  Serial.println("Huellas registradas:");
  for (int i = 0; i < MAX_USUARIOS; i++) {
    if (strlen(nombres[i]) > 0) {
      Serial.print("ID: ");
      Serial.print(i);
      Serial.print(" - Nombre: ");
      Serial.println(nombres[i]);
    }
  }
}

int buscarIDDisponible() {
  for (int i = 0; i < MAX_USUARIOS; i++) {
    if (strlen(nombres[i]) == 0) {
      return i;
    }
  }
  return -1;
}
