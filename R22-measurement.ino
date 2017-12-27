#include <ESP8266WiFi.h>
#include <EEPROM.h>

volatile int numPulsos;  // Mide la cantidad de pulsos recibidos del sensor
int pinSensor = D2;  // Define el Pin conectado al sensor
int frecuencia, frecuenciaMax;
const float factorConv = 10;  // Para convertir de frecuencia a flujo volumétrico
float calc = 0, volumen = 0, masa = 0, densidad = 1.25, volumenAux=0, masaAux=0;  // densidad de R22 líquido a 10 ºC
long dt = 0;  // Variación de tiempo por cada ciclo
long t0 = 0;  // Instante de tiempo de inicio del ciclo

const char* ssid = "xxxxxxxx";
const char* password = "xxxxxxxx";

WiFiClient client;

const int channelID = 00000000;  // Configuración del Canal ThingSpeak
String writeAPIKey = "xxxxxxxx";  // Llave de Escritura ThingSpeak
const char* server = "api.thingspeak.com";  // Dirección de la API ThingSpeak


void ContarPulsos ()  // Función llamada cuando el sensor registra una interrupción
{ 
  numPulsos++;  // Contador de pulsos que registra el sensor en cada ciclo
}

int ObtenerFrecuencia() 
{
  numPulsos = 0;
  interrupts();  // Habilita las interrupciones
  delay(1000);  // Muestra de 1 segundo
  noInterrupts();  // Deshabilita las interrupciones
  frecuencia = numPulsos;
  return frecuencia;
}

void setup() 
{
  Serial.begin(115200);
  Serial.println();
  Serial.print("Conectandose a red: ");
  Serial.println(ssid);
  
  WiFi.begin(ssid, password);            
  while (WiFi.status() != WL_CONNECTED)  // Ciclo de espera de conexión a la Red WiFi
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("Conectado, Dirección IP: ");
  Serial.println(WiFi.localIP());

  EEPROM.begin(512);
  
  EEPROM.get(0,masa); // (dirección inicial, variable) Lee de la memoria EEPROM tantas direcciones ocupe su tipo y guarda esos valores en la variable
  volumen = masa / densidad;
  pinMode(pinSensor, INPUT); 
  attachInterrupt(digitalPinToInterrupt(pinSensor),ContarPulsos,FALLING);  // Configura las interrupciones
  t0 = millis();

}

void loop ()    
{
  frecuencia = ObtenerFrecuencia();  // Obtiene la frecuencia de los pulsos en Hz
  dt = millis() - t0;  // Calcula la variación de tiempo
  volumenAux = (calc/60) * (dt/1000); // volumen(L) = flujo(L/s) * tiempo(s)
  masaAux = densidad * volumenAux;
  volumen = volumen + volumenAux;
  t0 = millis();
  masa = densidad * volumen;

  if (calc != 0 && calc < 5)  // Si el sensor de flujo registra una nueva medición
  { 
    EEPROM.put(0,masa); //  (dirección inicial, variable) Escribe en la memoria EEPROM el valor de la variable en tantas direciones ocupe su tipo
    EEPROM.commit(); // Confirma la escritura datos en la memoria EEPROM
  }
  
  Serial.println();
  Serial.print("Masa Acumulada: ");
  Serial.print(masa,2);
  Serial.print(" Kg     ");
  
  if (client.connect(server, 80)) // Si hay conexión con la API de ThingSpeak 
  {
    String body = "field1=";  // Construye el cuerpo de la solicitud y envía los datos a la API
           body += String(masa);

    client.print("POST /update HTTP/1.1\n");
    client.print("Host: api.thingspeak.com\n");
    client.print("Connection: close\n");
    client.print("X-THINGSPEAKAPIKEY: " + writeAPIKey + "\n");
    client.print("Content-Type: application/x-www-form-urlencoded\n");
    client.print("Content-Length: ");
    client.print(body.length());
    client.print("\n\n");
    client.print(body);
    client.print("\n\n");
   }
    client.stop();  // Se desconecta del servidor ("api.thingspeak.com")
}

