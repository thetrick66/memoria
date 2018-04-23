#include <DallasTemperature.h>
#include <OneWire.h>

#include <SoftwareSerial.h>
#include <string.h>
SoftwareSerial mySerial(3, 2); /* RX, TX */

#define MAX_STRING_LEN 50 /* Máximo largo a leer desde Serial */
#define ECG_SIZE 20       /* Cantidad de datos por envio para ECG */
#define CANTIDADT 100     /* Cantidad de datos de T a promediar */
#define FRECUENCIAECG 300 /* Frecuencia ECG aproximada al entero mas cercano */
#define ONE_WIRE_BUS 5

OneWire oneWire(ONE_WIRE_BUS);

DallasTemperature sensors(&oneWire);

boolean test = true;

char  buffer_char[MAX_STRING_LEN]; /* Almacenamiento máximo para lectura desde Serial */

String UUID_S = "66ecf52ce19f11e48a001681e6b88ec1";   /* Random UUID */
String UUID_ECG = "66ecf194e19f11e48a001681e6b88ec2"; /* Random UUID */
String UUID_T = "f6327dc6a8144e14b68036cf626cba58";   /* Random UUID */
String UUID_C = "962e8b4bedfc4688ac09dd8476ed2dc6"; /* Random UUID */

const int ECGPin = A1;    /* Pin para ECG */
const int TPin = A5;      /* Pin para T */


const int tiempoAntirebote = 10; /*tiempo antirebote*/
int estadoBoton;
int estadoBotonAnterior;


int ct = 0;
int subbuf;
String Hex = "";
boolean sub = false;
boolean detenerEnvio = false;
boolean iniciarEnvio = true;
int periodoECG = 0;
int periodoT = 0;
int totalesECG = 0;
int totalesT = 0;
boolean enviarECG = false;/* Usada para activar/Desactivar envio ECG data */
boolean enviarT = false;  /* Usada para activar/Desactivar envio T data */
int contadorECG = 0;      /* Usada para contar datos a enviar de ECG */
int contadorT = 0;        /* Usada para contar datos a enviar de T */
int TValue = 0;
String sensor = "00";

int veces = 0;            /* Usada para repetir una cantidad x de envíos */
String stringValueECG = "";  /* Usado para almacenar valores antes de un envío de ECG */
String stringValueT = "";  /* Usado para almacenar valores antes de un envío de T */
int T = 0;                /* Usado para almacenar los valores random de T en testing */
String randomTS = "";     /* String asociado a la T random por testing*/
unsigned int tiempo = 0;  /* Control de tiempo minucioso */


boolean antirebote  (int pin) {
  int  contador = 0;
  boolean estado;             // guarda el estado del boton
  boolean estadoAnterior;     // guarda el ultimo estado del boton

  do {
    estado = digitalRead (pin);
    if (estado != estadoAnterior ){  // comparamos el estado actual
      contador = 0;                   // reiniciamos el contador
      estadoAnterior = estado;
    }
    else{
      contador = contador +1;       // aumentamos el contador en 1
    }
    delay (1);
  }
  while (contador < tiempoAntirebote);
  return estado;
}


void setup() {
  periodoECG = 9000/FRECUENCIAECG;
  periodoT = 5500/CANTIDADT;
  Serial.begin(19200);
  while (! Serial);       /* No comenzar hasta que el canal Serial se encuentre disponible */
  mySerial.begin(19200);  /* Valores probados: 2400 9600 19200, defecto: 115200 */
  while (! mySerial);     /* No comenzar hasta que el canal Serial se encuentre disponible */
  

  mySerial.println("WC");delay(100);        /* Limpiar cualquier Script de la memoria */
  mySerial.println("ww");delay(100);        /* Limpiar cualquier Script de la memoria */
  mySerial.println("@PW_ON");delay(10);     /* Activar enviando un 'WR,0' por mySerial */
  mySerial.println("SF,1");delay(10);       /* Reset */
  mySerial.println("S-,Galeno1");delay(10); /* Nombre del Dispositivo */
  mySerial.println("SB,2");delay(10);       /* Baudios a utilizar */
  mySerial.println("SP,4");delay(10);       /* Potencia de Tx */
  mySerial.println("SR,24062000");delay(10);/* Características permitidas */
  mySerial.println("SS,00000001");delay(10);/* Tipo de Servicios (privado) */
  mySerial.println("R,1");delay(10);        /* Reiniciar */
  mySerial.println("WP");delay(100);        /* Detener Ejecución Script */
  mySerial.println("ww");delay(100);        /* Comenzar a agregar a Script */
  mySerial.println("@CONN");delay(10);      /* Activar enviando un 'WR,3' por mySerial */
  mySerial.println("B");delay(10);          /* Pedir vincular con dispositivo conectado */
  mySerial.println("WP");delay(100);        /* Detener Ejecución Script */
  mySerial.println("ww");delay(100);        /* Comenzar a agregar a Script */
  mySerial.println("@DISCON");delay(10);    /* Activar enviando un 'WR,3' por mySerial */
  mySerial.println("A,0064,2710");delay(10);/* Activar reconocimiento cada 100 ms por 10 s */
  mySerial.println("WP");delay(100);        /* Detener Ejecución Script */
  mySerial.println("\x1B");delay(100);      /* Inserta ESC para finalizar escritura de Script */


  
  mySerial.println("PZ");delay(100); /* Limpiar servicios privados */

  /* Servicio principal contenedor de las características */
  mySerial.println("PS,"+UUID_S);delay(100);                /*PS,UUID*/

  /* ECG Sensor */
  mySerial.println("PC,"+UUID_ECG+",10,14,01");delay(100);  

  /* Temperature Sensor */
  mySerial.println("PC,"+UUID_T+",10,03,01");delay(100);

  /* Control */
  mySerial.println("PC,"+UUID_C+",04,01,01");delay(100);

  mySerial.println("K");delay(100); /* Desconectarse al inicio */


  /* Valores iniciales para las características de ECG y T */
  mySerial.println("SUW,"+UUID_ECG+",00000000000000000000000000000000000000F0");
  mySerial.println("SUW,"+UUID_T+",0000F0");

}

