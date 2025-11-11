/*
 * Pauls Digital Enigma Machine Version 1.0 For Arduino MEGA 2560!
 * - PC USB Serial: text input/output
 * - Serial1 (pins 18/19): Morse output to second Arduino
 * - Rotary encoders + buttons for rotor/ring/reflector settings
 * - 16x2 I2C LCD for settings display
 * - Debouncing added for encoders and buttons
 */

#include <avr/pgmspace.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Encoder.h>

// ----------------------
// LCD
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ----------------------
// Rotary encoders (right, middle, left)
Encoder rotorEnc[3] = {
  Encoder(2, 3),
  Encoder(4, 5),
  Encoder(6, 7)
};

// Buttons for reflector A/B/C
const int reflectorBtns[3] = {A0, A1, A2};

// Toggle button for rotor/ring mode
const int toggleBtn = A3;

// ----------------------
// Enigma configuration
const char rotorI[]    PROGMEM = "EKMFLGDQVZNTOWYHXUSPAIBRCJ";
const char rotorII[]   PROGMEM = "AJDKSIRUXBLHWTMCQGZNPYFVOE";
const char rotorIII[]  PROGMEM = "BDFHJLCPRTXVZNYEIWGAKMUSQO";
const char rotorIV[]   PROGMEM = "ESOVPZJAYQUIRHXLNFTGKDCMWB";
const char rotorV[]    PROGMEM = "VZBRGITYUPSDNHLXAWMJQOFECK";
const char rotorNotches[5] PROGMEM = { 'Q', 'E', 'V', 'J', 'Z' };
const char reflectorA[] PROGMEM = "EJMZALYXVBWFCRQUONTSPIKHGD";
const char reflectorB[] PROGMEM = "YRUHQSLDPXNGOKMIEBFZCWVJAT";
const char reflectorC[] PROGMEM = "FVPJIAOYEDRZXWGCTKUQSBNMHL";

uint8_t rotorSelection[3] = {0,1,2};   // rotor indices
uint8_t rotorPositions[3] = {0,0,0};   // 0-25
uint8_t ringSettings[3]   = {0,0,0};   // 0-25
uint8_t reflectorChoice   = 1;         // 0=A,1=B,2=C
char plugboard[26];                    // identity + 10 pairs

// ----------------------
// UI state
bool ringMode = false;
long lastEncoder[3] = {0,0,0};
unsigned long lastEncoderTime[3] = {0,0,0};
const unsigned long encoderDebounce = 5;  // ms

// Button debounce
unsigned long lastToggleTime = 0;
bool lastToggleState = HIGH;
const unsigned long buttonDebounce = 50;  // ms

unsigned long lastRefBtnTime[3] = {0,0,0};
bool lastRefBtnState[3] = {HIGH,HIGH,HIGH};

// ----------------------
// Function prototypes
void initializeEnigma();
void plugboardConnect(char a,char b);
char plugboardSub(char c);
void stepRotors();
char rotorForward(char c,uint8_t r);
char rotorBackward(char c,uint8_t r);
char reflectorPass(char c);
char enigmaEncrypt(char c);
void updateLCD();

void setup() {
  Serial.begin(9600);    // PC
  Serial1.begin(9600);   // Morse Arduino

  Serial.println(F("Enigma Machine Ready"));
  Serial.println(F("Input text to encrypt/decrypt:"));

  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Enigma Ready");

  for(int i=0;i<3;i++) pinMode(reflectorBtns[i], INPUT_PULLUP);
  pinMode(toggleBtn, INPUT_PULLUP);

  initializeEnigma();
  updateLCD();
}

// ----------------------
void loop() {
  handleEncoders();
  handleButtons();

  // PC input
  if(Serial.available()){
    String input = Serial.readString();
    input.toUpperCase();
    input.replace(" ","");
    String encryptedLine = "";

    for(int i=0;i<input.length();i++){
      char c = input[i];
      if(c>='A' && c<='Z'){
        char encrypted = enigmaEncrypt(c);
        encryptedLine += encrypted;
        Serial1.print(encrypted); // Morse Arduino
      }
    }
    Serial.println(encryptedLine);
    Serial1.println();
  }

  delay(10); // small loop delay
}

// ----------------------
// Debounced encoders
void handleEncoders(){
  for(int i=0;i<3;i++){
    long val = rotorEnc[i].read() / 4; // adjust sensitivity
    unsigned long now = millis();
    if(val != lastEncoder[i] && (now - lastEncoderTime[i]) > encoderDebounce){
      lastEncoder[i] = val;
      lastEncoderTime[i] = now;

      char letter = 'A' + ((val % 26 + 26) % 26);
      if(ringMode) ringSettings[i] = letter - 'A';
      else rotorPositions[i] = letter - 'A';
      updateLCD();
    }
  }
}

