// Définition des variables globales
#define SWITCH_REVERSING_CONTROL_DEFAULT_VALUE true
volatile int switch_reversing_control = SWITCH_REVERSING_CONTROL_DEFAULT_VALUE;

bool satellite_joined = false;            // Variable pour suivre si le satellite est rejoint
unsigned long last_time_send = 0;
int current_value = -1;                   // Valeur actuellement envoyée
bool value_acknowledged = false;          // Indique si la dernière valeur a été reconnue
unsigned long current_time;

//############################################ code ultrason #################################
int pwmPin1 = PA2;
int pwmPin2 = PA3;
int RX1 = PB13;
int RX2 = PB14;

const int alimPin = PB3;
const int pirPin = PB4;
const int alimPin1 = PB15;
const int pirPin1 = PB5;
bool connexionEffectuee = false;
int seuil = 100;
int compt_entree = 0;
int compt_sortie = 0;
int etape = 0;
int pret;

unsigned long previousMillis = 0;
unsigned long lastActivityTime = 0;
const long interval = 2000; //interval d'attente entre les deux capteurs
const long timeout = 60000;  // temps de veilles
const long temps_envoie = 120000; // 60 secondes pour envoyer les donner

bool systemActive = true;

void setup(void) {
    pinMode(pwmPin1, INPUT);
    pinMode(pwmPin2, INPUT);
    pinMode(RX1, OUTPUT);
    pinMode(RX2, OUTPUT);
    pinMode(pirPin, INPUT);
    pinMode(alimPin, OUTPUT);
    pinMode(pirPin1, INPUT);
    pinMode(alimPin1, OUTPUT);

    // Serial.begin(9600);

    digitalWrite(RX1, LOW);
    digitalWrite(RX2, LOW);
    digitalWrite(alimPin, HIGH);
    digitalWrite(alimPin1, HIGH);

    // Initialisation du port série
    Serial.begin(115200);

    // Initialisation du contrôle du commutateur inverse
    // connexion_sat();
}

// Boucle principale
void loop(void) {
    
    lecture_port();
    comptage();
}

//---------------------------------------------- Reception satellitaire -----------------------------

void reception_satellitaire() {
    if (ECHOSTAR_SERIAL.available()) {
        String response = ECHOSTAR_SERIAL.readStringUntil('\n');
        Serial.print("Received from satellite: ");
        Serial.println(response);

        // Vérification si la connexion au satellite a réussi
        if (!satellite_joined && response.indexOf("Successfully joined network") != -1) {
            satellite_joined = true;
            // Serial.println("Satellite successfully joined!");
        }

        // Vérification de l'acquittement de la valeur envoyée
        if (response.indexOf("QUEUED:1") != -1) {
            Serial.println("Value acknowledged by satellite.");
            value_acknowledged = true; // Marquer la valeur comme acquittée
            current_value = -1;        // Réinitialiser la valeur pour permettre de générer une nouvelle après 1 heure
        }
    }
}

//-------------------------------------- Transfert des données entre le port série et ECHOSTAR_SERIAL ------------------------------

void lecture_port() {
    if (Serial.available()) {
        char c = Serial.read();
        ECHOSTAR_SERIAL.write(c);
        reception_satellitaire();

        // Contrôle du commutateur via le caractère '*'
        if (c == '*') {
            switch_reversing_control ^= true;
            do_switch_ctrl_update();
        }
    }
}

//-------------------------------------- Lecture capteur ultrason -----------------------------------------

float lireCapteur1() {
    digitalWrite(RX1, HIGH);
    delay(40);
    long duration = pulseIn(pwmPin1, HIGH);
    digitalWrite(RX1, LOW);
    return duration / 58.0;
}

float lireCapteur2() {
    digitalWrite(RX2, HIGH);
    delay(40);
    long duration = pulseIn(pwmPin2, HIGH);
    digitalWrite(RX2, LOW);
    return duration / 58.0;
}

//--------------------------------------- Fonction pour mettre à jour le contrôle du commutateur ---------------------------------

void do_switch_ctrl_update(void) {
    if (digitalRead(ECHOSTAR_SWCTRL_PIN) == LOW) {
        digitalWrite(DPDT_CTRL_PIN, switch_reversing_control ? HIGH : LOW);
        digitalWrite(LED_BUILTIN, switch_reversing_control ? HIGH : LOW);
    } else {
        digitalWrite(DPDT_CTRL_PIN, switch_reversing_control ? LOW : HIGH);
        digitalWrite(LED_BUILTIN, switch_reversing_control ? LOW : HIGH);
    }
}

//-------------------------------- Définition de la fonction ISR (Interruption Service Routine) --------------------

void swctrl_change_isr(void) {
    do_switch_ctrl_update();
}

//---------------------------------- Mise en veille ---------------------------------------------


