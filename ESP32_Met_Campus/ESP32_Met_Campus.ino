#include <DHT.h>                // Biblioteca DHT
#include <SPI.h>                // SSD1396
#include <stdio.h>
#include <stdlib.h>
#include "general.h"
#include <SD.h>


/* Defines do projeto */
#define REED          33    // pin onde o sensor magnetico esta conectado
#define DHTPIN        27    // pin onde dht esta conectado
// #define WindSensor    35    // The pin location of the anemometer sensor
#define VOLTPIN       34    // pin onde o sensor de tensão está conectado
#define CURPIN        32    // pin onde o sensor de corrente está conectado
#define UVPIN         25    // pin onde o sensor uv esta conectado

#define DIAMETRO 125       // diametro interno do balde
#define RAIO     6.25      // raio interno do balde
#define VOLUME   3.05      // volume da bascula (em cm3) (1cm3 == 1ml) (1ml == 1000mm3)

#define DHTTYPE DHT11 // DHT 22
DHT dht(DHTPIN, DHTTYPE);

#define PressaoaNivelDoMar_HPA (1013.25) 
Adafruit_BMP280 bmp;

// Broker defines
#define BROKER_ADDRESS    "150.162.235.160" // "3.133.207.110"
#define BROKER_PORT       8004
#define BROKER_USE_SECURE false


unsigned long lastSend;
unsigned long lastREED;

// Variáveis DHT
float temperatura_lida = 0;
float umidade_lida = 0;

// Variáveis pluviometro:
int val = 0;
int old_val = 0;
volatile unsigned long REEDCOUNT = 0;
float volume_coletado;

// Variable definitions
unsigned int Sample = 0;   // Sample number
unsigned int counter = 0; // magnet counter for sensor
unsigned long RPM = 0;            //Rotações por minuto
float speedwind = 0;             //Velocidade do vento (km/h)
float windspeed = 0;             //Velocidade do vento (m/s)

// Variaveis Anemometro
const float pi = 3.14159265;     //Número de pi
int period = 5000;               //Tempo de medida(miliseconds)
int delaytime = 2000;            //Invervalo entre as amostras (miliseconds)
int radius = 120;                //Raio do anemometro(mm)

volatile unsigned long ContactBounce = 0;

//Variaveis bmp280
float bmp_temp;
float bmp_pressao;
float bmp_altitude;

// Variaveis Sensor de Tensão
float adc_voltage = 0;
float in_voltage = 0;

float R1 = 30000;
float R2 = 7500;

float ref_voltage = 5;
int adc_value = 0;

// Variaveis Sensor de Corrente
int mVperAmp = 185;     //185mV->5A ; 100mV->20A ; 66mV->30A
int Watt = 0;
double Voltage = 0;
double VRMS = 0;
double AmpsRMS = 0;

// Variaveis sensor UV
float sensorValue = 0;

// This is the function that the interrupt calls to increment the turning count
void IRAM_ATTR isr_rain () {
  if ((millis() - ContactBounce) > 0 ) { // debounce the switch contact.
    REEDCOUNT = REEDCOUNT + 1;              // Adiciona 1 à cntagem de pulsos
    // ContactBounce = millis();
    // Serial.println("funcao interrupcao chuva");
  }
}