void loop() {
  if (tiempo == 10000){
    tiempo = 0;
  }
  tiempo = tiempo + 1;


    /* TOMA DE DATOS ECG */
  if (enviarECG && tiempo%periodoECG == 0){
    int ECGValue = analogRead(ECGPin);
    ECGValue = ECGValue/4;
    if(ECGValue>16){
      if (test)stringValueECG = stringValueECG + String(150, HEX);
      else stringValueECG = stringValueECG + String(ECGValue, HEX);
      contadorECG = contadorECG + 1;
    }
  }

  /* ENVIO DE DATOS ECG */
  
  if (enviarECG && contadorECG == 20){
      totalesECG = totalesECG + 1;
      mySerial.println("SUW,"+UUID_ECG+","+stringValueECG);
      stringValueECG = "";
      contadorECG = 0;
  }

  /* TOMA DE DATOS T */
  
  if (enviarT && tiempo%periodoT == 0){
    contadorT = contadorT + 1;
  }
  
  if(enviarT && contadorT == CANTIDADT){
    sensors.requestTemperatures();
    TValue = (sensors.getTempCByIndex(0)+2.5)*100;
    if(test) TValue = 3700;
    

    if(TValue>16){
      totalesT = totalesT + 1;
      stringValueT = String(TValue,HEX);
      while (stringValueT.length() < 6) stringValueT = "0" + stringValueT;
      mySerial.println("SUW,"+UUID_T+","+stringValueT);
    }
    stringValueT = "";
    T=0;
    contadorT = 0;
  }


  int MavailableBytes = mySerial.available();
  if (MavailableBytes > 1){
    for(int i=0; i<MavailableBytes; i++)
    {
      char c = mySerial.read();
      buffer_char[i] = c;
      if (i == MavailableBytes-1){
        buffer_char[i+1] = '\0';
        }
     }
     int len = strlen(buffer_char);
     if (len == 3){
      sub = true;
      subbuf = len-3;
      buffer_char[len-1]='\0';
      }
     else if (len == 4 || len==5 || len==7){
      sub = true;
      subbuf = len-4;
      buffer_char[len-2]='\0';
     }
     else if(len==6){
      sub = true;
      subbuf = len-2;
      }
     if(sub){      
      if(detenerEnvio && strcmp(&buffer_char[subbuf], "00") == 0){
        periodoT = 5500/CANTIDADT;
        Serial.println("Apagar ambos!");
        iniciarEnvio = true;
        enviarECG = false;
        enviarT = false;
        Hex = String(totalesECG, HEX);
        while (Hex.length() < 4) Hex = "0" + Hex;
        mySerial.println("SUW,"+UUID_ECG+",FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF" + Hex);
        totalesECG = 0;
        //delay(10);
        Hex = String(totalesT, HEX);
        while (Hex.length() < 4) Hex = "0" + Hex;
        mySerial.println("SUW,"+UUID_T+",FF"+Hex);
        totalesT = 0;
        detenerEnvio = false;
        }
      else if(iniciarEnvio && strcmp(&buffer_char[subbuf], "11") == 0){
        sensor = "11";
        periodoT = periodoT/3;
        Serial.println("Encender ambos!");
        mySerial.println("SUW,"+UUID_ECG+",AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"+sensor);
        mySerial.println("SUW,"+UUID_T+",AAAA"+sensor);
        iniciarEnvio = false;
        enviarECG = true;
        enviarT = true;
        detenerEnvio = true;
      }
      else if(iniciarEnvio && strcmp(&buffer_char[subbuf], "10") == 0){
        sensor = "10";
        Serial.println("Encender ECG!");
        mySerial.println("SUW,"+UUID_ECG+",AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"+sensor);
        iniciarEnvio = false;
        enviarECG = true;
        enviarT = false;
        detenerEnvio = true;
      }
      else if(iniciarEnvio && strcmp(&buffer_char[subbuf], "01") == 0){
        sensor = "01";
        ct = ct + 1;
        if(ct == 2){
          ct = 0;
          Serial.println("Encender T!");
          mySerial.println("SUW,"+UUID_T+",AAAA"+sensor);
          iniciarEnvio = false;
          enviarECG = false;
          enviarT = true;
          detenerEnvio = true;
        }
      }
      }
     sub = false;
  }
  
  int SavailableBytes = Serial.available();
  if (SavailableBytes > 0){
    for(int i=0; i<SavailableBytes; i++)
    {
      char c = Serial.read();
      buffer_char[i] = c;
      if (i == SavailableBytes-1){
        buffer_char[i+1] = '\0';
        }
    }
    mySerial.print(buffer_char);
  }
  
  if (tiempo%3000 == 0){
    mySerial.print("SUR,"+UUID_C+"\n\0");
  }
  delayMicroseconds(100);
}
