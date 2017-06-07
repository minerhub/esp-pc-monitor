#include <dummy.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

const String* WLAN_SSID = new String("WLAN_PRIVATE");
const String* WLAN_PASS = new String("belennereaTKM_432");

const char* www_username = "miner01";
const char* www_password = "minerTKM_432";

const long POWER_MONITOR_TIME = 1000 * 60 * 5;

const int pwrPin = D4;
const int resetPin = D5;

const int pwrBtnPin = D3;
const int resetBtnPin = D6;

int reset = 0;
int power = 0;
int powerMonitorEnabled = 0;
long powerMonitorUpdate = 0;

ESP8266WebServer server(80);

void commandPowerOn() {
  digitalWrite(pwrPin, LOW);
  delay(500);
  digitalWrite(pwrPin, HIGH);
  Serial.println("Ordenador encendido.");
}

void commandPowerOff() {
  digitalWrite(pwrPin, LOW);
  delay(6000);
  digitalWrite(pwrPin, HIGH);
  Serial.println("Ordenador apagado.");
}

void commandReboot() {
  digitalWrite(resetPin, LOW);
  delay(500);
  digitalWrite(resetPin, HIGH);
  powerMonitorUpdate = 0;
  Serial.println("Ordenador reiniciado.");
}

bool connectWLAN(bool force) { connectWLAN(force, NULL, NULL); }

bool connectWLAN(bool force, const String* ssid, const String* password) {
  static String* lastSsid = new String(WLAN_SSID->c_str());
  static String* lastPass = new String(WLAN_PASS->c_str());

  if (ssid != NULL) lastSsid = new String(ssid->c_str());
  if (password != NULL) lastPass = new String(password->c_str());

  if (WiFi.status() != WL_CONNECTED || force) {
    WiFi.disconnect();
    
    Serial.print("Connecting to ");
    Serial.print(lastSsid->c_str());
    Serial.println("...");
    WiFi.mode(WIFI_STA);
    WiFi.begin(lastSsid->c_str(), lastPass->c_str());
    
    int count = 100;
    while (WiFi.status() != WL_CONNECTED && count > 0) {
      delay(100);
      count--;
    }

    if (count <= 0) {
      Serial.println("Error. Connection timeout...");
      return false;
    }
    else {
      Serial.print("Connected to ");
      Serial.print(lastSsid->c_str());
      Serial.print("! (");
      Serial.print(WiFi.localIP());
      Serial.println(")");
    }
  }

  return true;
}

void setup() {
  Serial.begin(9600);
  
  pinMode(pwrPin, OUTPUT);
  pinMode(resetPin, OUTPUT);
  digitalWrite(pwrPin, HIGH);
  digitalWrite(resetPin, HIGH);

  pinMode(pwrBtnPin, INPUT_PULLUP);
  pinMode(resetBtnPin, INPUT_PULLUP);
  
  connectWLAN(true);
  
  server.on("/", [](){
    if(!server.authenticate(www_username, www_password)) { return server.requestAuthentication(); }
    server.send(200, "text/html", printHome());
  });
  
  server.on("/command/poweron", [](){
    if(!server.authenticate(www_username, www_password)) { return server.requestAuthentication(); }
    commandPowerOn();
    server.send(200, "text/html", redirectHome("Señal de encendido enviada."));
  });
  
  server.on("/command/poweroff", [](){
    if(!server.authenticate(www_username, www_password)) { return server.requestAuthentication(); }
    commandPowerOff();
    server.send(200, "text/html", redirectHome("Señal de apagado enviada."));
  });
  
  server.on("/command/reboot", [](){
    if(!server.authenticate(www_username, www_password)) { return server.requestAuthentication(); }
    commandReboot();
    server.send(200, "text/html", redirectHome("Señal de reinicio enviada."));
  });
  
  server.begin();
  
  attachInterrupt(digitalPinToInterrupt(pwrBtnPin), commandPowerOn, FALLING);
  attachInterrupt(digitalPinToInterrupt(resetBtnPin), commandReboot, FALLING);
}

String redirectHome(const char* message) {
  String html = "";
  html.concat("<!DOCTYPE html>"
    "<html>"
    "  <head>"
    "    <meta charset=\"UTF-8\">"
    "    <meta http-equiv=\"refresh\" content=\"3; url=/\" />"
    "  </head>"
    "  <body>");
  html.concat(message);
  html.concat("  </body><html>");
  return html;
}