/*    ME DESCOMENTE PARA TER AS FUNÇÕES DO ANEMOMETRO LIGADAS
void addcount(){
  counter++;
} 

// Measure wind speed
void windvelocity(){
  speedwind = 0;
  windspeed = 0;
  
  counter = 0;  
  attachInterrupt(0, addcount, RISING);
  unsigned long millis();       
  long startTime = millis();
  while(millis() < startTime + period) {
  }
}

//Função para calcular o RPM
void RPMcalc() {
  RPM = ((counter) * 60) / (period / 1000); // Calculate revolutions per minute (RPM)
}

//Velocidade do vento em m/s
void WindSpeed() {
  char str[64];

  windspeed = ((4 * pi * radius * RPM) / 60) / 1000; //Calcula a velocidade do vento em m/s
  
  snprintf(str, sizeof(str), "windspeedm/s= %.2f", windspeed);
  WriteFile("/test.txt", str);
} //end WindSpeed

//Velocidade do vento em km/h
void SpeedWind() {
  char str[64];

  speedwind = (((4 * pi * radius * RPM) / 60) / 1000) * 3.6; //Calcula velocidade do vento em km/h
  
  snprintf(str, sizeof(str), "speedwindkm/h= %.2f", speedwind);
  WriteFile("/test.txt", str);
} //end SpeedWind

    MEU COMENTARIO ACABA AQUI  */   

/*
  SD Card Interface code for ESP32
  CS    = 5;
  SCK   = 18; 
  MOSI  = 23;
  MISO  = 19;
*/

File myFile;
#define CS 5

void WriteFile(const char * path, const char * message){
  // open the file. note that only one file can be open at a time,
  // so you have to close this one before opening another.
  myFile = SD.open(path, FILE_APPEND);
  // if the file opened okay, write to it:
  if (myFile) {
    Serial.printf("Writing to %s ", path);
    myFile.println(message);
    myFile.close(); // close the file:
    Serial.println("completed.");
  } 
  // if the file didn't open, print an error:
  else {
    Serial.println("error opening file ");
    Serial.println(path);
  }
}

void ReadFile(const char * path){
  // open the file for reading:
  myFile = SD.open(path);
  if (myFile) {
     Serial.printf("Reading file from %s\n", path);
     // read from the file until there's nothing else in it:
    while (myFile.available()) {
      Serial.write(myFile.read());
    }
    myFile.close(); // close the file:
  } 
  else {
    // if the file didn't open, print an error:
    Serial.println("error opening test.txt");
  }
}

void get_temp(){
    char str[64];

    temperatura_lida = dht.readTemperature();

    Serial.print("T: ");                    //ESCREVE O TEXTO NO DISPLAY
    Serial.println(temperatura_lida);

    snprintf(str, sizeof(str), "DHTTemp= %.2f", temperatura_lida);
    WriteFile("/test.txt", str);
}

void get_umi(){
    char str[64];
    umidade_lida = dht.readHumidity();  

    Serial.print("U: ");                    //ESCREVE O TEXTO NO DISPLAY   
    Serial.println(umidade_lida);
    
    snprintf(str, sizeof(str), "DHTUmid= %.2f", umidade_lida);
    WriteFile("/test.txt", str);
} 

void get_rain(){
  // isr_rain();
  // float area_recipiente = 3.14159265 * (RAIO * RAIO); // área da seção transversal do recipiente em cm²
  // float volume_por_virada = (VOLUME/area_recipiente);
  // volume_coletad = volume_por_virada * REEDCOUNT * 10;
  char str[64];
  
  volume_coletado = (REEDCOUNT * 0.25) * 10; // volume total coletado em cm³

  Serial.print("Viradas: ");
  Serial.println(REEDCOUNT);

  Serial.print("Chuva: ");
  Serial.print (volume_coletado);
  Serial.println(" mm");

  if(millis() - lastREED > 86400000) { 
    REEDCOUNT = 0;

    lastREED = millis(); 
  }

  snprintf(str, sizeof(str), "Chuva= %.2f", volume_coletado);
  WriteFile("/test.txt", str);
}
void get_voltage() {
  char str[64];

  //Lê o pino analogico do sensor
  adc_value = analogRead(VOLTPIN);

  //Calcula a tensão de entrada ADC
  adc_voltage = (adc_value * ref_voltage) / 1024;

  // Calcula a tensão de entrada com o divisor de tensão
  in_voltage = adc_voltage / (R2/ (R1+R2));

  snprintf(str, sizeof(str), "Voltage= %.2f", in_voltage);
  WriteFile("/test.txt", str);
}

