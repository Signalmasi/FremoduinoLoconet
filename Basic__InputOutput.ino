///////////////
// Pin Alias
//
// RX = 0
// TX = 1
// 
// A0 = 14
// A1 = 15
// A2 = 16
// A3 = 17
// A4 = 18
// A5 = 19
//////////////////////////////
// Konfiguration
//////////////////////////////

const uint8_t N_INPUT = 5;     //Anzahl Pins für Eingänge
const uint8_t N_OUTPUT = 11;    //Anzahl Pins für Ausgänge

const int TIME_DEBOUNCE_LOW = 41;    //Zeit bis LOW=EIN erkannt wird
const int TIME_DEBOUNCE_HIGH = 205;  //Zeit bis HIGH=AUS erkannt wird

const int TIME_BLINK_DOWN = 1000; //Zeit, die blinkender Ausgang ausgeschaltet bleibt
const int TIME_BLINK_UP = 1000;   //Zeit, die blinkender Ausgang eingeschaltet bliebt

uint8_t INPUT_PINS[N_INPUT] = {A0, A1, A4, A3, A2};                        //Pin-Zuordnung am Arduino für Eingänge
uint8_t OUTPUT_PINS[N_OUTPUT] = {12, 4, 3, 5, 6, 2, 10, 11, 9, 1, 0};          //Pin-Zuordnung am Arduino für Ausgänge

//////////////////////////////
// Ab hier nichts ändern
//////////////////////////////
// Variablen
//////////////////////////////

struct {
  uint8_t inp_pin;            //Eingangspin am Arduino
  unsigned long time_ein; //Zeitpunkt beim letzten Zustand auf LOW
  unsigned long time_aus; //Zeitpunkt beim letzten Zustand auf HIGH
  boolean state;              //Nach abgelaufener Pufferzeit gespeicherter Status (1=EIN=LOW)
} E[N_INPUT];    //Datenfeld für Eingänge

struct {
  uint8_t out_pin;                //Ausgangspin am Arduino
  unsigned long time_ziel_ein;    //Wartezeit zum Einschalten
  unsigned long time_ziel_aus;    //Wartezeit zum Ausschalten
  unsigned long time_ziel_blink;  //Wartezeit zum Blinken
  unsigned long time_temp_blink;  //Wartezeit zur nächsten Zustandsänderung
  boolean b_ziel_ein;             //Merker, dass nach Ablauf von time_ziel_ein eingeschaltet werden soll
  boolean b_ziel_aus;             //Merker, dass nach Ablauf von time_ziel_aus ausgeschaltet werden soll
  boolean b_ziel_blink;           //Merker, dass nach Ablauf von time_ziel_blink geblinkt werden soll
  boolean b_state;                //Merker zum aktuellen Zustand des Ausgangs (true=ein), wechselt beim blinken
  boolean b_state_blink;
} A[N_OUTPUT];    //Datenfeld für Ausgänge

//////////////////////////////
// Funktion
//////////////////////////////

void InputReaction(uint8_t pin, boolean state);

void InputInit()
{
  uint8_t i;
  
  for (i=0; i<=N_INPUT-1; i++)
  {
   E[i].time_ein= 0;
   E[i].time_aus= 0;
   E[i].state= 0;
   
   E[i].inp_pin = INPUT_PINS[i];      //Hole Eingangspins aus dem Konfig.-Array
   pinMode(E[i].inp_pin, INPUT);      //Setze Pin als Eingang
   digitalWrite(E[i].inp_pin, HIGH);  //Aktiviere Pull-Up
   //Serial.print("Initialisierung Eingang: "); Serial.print(i); Serial.print(" Pin: "); Serial.println(E[i].inp_pin);  
  }
}

void OutputInit()
{
  uint8_t i;
  
  for (i=0; i<=N_OUTPUT-1; i++)
  {
   A[i].time_ziel_ein= 0;
   A[i].time_ziel_aus= 0;
   A[i].time_ziel_blink= 0;
   A[i].time_temp_blink= 0;
   A[i].b_ziel_ein= false;
   A[i].b_ziel_aus= false;
   A[i].b_ziel_blink= false;
   A[i].b_state= false;
   A[i].b_state_blink= false;
   A[i].out_pin = OUTPUT_PINS[i];
   pinMode(A[i].out_pin, OUTPUT);  //Setze Pin als Ausgang
   //Serial.print("Initialisierung Ausgang: "); Serial.print(i); Serial.print(" Pin: "); Serial.println(A[i].out_pin);
  }
}

void InputExec()
{
  uint8_t i, inp;
  
  for (i=0; i<=N_INPUT-1; i++)  //alle Eingänge durchgehen
  {
    inp = digitalRead(E[i].inp_pin);    //Pin einlesen
    if (inp == LOW)
      {E[i].time_ein = looptime;}       //wenn auf LOW gezogen, Zeitmarke setzen
    else
      {E[i].time_aus = looptime;}       //wenn auf HIGH gezogen, Zeitmarke setzen
    if(!E[i].state)                       //wenn Zustand AUS
    {
      if((looptime-E[i].time_aus) > TIME_DEBOUNCE_LOW)  //wenn während LOW-Pufferzeit kein HIGH vorkam
      {
        E[i].state = 1;                   //Merke Zustand EIN
        //Serial.print("Eingang "); Serial.print(i); Serial.print(" (Pin "); Serial.print(E[i].inp_pin); Serial.println("): EIN");
        InputReaction(i, E[i].state);
      }
    }
    else                                  //wenn Zustand EIN
    {
      if((looptime-E[i].time_ein) > TIME_DEBOUNCE_HIGH)  //wenn während HIGH-Pufferzeit kein LOW vorkam
      {
        E[i].state = 0;                 //Merke Zustand AUS
        //Serial.print("Eingang "); Serial.print(i); Serial.print(" (Pin "); Serial.print(E[i].inp_pin); Serial.println("): AUS");
        InputReaction(i, E[i].state);
      }
    } 
  }
}


