#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <DHT.h>

#define DHTPIN 4
#define DHTTYPE DHT22

DHT dht(DHTPIN, DHTTYPE);

//API
//Pronostico del tiempo
// openweathermap.org
//Se debe adquirir el API Key del sitio web
// http://api.openweathermap.org/data/2.5/forecast?q=Buga&appid=884ab41f0f1aa01581a9cbde6ac481b2&units=metric&lang=es

//Datos para la conexion a la red WiFi
const char* ssid     = "OcLopez";
const char* password = "1020304050";
const char * host = "dweet.io";

//Ruta para adquirir los datos -> http://api.openweathermap.org/data/2.5/forecast?q=Buga&appid=884ab41f0f1aa01581a9cbde6ac481b2&units=metric
const char* server = "api.openweathermap.org";
const char* recurso = "/data/2.5/forecast?q=Buga&appid=884ab41f0f1aa01581a9cbde6ac481b2&units=metric&lang=es";

//Definicion de una estructura que almacena los datos de cada pronostico
typedef struct Pronostico {
    float temperatura;
    float sensacionTermica;
    float temp_min;
    float temp_max;
    int presion;
    int nivelMar;
    int nivelSuelo;
    int humedad;
    const char* horaFecha;
    const char* clima;
    const char* descripcionClima;
    const char* iconoClima;
    int nubosidad;
    float velocidadViento;
    int direccionViento;
    float lluvia3h;
    const char* ciudad;
} Pronostico;