String printHome() {
  String html = "";
  html.concat("<!DOCTYPE html>"
    "<html>"
    "  <head>"
    "   <meta charset=\"UTF-8\">"
    "  </head>"
    "  <body>"
    "    <h2>Monitor de RIG-01</h2>");

  if (powerMonitorEnabled) {
    html.concat("  <small style=\"color: blue;\">(Ultima actualización hace ");
    html.concat(((millis() - powerMonitorUpdate) / 1000));
    html.concat(" segundos)</small>");
  }
  else {
    html.concat("<small style=\"color: red;\">(Monitor de actividad inactivo)</small>");
  }
  
  html.concat("    <ul>"
    "      <li><a href=\"/command/poweron\">Encender ordenador</a></li>"
    "      <li><a href=\"/command/poweroff\">Apagar ordenador</a></li>"
    "      <li><a href=\"/command/reboot\">Reiniciar ordenador</a></li>"
    "    </ul>"
    "  </body>"
    "<html>\n");

  return html;
}

char** str_split(char* a_str, const char a_delim)
{
    char** result    = 0;
    size_t count     = 0;
    char* tmp        = a_str;
    char* last_comma = 0;
    char delim[2];
    delim[0] = a_delim;
    delim[1] = 0;

    /* Se cuenta el numero de elementos */
    while (*tmp)
    {
        if (a_delim == *tmp)
        {
            count++;
            last_comma = tmp;
        }
        tmp++;
    }

    /* Se añade espacio para el token */
    count += last_comma < (a_str + strlen(a_str) - 1);

    /* Añadir espacio strings terminados en null para saber donde terminan los strings. */
    count++;

    result = (char**)malloc(sizeof(char*) * count);

    if (result)
    {
        size_t idx  = 0;
        char* token = strtok(a_str, delim);

        while (token)
        {
            if (!(idx < count)) abort();
            *(result + idx++) = strdup(token);
            token = strtok(0, delim);
        }
        if (!(idx == count - 1)) abort();
        *(result + idx) = 0;
    }

    return result;
}

void handleSerialCommand() {
  static String* cmdBuffer = new String("");
  
  if (Serial.available() > 0) {
    String* command = NULL;

    // Metemos datos en el buffer
    cmdBuffer->concat(Serial.readString());

    // Parseamos los datos para localizar comandos
    int endPosition = cmdBuffer->indexOf('}');
    if (endPosition != -1) {
      command = new String(cmdBuffer->substring(cmdBuffer->indexOf('{'), endPosition + 1));
      cmdBuffer = new String(cmdBuffer->substring(endPosition + 1));
    }
     
    if (command != NULL) {
      String cleanCommand = command->substring(1, command->length() - 1);
      char** commandParts = str_split(const_cast<char*>(cleanCommand.c_str()), '|');

      if (*commandParts) {
        String comm = String(*commandParts);
        if (comm.compareTo("wlan") == 0) {
          if (*(commandParts + 1) && *(commandParts + 2)) {
            String ssid = String(*(commandParts + 1));
            String pass = String(*(commandParts + 2));
            bool result = connectWLAN(true, &ssid, &pass);
            Serial.print("{wlan_return|");
            Serial.print(result);
            Serial.println("}");
          }
          else {
            Serial.println("Parametros incorrectos. {wlan|[ssid]|[password]}");
          }
        }
        else if (comm.compareTo("poweron") == 0) {
          commandPowerOn();
          Serial.println("{poweron_return|1}");
        }
        else if (comm.compareTo("poweroff") == 0) {
          commandPowerOff();
          Serial.println("{poweroff_return|1}");
        }
        else if (comm.compareTo("reboot") == 0) {
          commandReboot();
          Serial.println("{reboot_return|1}");
        }
        else if (comm.compareTo("power_monitor") == 0) {
          if (*(commandParts + 1)) {
            powerMonitorUpdate = millis();
            powerMonitorEnabled = atoi(*(commandParts + 1));
            Serial.println("{power_monitor_return|1}");
          }
          else { Serial.println("{power_monitor_return|0}"); }
        }
        else if (comm.compareTo("pm_tick") == 0) {
          powerMonitorUpdate = millis();
          Serial.println("{pm_tick_return|1}");
        }
        else {
          Serial.println("Comando no reconocido.");
        }
      }

      // Liberamos la memoria
      for (int i = 0; *(commandParts + i); i++)
      {
          free(*(commandParts + i));
      }
      free(commandParts);
    }
  }
}

void loop() {
  // Se comprueba si la WIFI sigue conectada y se intenta reconectar
  if (WiFi.status() != WL_CONNECTED) connectWLAN(true);

  // Se procesan las peticiones HTTP pendientes
  server.handleClient();

  // Se procesa la entrada de serial y los comandos pendientes
  handleSerialCommand();

  // Se comprueba si el monitor de hardware está activo y se reinicia de ser necesario
  if (powerMonitorEnabled && (powerMonitorUpdate + POWER_MONITOR_TIME) < millis()) {
    commandReboot();
    powerMonitorEnabled = false;
  }
}