void OutputExec()
{
  uint8_t i;
  for (i=0; i<=N_OUTPUT-1; i++)                    //Alle Ausgangspins durchgehen
  {   
    if (A[i].b_ziel_ein && (looptime > A[i].time_ziel_ein))  //Wenn Merker zum Einschalten gesetzt und Wartezeit abgelaufen
    {
      A[i].b_state_blink= false;
      A[i].b_ziel_ein = false;                          //Entferne Merker
      A[i].b_state = true;                         //Merke Zustand EIN
      digitalWrite(A[i].out_pin, HIGH);            //Schalte Ausgang auf
      //Serial.print("Ausgang "); Serial.print(i); Serial.print(" (Pin "); Serial.print(A[i].out_pin); Serial.println("): Eingeschaltet wegen Erreichen des EIN-Kommandos");
    } 
    if (A[i].b_ziel_blink && (looptime > A[i].time_ziel_blink)) //wenn Blinkmodus angefragt
    {
      A[i].b_ziel_blink = false;                       //Entferne Merker
      A[i].b_state_blink = true;                       //Blinkmodus einschalten
      //Serial.print("Ausgang "); Serial.print(i); Serial.print(" (Pin "); Serial.print(A[i].out_pin); Serial.println("): Blinkzustand wegen Erreichen des BLINK-Kommandos");
    }
    if (A[i].b_ziel_aus && (looptime > A[i].time_ziel_aus))  //Wenn Merker zum Ausschalten gesetzt und Wartezeit abgelaufen
    {
      A[i].b_state_blink= false;
      A[i].b_ziel_aus = false;                     //Entferne Merker
      A[i].b_state = false;                        //Merke Zustand AUS
      digitalWrite(A[i].out_pin, LOW);             //Schalte Ausgang ab
      //Serial.print("Ausgang "); Serial.print(i); Serial.print(" (Pin "); Serial.print(A[i].out_pin); Serial.println("): Ausgeschaltet wegen Erreichen des AUS-Kommandos");
    }    
    if (A[i].b_state_blink)                              //wenn Blinkmodus an
    {
      if (A[i].b_state && (looptime > A[i].time_temp_blink))  //wenn Lampe ein und Zeit abgelaufen
      {
        A[i].b_state = false;
        digitalWrite(A[i].out_pin, LOW);             //Schalte Ausgang ab
        A[i].time_temp_blink = looptime + TIME_BLINK_DOWN;//Neuen Einschaltzeitpunkt setzen
        //Serial.print("Ausgang "); Serial.print(i); Serial.print(" (Pin "); Serial.print(A[i].out_pin); Serial.println("): Ausgeschaltet im BLINK-Modus");
      }
      if (!A[i].b_state && (looptime > A[i].time_temp_blink))  //wenn Lampe aus und Zeit abgelaufen
      {
        A[i].b_state = true;
        digitalWrite(A[i].out_pin, HIGH);            //Schalte Ausgang auf
        A[i].time_temp_blink = looptime + TIME_BLINK_UP;  //Neuen Ausschaltzeitpunkt setzen
        //Serial.print("Ausgang "); Serial.print(i); Serial.print(" (Pin "); Serial.print(A[i].out_pin); Serial.println("): Eingeschaltet im BLINK-Modus");
      }
    }   
  }
}

void ON(int lamp, unsigned long time = 0)
{
  if (lamp < N_OUTPUT)                //wenn aufgerufener Ausgang existiert
  {
    A[lamp].b_ziel_ein= true;              //Setze Merker EIN
    A[lamp].time_ziel_ein= looptime+time;  //Setze Einschaltzeitpunkt
    //Serial.print("Ausgang "); Serial.print(lamp); Serial.print(" (Pin "); Serial.print(A[lamp].out_pin); Serial.println("): ON-Kommando gesetzt");
  }
}

void OFF(int lamp, unsigned long time = 0)
{
  if (lamp < N_OUTPUT)                //wenn aufgerufener Ausgang existiert
  {
    A[lamp].b_ziel_aus = true;             //Setze Merker AUS
    A[lamp].time_ziel_aus= looptime+time;  //Setze Ausschaltzeitpunkt
    //Serial.print("Ausgang "); Serial.print(lamp); Serial.print(" (Pin "); Serial.print(A[lamp].out_pin); Serial.println("): OFF-Kommando gesetzt");
 /*   if (time == 0)
    {
      A[lamp].b_ziel_ein = false;
      A[lamp].b_ziel_blink = false;         //Vorzugshandlung für OFF, wenn ohne Verzögerung gewählt  
      //Serial.print("Ausgang "); Serial.print(lamp); Serial.print(" (Pin "); Serial.print(A[lamp].out_pin); Serial.println("): OFF-Kommando Vorzugshandlung");
    } */
  }
}

void BLINK(int lamp, unsigned long time = 0)
{
  if (lamp < N_OUTPUT)                //wenn aufgerufener Ausgang existiert
  {
    A[lamp].b_ziel_blink= true;              //Setze Merker EIN
    A[lamp].time_ziel_blink= looptime+time;  //Setze Einschaltzeitpunkt
    //Serial.print("Ausgang "); Serial.print(lamp); Serial.print(" (Pin "); Serial.print(A[lamp].out_pin); Serial.println("): BLINK-Kommando gesetzt");
  }
}

void Setup_InputOutput()
{
  InputInit();
  OutputInit();  
}

void Loop_InputOutput()
{
  InputExec(); 
  OutputExec();  
}