void setup() {
  Serial.begin(115200);
  delay(10); 
  dht.begin();

  // Iniciamos la conexion a nuestro AP WiFi

  Serial.println();
  Serial.println();
  Serial.print("Conectandose a ");
  Serial.println(ssid);
  
  /* Ajustar explicitamente el ESP8266 como un cliente WiFi, de otro lado, el por defecto
     intentaria actuar como ambos, un cliente y un Access Point.
  */
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi conectado");  
  Serial.println("Direccion IP: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  delay(5000);
//Codigo para realizar la conexion al servidor/  
  Serial.print("Conectandose a ");
  Serial.println(server);
  
  // Utilice la clase WiFiClient para crear conexiones TCP
  WiFiClient client;
  const int httpPort = 80;
  if (!client.connect(server, httpPort)) {
    Serial.println("conexion fallida");
    return;
  }

//Codigo para solicitar el recurso/
  Serial.print("Solicitando la URL: ");
  Serial.println(recurso);
  
  // Esto enviara la peticion al servidor
  client.print(String("GET ") + recurso + " HTTP/1.1\r\n" +
               "Host: " + server + "\r\n" + 
               "Connection: close\r\n\r\n");
  
  //Espere 5 segundos a que se conecte al servidor, sino responde, cierre la conexion
  unsigned long timeout = millis();           //Iniciamos el cronometro
  while (client.available() == 0) {           //Mientras no haya datos recibidos del servidor
    if (millis() - timeout > 5000) {          //Si han transcurrido 5 segundos 
      Serial.println(">>> Expiro el tiempo de recepcion!"); //Se imprime un mensaje de error
      client.stop();                          //Se detiene el cliente
      return;
    }
  }

  // Prueba el status HTTP
  char status[32] = {0};
  client.readBytesUntil('\r', status, sizeof(status));
  // It should be "HTTP/1.0 200 OK" or "HTTP/1.1 200 OK"
  if (strcmp(status + 9, "200 OK") != 0) {
    Serial.print(F("Respuesta inesperada: "));
    Serial.println(status);
    return;
  }

  // Salta la cabecera HTTP
  char endOfHeaders[] = "\r\n\r\n";
  if (!client.find(endOfHeaders)) {
    Serial.println(F("Respuesta invalida"));
    return;
  }


  // Localiza memoria para el documento JSON
  // Use arduinojson.org/v6/assistant para calcular la capacidad. Esto se hace para que no consuma mucha RAM
  //const size_t capacity = 40*JSON_ARRAY_SIZE(1) + JSON_ARRAY_SIZE(40) + 116*JSON_OBJECT_SIZE(1) + 41*JSON_OBJECT_SIZE(2) + 40*JSON_OBJECT_SIZE(4) + JSON_OBJECT_SIZE(5) + 4*JSON_OBJECT_SIZE(7) + 37*JSON_OBJECT_SIZE(8) + 40*JSON_OBJECT_SIZE(9) + 9590;
  DynamicJsonDocument doc(24576);
  
  // Analizar el objeto JSON
  DeserializationError error = deserializeJson(doc, client);
  if (error) {
    Serial.print(F("deserializeJson() fallido: "));
    Serial.println(error.c_str());
    return;
  }

  Pronostico pronostico;
  JsonArray list = doc["list"];

  for(char index=0; index<=3;index++){
    pronostico.temperatura = list[index]["main"]["temp"]; // 299.27
    pronostico.sensacionTermica = list[index]["main"]["feels_like"]; // 299.98
    pronostico.temp_min = list[index]["main"]["temp_min"]; // 297.85
    pronostico.temp_max = list[index]["main"]["temp_max"]; // 299.27
    pronostico.presion = list[index]["main"]["pressure"]; // 1012
    pronostico.nivelMar = list[index]["main"]["sea_level"]; // 1012
    pronostico.nivelSuelo = list[index]["main"]["grnd_level"]; // 905
    pronostico.humedad = list[index]["main"]["humidity"]; // 51
    pronostico.horaFecha = list[index]["dt_txt"];
    pronostico.clima = list[index]["weather"][0]["main"]; // "Rain"
    pronostico.descripcionClima = list[index]["weather"][0]["description"]; // "moderate rain"
    pronostico.iconoClima = list[index]["weather"][0]["icon"]; // "10d"
    pronostico.nubosidad = list[index]["clouds"]["all"]; // 95
    pronostico.velocidadViento = list[index]["wind"]["speed"]; // 1.39
    pronostico.direccionViento = list[index]["wind"]["deg"]; // 309
    pronostico.lluvia3h = list[index]["rain"]["3h"]; // 3.08  
    pronostico.ciudad = doc["city"]["name"];

    Serial.println("Pronostico para " + String(pronostico.ciudad) + " Hora/fecha: " + String(pronostico.horaFecha));
    Serial.println("Temperatura: " + String(pronostico.temperatura) + String(" °C"));
    Serial.println("Presion: " + String(pronostico.presion) + String(" hPa"));
    Serial.println("Humedad: " + String(pronostico.humedad) + String(" %"));
    Serial.println("Sensacion Termica: " + String(pronostico.sensacionTermica) + String(" °C"));
    Serial.println("Temperatura minima: " + String(pronostico.temp_min) + String(" °C"));
    Serial.println("Temperatura maxima: " + String(pronostico.temp_max) + String(" °C"));
    Serial.println("Presion atmosferica al nivel del mar: " + String(pronostico.nivelMar) + String(" hPa"));
    Serial.println("Presion atmosferica al nivel del suelo: " + String(pronostico.nivelSuelo) + String(" hPa"));
    Serial.println("Clima: " + String(pronostico.clima));
    Serial.println("Descripcion clima: " + String(pronostico.descripcionClima));
    Serial.println("Icono: " + String(pronostico.iconoClima));
    Serial.println("Nubosidad: " + String(pronostico.nubosidad) + String(" %"));
    Serial.println("Velocidad Viento: " + String(pronostico.velocidadViento) + String(" m/s"));
    Serial.println("Direccion Viento: " + String(pronostico.direccionViento) + String("°"));
    Serial.println("Lluvia 3h: " + String(pronostico.lluvia3h) + String(" mm"));
    delay(1000);
  }
  Serial.println();
  Serial.println("Cerrando la conexion");               
  // Desconectarse
  client.stop();


  WiFiClient cliente;

  const int puerto = 80;

  if(!cliente.connect(host, puerto)){
    Serial.println("Conexion fallida");
    delay(10000);
    return;
  }

  delay(2000);

delay(2000);
  float humedad = dht.readHumidity();
  float temperatura = dht.readTemperature();
  float temperaturaf = dht.readTemperature(true);
  float indice = dht.computeHeatIndex(temperatura, humedad);
  //Verificamos las lecturas del sensor: NaN : Not a Number (no es un numero)

  if(isnan(humedad)||isnan(temperatura)||isnan(temperaturaf)){
    Serial.println("Fallo la lectura del sensor");
    return;
  }

  Serial.print("Indice de calor: ");
  Serial.print(indice);
  Serial.print("°C");

  Serial.print(" |  Humedad: ");
  Serial.print(humedad);

  Serial.print(" |  Temperatura: ");
  Serial.print(temperatura);
  Serial.print("°C");

  Serial.print(" |  Temperatura Farenheit: ");
  Serial.print(temperaturaf);
  Serial.println("°F");

  String url = "/dweet/for/uceva_alejandro?indiceactu="+String(indice)+"&humedadactu="+String(humedad)+"&tempactu="+String(temperatura)+"&tempmin="+String(pronostico.temp_min)+"&tempmax="+String(pronostico.temp_max)+"&humedad="+String(pronostico.humedad)
  +"&clima="+String(pronostico.clima)+"&velocidadv="+String(pronostico.velocidadViento)
  +"&presion="+String(pronostico.presion)+"&stermica="+String(pronostico.sensacionTermica)+"&temperatura="+String(pronostico.temperatura)
  +"&nubosidad="+String(pronostico.nubosidad)+"&direccionv="+String(pronostico.direccionViento);

  Serial.print("Solicitando el recursos: ");
  Serial.println(url);

  /*
  Esta funcion realiza la conexion con google.com y le solicita la pagina index.html por medio de este texto que hace parte del protocolo http
   GET /dweet/for/uceva_alejandro?temperatura=24.38&humedad=50.1&indice=23 HTTP/1.1
   Host: dweet.io
   connection: close
  */

  cliente.print(String("GET ")+ url +" HTTP/1.1\r\n" + "Host: "+ host + "\r\n" + "connection: close\r\n\r\n");

  unsigned long tiempoinicial = millis();  // Miro el reloj
  while(cliente.available()==0){
    if(millis()- tiempoinicial > 5000){   // Reviso si ya pasaron mas de los 5 segundos
      Serial.println("Expiro el tiempo de espera");
      cliente.stop();                     // Detengo la conexion a google
      return;
    }
  }

  while(cliente.available()){
    String linea = cliente.readStringUntil('\r');
    Serial.println(linea);
  }
  delay(20000);
  Serial.println("Fin de la conexion");
  cliente.stop();

}