// ----------------------
// Debounced buttons
void handleButtons(){
  unsigned long now = millis();

  // Toggle button
  bool toggleState = digitalRead(toggleBtn);
  if(toggleState == LOW && lastToggleState == HIGH && (now - lastToggleTime) > buttonDebounce){
    ringMode = !ringMode;
    updateLCD();
    lastToggleTime = now;
  }
  lastToggleState = toggleState;

  // Reflector buttons
  for(int i=0;i<3;i++){
    bool state = digitalRead(reflectorBtns[i]);
    if(state == LOW && lastRefBtnState[i] == HIGH && (now - lastRefBtnTime[i]) > buttonDebounce){
      reflectorChoice = i;
      updateLCD();
      lastRefBtnTime[i] = now;
    }
    lastRefBtnState[i] = state;
  }
}

// ----------------------
void initializeEnigma(){
  for(int i=0;i<26;i++) plugboard[i]='A'+i;
  plugboardConnect('A','B'); plugboardConnect('C','D'); plugboardConnect('E','F');
  plugboardConnect('G','H'); plugboardConnect('I','J'); plugboardConnect('K','L');
  plugboardConnect('M','N'); plugboardConnect('O','P'); plugboardConnect('Q','R');
  plugboardConnect('S','T');
}

void plugboardConnect(char a,char b){
  plugboard[a-'A']=b;
  plugboard[b-'A']=a;
}

char plugboardSub(char c){ return plugboard[c-'A']; }

void stepRotors(){
  rotorPositions[0] = (rotorPositions[0]+1)%26;
  if(rotorPositions[0]==(pgm_read_byte(rotorNotches+rotorSelection[0])-'A')) rotorPositions[1] = (rotorPositions[1]+1)%26;
  if(rotorPositions[1]==(pgm_read_byte(rotorNotches+rotorSelection[1])-'A')) rotorPositions[2] = (rotorPositions[2]+1)%26;
}

char rotorForward(char c,uint8_t r){
  const char* w;
  switch(rotorSelection[r]){
    case 0:w=rotorI;break;
    case 1:w=rotorII;break;
    case 2:w=rotorIII;break;
    case 3:w=rotorIV;break;
    case 4:w=rotorV;break;
    default:w=rotorI;
  }
  uint8_t off=(c-'A'+rotorPositions[r]-ringSettings[r]+26)%26;
  char out=pgm_read_byte(w+off);
  out=(out-'A'-rotorPositions[r]+ringSettings[r]+26)%26+'A';
  return out;
}

char rotorBackward(char c,uint8_t r){
  const char* w;
  switch(rotorSelection[r]){
    case 0:w=rotorI;break;
    case 1:w=rotorII;break;
    case 2:w=rotorIII;break;
    case 3:w=rotorIV;break;
    case 4:w=rotorV;break;
    default:w=rotorI;
  }
  uint8_t off=(c-'A'+rotorPositions[r]-ringSettings[r]+26)%26;
  for(uint8_t i=0;i<26;i++){
    if(pgm_read_byte(w+i)=='A'+off){
      char out=(i-rotorPositions[r]+ringSettings[r]+26)%26+'A';
      return out;
    }
  }
  return c;
}

char reflectorPass(char c){
  const char* r;
  switch(reflectorChoice){
    case 0:r=reflectorA;break;
    case 1:r=reflectorB;break;
    case 2:r=reflectorC;break;
    default:r=reflectorB;
  }
  return pgm_read_byte(r+c-'A');
}

char enigmaEncrypt(char c){
  stepRotors();
  char x=plugboardSub(c);
  for(int i=0;i<3;i++) x=rotorForward(x,i);
  x=reflectorPass(x);
  for(int i=2;i>=0;i--) x=rotorBackward(x,i);
  x=plugboardSub(x);
  return x;
}

void updateLCD(){
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("R:");
  lcd.print((char)('A'+rotorPositions[2]));
  lcd.print((char)('A'+rotorPositions[1]));
  lcd.print((char)('A'+rotorPositions[0]));
  lcd.print(" Ref:");
  lcd.print((char)('A'+reflectorChoice));

  lcd.setCursor(0,1);
  lcd.print("Ring:");
  lcd.print((char)('A'+ringSettings[2]));
  lcd.print((char)('A'+ringSettings[1]));
  lcd.print((char)('A'+ringSettings[0]));
}