//---------------------------------- Compteur de personne ----------------------------------------------
void comptage() {
    if (!systemActive) {
        if ((digitalRead(pirPin) == HIGH) || (digitalRead(pirPin1) == HIGH) || (pret == 1)) {
            Serial.println("Mouvement détecté par le PIR ! Réveil du système.");
            systemActive = true;
            lastActivityTime = millis();
        }
        return; // Ne pas exécuter le reste tant que le système est en veille
    }
    if (current_time - last_time_send < temps_envoie) {
        float distance1 = lireCapteur1();
        delay(40);
        float distance2 = lireCapteur2();
        delay(40);

        unsigned long currentMillis = millis();

        switch (etape) {
            case 0:
                if (distance2 <= seuil) {
                    etape = 1;
                    lastActivityTime = currentMillis;
                } else if (distance1 <= seuil) {
                    etape = 2;
                    lastActivityTime = currentMillis;
                }
                break;

            case 1:
                if (distance1 <= seuil) {
                    compt_entree++;
                    etape = 0;
                    lastActivityTime = currentMillis;
                } else if (currentMillis - previousMillis >= interval) {
                    previousMillis = currentMillis;
                    etape = 0;
                }
                break;

            case 2:
                if (distance2 <= seuil) {
                    compt_sortie++;
                    etape = 0;
                    lastActivityTime = currentMillis;
                } else if (currentMillis - previousMillis >= interval) {
                    previousMillis = currentMillis;
                    etape = 0;
                }
                break;
        }

        Serial.print("Capteur 1 - Distance : ");
        Serial.print(distance1);
        Serial.println(" cm");

        Serial.print("Capteur 2 - Distance : ");
        Serial.print(distance2);
        Serial.println(" cm");

        Serial.print("Entrée détectée : ");
        Serial.println(compt_entree);

        Serial.print("Sortie détectée : ");
        Serial.println(compt_sortie);

        delay(50);

        if (currentMillis - lastActivityTime >= timeout) {
            Serial.println("Aucune activité détectée. Mise en veille.");
            systemActive = false;
        }

        current_time = millis();
    } else if (current_time - last_time_send >= temps_envoie) {
      //Serial.println("Procédure d'envoi des données");
      envoie(); // fonction d'envoie
    }
}              

//----------------------------------------- Fonction d'envoi ------------------------------------------------------

void envoie() {
    if (!connexionEffectuee) {
        Serial.println("Procédure d'envoi des données");
        connexion_sat(); // Appeler cette fonction une seule fois
        connexionEffectuee = true;
    }

    reception_satellitaire();

    if (satellite_joined) {
        const int max_retries = 5;
        int retry_count = 0;

        pret = 1;
        value_acknowledged = false;
        retry_count = 0;
        if (!value_acknowledged) {
          for(int i=0 ; i<=3; i++){
            delay(10000);
            char int_command[50];
            sprintf(int_command, "AT+SEND=1,0,8,1,%d,%d\r\n", compt_entree, compt_sortie);
            ECHOSTAR_SERIAL.print(int_command);
            delay(12000);
            last_time_send = current_time;
            lecture_port();
            Serial.println("Données envoyées : ");
            
            //reception_satellitaire();
          }

            //connexionEffectuee = false;
            value_acknowledged = false;
            if (value_acknowledged== true ) {
                Serial.print("Données envoyées : ");
                last_time_send = current_time;
                return;

            }
            

        }
    } else {
        pret = 0;
    }
}

//---------------------------------------- Connexion satellitaire -----------------------

void connexion_sat() {
    switch_reversing_control = SWITCH_REVERSING_CONTROL_DEFAULT_VALUE;

    // Initialisation des broches
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH);

    #if defined(ECHOSTAR_PWR_ENABLE_PIN)
        pinMode(ECHOSTAR_PWR_ENABLE_PIN, OUTPUT);
        digitalWrite(ECHOSTAR_PWR_ENABLE_PIN, HIGH);
    #endif

    #if defined(DPDT_PWR_ENABLE_PIN)
        pinMode(DPDT_PWR_ENABLE_PIN, OUTPUT);
        digitalWrite(DPDT_PWR_ENABLE_PIN, HIGH);
    #endif

    pinMode(DPDT_CTRL_PIN, OUTPUT);
    digitalWrite(DPDT_CTRL_PIN, HIGH);

    pinMode(ECHOSTAR_SWCTRL_PIN, INPUT);
    attachInterrupt(digitalPinToInterrupt(ECHOSTAR_SWCTRL_PIN), swctrl_change_isr, CHANGE);

    while (!Serial);

    // Mise à jour du contrôle du commutateur
    do_switch_ctrl_update();
    Serial.print("RF Switch reverse control: ");
    Serial.println(switch_reversing_control ? "ENABLE" : "DISABLE");

    // Initialisation de la communication série avec le satellite
    ECHOSTAR_SERIAL.begin(115200);
    delay(1000);

    digitalWrite(LED_BUILTIN, LOW);

    // Envoi de la commande de connexion au satellite
    ECHOSTAR_SERIAL.println("AT+JOIN");
    Serial.println("Attempting to join satellite...");
}
void mensure_batterie(){



}
// ---------------------------------------- Fin du code -----------------------------------
