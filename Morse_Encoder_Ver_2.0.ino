const int buzzerPin = 8;
const int unitDuration = 100; // Base time unit in milliseconds

// Morse code lookup table for letters and digits
const char* morseCodes[] = {
  ".-", "-...", "-.-.", "-..", ".", "..-.", "--.", "....", "..", 
  ".---", "-.-", ".-..", "--", "-.", "---", ".--.", "--.-", ".-.", 
  "...", "-", "..-", "...-", ".--", "-..-", "-.--", "--..", 
  "-----", ".----", "..---", "...--", "....-", ".....", 
  "-....", "--...", "---..", "----."
};

void setup() {
  pinMode(buzzerPin, OUTPUT);
  Serial.begin(9600);
  Serial.println("Enter text to encode to Morse:");
}

void loop() {
  if (Serial.available() > 0) {
    String input = Serial.readStringUntil('\n');
    input.toUpperCase(); // Convert input to uppercase for consistency
    for (int i = 0; i < input.length(); i++) {
      char c = input[i];
      if (c == ' ') {
        delay(7 * unitDuration); // Word gap (7 units)
      } else if (isAlphaNumeric(c)) {
        int index = (c >= 'A' && c <= 'Z') ? c - 'A' : c - '0' + 26;
        const char* morseStr = morseCodes[index];
        for (int j = 0; morseStr[j] != '\0'; j++) {
          if (morseStr[j] == '.') {
            beep(unitDuration); // Dot
          } else if (morseStr[j] == '-') {
            beep(3 * unitDuration); // Dash
          }
          if (morseStr[j + 1] != '\0') {
            delay(unitDuration); // Symbol gap (1 unit)
          }
        }
        delay(2 * unitDuration); // Letter gap (3 units total = 1 symbol gap + 2)
      }
    }
    Serial.println("Transmission complete.");
  }
}

void beep(int duration) {
  digitalWrite(buzzerPin, HIGH);
  delay(duration);
  digitalWrite(buzzerPin, LOW);
}