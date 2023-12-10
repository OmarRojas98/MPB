#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include <ModbusMaster.h>

//                    Variles de USO

int BAUD = 9600;  // definimos la velocidad de comunicacion del sensor seleccionado, por defoult 9600

// Variables internas del sensor de uso
const int numRegisters = 7;      // Numero de registros que deseas leer
int registerAddresses[] = {0, 3, 7, 20, 26, 30, 40}; // Direcciones de los registros que deseas leer

// Variables donde se guardan los valores de transmitir
float voltaje;
float corriente;
float potencia;
float FP;
float FREQ;
float active_P;



#define MAX485_DE      27     // pines de cotrol del convertidor Serial/ModBus
#define MAX485_RE_NEG  26     //
ModbusMaster node; 

void preTransmission()
{
  digitalWrite(MAX485_RE_NEG, 1);
  digitalWrite(MAX485_DE, 1);
}

void postTransmission()
{
  digitalWrite(MAX485_RE_NEG, 0);
  digitalWrite(MAX485_DE, 0);
}
//                      definiendo las credenciales de usuario de la base de datos de FIREBASE
#define USER_EMAIL "mpbtesis@gmail.com"
#define USER_PASSWORD "mpbtesis123"

                    // definiendo las credenciales de la conexion WIFI
#define WIFI_SSID "FAMILIA_VASQUEZ"
#define WIFI_PASSWORD "Susga2007"

//                    definiendo las credenciales de la base de datos

#define API_KEY "AIzaSyBuQr9PKvRGGbs9INA68TatoFB_y6HjwI8"
#define DATABASE_URL "https://sensortemp-var-default-rtdb.firebaseio.com/"
//#define DATABASE_SECRET "DATABASE_SECRET" // Only If you edit the database rules yourself


//-------------------------------------------------------------- Defininision de FIREBASE ---------------------------------------------------------------

FirebaseData fbdo; // Define the Firebase Data object
FirebaseAuth auth; // Define the FirebaseAuth data for authentication data
FirebaseConfig config; // Define the FirebaseConfig data for config data
// fbdo.setResponseSize(2048); // Limit the size of response payload to be collected in FirebaseData
unsigned long sendDataPrevMillis = 0;

void setup()
{
  Serial.begin(115200);

  pinMode(MAX485_RE_NEG, OUTPUT);
  pinMode(MAX485_DE, OUTPUT);
  // Init in receive mode
  digitalWrite(MAX485_RE_NEG, 0);
  digitalWrite(MAX485_DE, 0);
  Serial2.begin(BAUD, SERIAL_8N1);
  // Modbus slave ID 1
  node.begin(1, Serial2);
  // Callbacks allow us to configure the RS485 transceiver correctly
  node.preTransmission(preTransmission);
  node.postTransmission(postTransmission);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.print("Connecting to Wi-Fi");
  unsigned long ms = millis();
  while (WiFi.status() != WL_CONNECTED)
  {
      Serial.print(".");
      delay(300);
  }

  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();
  Serial.printf("Firebase Client v%s\n\n", FIREBASE_CLIENT_VERSION);


  config.api_key = API_KEY;
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;
  config.database_url = DATABASE_URL;
  Firebase.reconnectNetwork(true);
  Firebase.begin(&config, &auth);
}

void loop()
{ 
  uint8_t result;                  // variable para verificar si la operación de lectura de registros fue exitosa
  uint16_t data[numRegisters];     // Variable que guarda la lectura de los registros 
  if (Firebase.ready() && (millis() - sendDataPrevMillis > 5000 || sendDataPrevMillis == 0)){
    sendDataPrevMillis = millis();
  for (int i = 0; i < numRegisters; i++)
  {
    result = node.readInputRegisters(registerAddresses[i], 1);

    if (result == node.ku8MBSuccess)
    {
      // Guardar el valor del registro en el array data
      data[i] = node.getResponseBuffer(0);
    }
    else
    {
      // Manejar el error si la lectura no tiene éxito
      Serial.println("Error reading register");
    }

    // Esperar un breve periodo de tiempo entre llamadas (ajusta según sea necesario)
    delay(100);
  }
    Serial.println("------------");
    Serial.print("V: ");
    Serial.println(data[0]/10.0f);
    Serial.print("A: ");
    Serial.println(data[1]/100.0f);
    Serial.print("W: ");
    Serial.println(data[2]);
    Serial.print("FP: ");
    Serial.println(data[3]/1000.f);
    Serial.print("FREQ: ");
    Serial.println(data[4]/100.0f);
    Serial.print("T_Active Energy: ");
    Serial.println(data[5]/100.0f);
    Serial.print("Red Active Energy: ");
    Serial.println(data[6]/100.0f);

    float voltaje = data[0]/10.0f;
    float corriente = data[1]/100.0f;
    float potencia =data[2];
    float FP = data[3]/1000.f;
    float FREQ = data[4]/100.0f;


   //objeto JSON con los datos
    FirebaseJson sensorData;
    sensorData.set("Sensor/Corriente", corriente);
    sensorData.set("Sensor/Voltaje", voltaje);
    sensorData.set("Sensor/Potencia", potencia);
    sensorData.set("Sensor/Factor potencia", FP);
    sensorData.set("Sensor/Frecuencia", FREQ);

 // Enviar datos a Firebase
    if(Firebase.RTDB.setJSON(&fbdo, "SensorData", &sensorData)){
      Serial.println("- Successfully saved sensor data to Firebase");
    } else {
      Serial.println("FAILED: " + fbdo.errorReason());
    }
         
    delay(1000); // Wait 2 seconds to update the channel again
  }
}