// ** function calls Current Sensor***
float getVPP()
{
  float result;
  int readValue;                // value read from the sensor
  int maxValue = 0;             // store max value here
  int minValue = 4096;          // store min value here ESP32 ADC resolution
  
   uint32_t start_time = millis();
   while((millis()-start_time) < 1000) //sample for 1 Sec
   {
       readValue = analogRead(CURPIN);
       // see if you have a new maxValue
       if (readValue > maxValue) 
       {
           /*record the maximum sensor value*/
           maxValue = readValue;
       }
       if (readValue < minValue) 
       {
           /*record the minimum sensor value*/
           minValue = readValue;
       }
   }
   
   // Subtract min from max
   result = ((maxValue - minValue) * 3.3)/4096.0; //ESP32 ADC resolution 4096
      
   return result;
 }

void get_current(){
  char strVRMS[64];
  char strAmpsRMS[64];
  char strWatt[64];

  Voltage = getVPP();
  VRMS = (Voltage / 2) *0.707;
  AmpsRMS = ((VRMS * 1000) / mVperAmp) - 0.3;    //0.3 é o erro do sensor

  Watt = (AmpsRMS * 240) / 1.2;

  Serial.print("Voltage : ");
  Serial.print(Voltage);
  Serial.println(" V");

  Serial.print("VRMS : ");
  Serial.print(VRMS);
  Serial.println(" V");
 
  Serial.print("AmpsRMS : ");
  Serial.print(AmpsRMS);
  Serial.println(" A");

  Serial.print("Watt : ");
  Serial.print(Watt);
  Serial.println(" W");

  snprintf(strVRMS, sizeof(strVRMS), "VRMS= %.2f", VRMS);
  WriteFile("/test.txt", strVRMS);
  snprintf(strAmpsRMS, sizeof(strAmpsRMS), "AmpsRMS= %.2f", AmpsRMS);
  WriteFile("/test.txt", strAmpsRMS);
  snprintf(strWatt, sizeof(strWatt), "Watt= %.2f", Watt);
  WriteFile("/test.txt", strWatt);
}

 void get_bmptemp(){
  char str[64];
  bmp_temp= bmp.readTemperature();

  Serial.print("Temperatura : ");
  Serial.print(bmp_temp);
  Serial.println(" *C");
  
  snprintf(str, sizeof(str), "BMPTemp= %.2f", bmp_temp);
  WriteFile("/test.txt", str);
 };
 
 void get_bmppres(){
  char str[64];
  bmp_pressao = bmp.readPressure() / 100.0F;

  Serial.print("Pressao : ");
  Serial.print(bmp_pressao);
  Serial.println(" hPa");
  
  snprintf(str, sizeof(str), "BMPPres= %.2f", bmp_pressao);
  WriteFile("/test.txt", str);
 };

 void get_bmpalt(){
  char str[64];
  bmp_altitude = bmp.readAltitude(PressaoaNivelDoMar_HPA);

  Serial.print("Altitude Aproximada : ");
  Serial.print(bmp_altitude);
  Serial.println(" m");

  snprintf(str, sizeof(str), "BMPAlt= %.2f", bmp_altitude);
  WriteFile("/test.txt", str);
 };

void get_uv() {
  char str[64];
  sensorValue = analogRead(UVPIN);//connect UV sensors to Analog 0
  float valuemv =  sensorValue*3300/1023.0;
  
  Serial.print(valuemv);//print the value in mV to serial
  Serial.println("mV");

  snprintf(str, sizeof(str), "UV= %.2f", valuemv);
  WriteFile("/test.txt", str);
}

/*
void get_wind(){
    Sample++;
    Serial.print(Sample);
    Serial.print(": Start measurement...");
    windvelocity();
    Serial.println("   finished.");
    Serial.print("Counter: ");
    Serial.print(counter);
  
    RPMcalc();
    Serial.print("RPM: ");
    Serial.println(RPM);

    WindSpeed();
    Serial.print("WindSpeed [m/s]: ");
    Serial.println(windspeed);

    SpeedWind();
    Serial.print("WindSpeed [km/h]: ");
    Serial.println(speedwind);
}
*/

void send_packets(){
  // Crie um array de bytes para armazenar os dados

  /*FUNÇÃO DE ENVIO PARA O SERVIDOR DO EDUARDO */

  // sprintf(strbuf, "{\"version\" : %d, \"temp\" : %.1f, \"umid\" : %.1f, \"anem\" : %.1f, \"pluv\" : %.1f}", 
  //         FW_VERSION, temperatura_lida, umidade_lida, windspeed, volume_coletado);
  
  sprintf(strbuf, "{\"version\" : %d, \"temp\" : %.1f, \"umid\" : %.1f, \"pluv\" : %.1f, \"bmp_temped\" : %.1f, \"bmp_presed\" : %.1f, \"bmp_alted\" : %.1f, \"voltage\" : %.1f, \"current\" : %.1f}", 
          FW_VERSION, temperatura_lida, umidade_lida, volume_coletado, bmp_temp, bmp_pressao, bmp_altitude, in_voltage, AmpsRMS);
  Serial.println(strbuf);
  client.publish(topic_out, strbuf);


};

void setup() {
  Serial.begin(115200);       // velocidade monitor serial

  initSPIFFS();
  
  if (initWiFi()){
    //MQTT
    server_config();

    client.setServer(BROKER_ADDRESS, BROKER_PORT);
    client.setCallback(on_message);
    if (client.connect("aquisiton")){
      client.subscribe(topic_in);
    }
    wifi_success = true;
  } 
  else {
    wifi_config();
  }

  /* Inicializa sensor de temperatura e umidade relativa do ar */
  dht.begin();
  bmp.begin(0x76);

  Serial.println(mac);

  lastSend = 0;
  lastREED = 0;

  // Inicizalizacao do modulo SD card
  Serial.println("Initializing SD card...");
  if (!SD.begin(CS)) {
    Serial.println("initialization failed!");
    return;
  }
  Serial.println("initialization done.");

  // Configura o BMP
  bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,     /* Operating Mode. */
                Adafruit_BMP280::SAMPLING_X2,     /* Temp. oversampling */
                Adafruit_BMP280::SAMPLING_X16,    /* Pressure oversampling */
                Adafruit_BMP280::FILTER_X16,      /* Filtering. */
                Adafruit_BMP280::STANDBY_MS_500); /* Standby time. */

  // Seta a resolucao do sensor uv em 10 e a atenuacao do sinal
  analogReadResolution(10);
  analogSetPinAttenuation(UVPIN, ADC_11db); //ADC_6db -> ideal.

  // pinMode(WindSensor, INPUT);
  // digitalWrite(WindSensor, HIGH);     //internall pull-up active
  pinMode(REED, INPUT_PULLUP);
  attachInterrupt(REED, isr_rain, FALLING);
}

void loop() {

  // Aquisição de dados dos sensores DHT22 e Pluviometro

  get_rain();

  if (client.connected()) // Verifica conexão com broker e calcula tempo para envio da publicação, tempo em ms
  { 
    if(millis() - lastSend > 60000)
    {
      // get_wind();
      get_temp();
      get_umi();
      get_bmptemp();
      get_bmppres();
      get_bmpalt();
      get_uv();
      get_voltage();
      get_current();
      Serial.println("Sending packet !!!");
      delay(100);
      
      send_packets(); // Envio de pacotes através da rede WiFi pro broker MQTT

      lastSend = millis();   // Atualiza valor atual do contador de ultimo envio
    }
  } 
  else
  {
    reconnect();
  }
  client.loop();

}
