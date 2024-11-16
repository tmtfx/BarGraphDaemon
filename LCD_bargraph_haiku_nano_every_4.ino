#include <LiquidCrystal.h>

const int rs = 12, en = 11, d4 = 5, d5 = 4, d6 = 3, d7 = 2;
const int backlightPin = 9;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

byte semiTransparentChar[8] = {
  0b10101,  // Riga 1
  0b01010,  // Riga 2
  0b10101,  // Riga 3
  0b01010,  // Riga 4
  0b10101,  // Riga 5
  0b01010,  // Riga 6
  0b10101,  // Riga 7
  0b01010   // Riga 8
};
byte InnerBarChar[8] = {
  0b11111,  // Riga 1
  0b00000,  // Riga 2
  0b00000,  // Riga 3
  0b00000,  // Riga 4
  0b00000,  // Riga 5
  0b00000,  // Riga 6
  0b00000,  // Riga 7
  0b11111   // Riga 8
};
byte EndBarChar[8] = {
  0b11111,  // Riga 1
  0b00001,  // Riga 2
  0b00001,  // Riga 3
  0b00001,  // Riga 4
  0b00001,  // Riga 5
  0b00001,  // Riga 6
  0b00001,  // Riga 7
  0b11111   // Riga 8
};
byte StartBarChar[8] = {
  0b11111,  // Riga 1
  0b10000,  // Riga 2
  0b10000,  // Riga 3
  0b10000,  // Riga 4
  0b10000,  // Riga 5
  0b10000,  // Riga 6
  0b10000,  // Riga 7
  0b11111   // Riga 8
};

String labels[8] = {"1:", "2:", "3:", "4:", "5:", "6:", "7:", "8:"};  // Etichette predefinite
int numBars = 4;  // Numero iniziale di barre, configurabile
bool showLabels = true;  // Variabile per decidere se visualizzare le etichette

void setup() {
  pinMode(backlightPin, OUTPUT);
  lcd.begin(20, 4);bool showLabels = true;  // Variabile per decidere se visualizzare le etichette
  lcd.createChar(0, semiTransparentChar);
  lcd.createChar(1, InnerBarChar);
  lcd.createChar(2, StartBarChar);
  lcd.createChar(3, EndBarChar);
  Serial.begin(115200);
  //Serial.println("Inizio setup");
  analogWrite(backlightPin, 128);  // Imposta retroilluminazione a metà potenza inizialmente
  //Serial.println("Fine setup");
}

void loop() {
  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');
    //Serial.print("Received string: ");
    //Serial.println(input);
    int commandType = input.substring(0, 1).toInt();

    if (commandType == 0) {
      // Modalità di visualizzazione dei dati
      displayData(input.substring(2));
    } else if (commandType == 1) {
      // Modalità di configurazione etichette
      configureLabels(input.substring(2));
    } else if (commandType == 2) {
      // Comando per abilitare/disabilitare le etichette
      showLabels = !showLabels;
      //Serial.print("Show labels: ");
      //Serial.println(showLabels ? "Enabled" : "Disabled");
    } else if (commandType == 3) {
      String message = input.substring(2);
      if (message.length() > 0) {
        displayMessage(message);
        delay(3000);
      }
      // Spegni la retroilluminazione del display*/
      lcd.clear();
      analogWrite(backlightPin, 0);
    }
  }
}

void displayMessage(String message) {
  lcd.clear();
  // Se la stringa contiene un carattere di newline, la divido in due parti
  int newlinePos = message.indexOf('\n');
  
  if (newlinePos != -1) {
    // Se c'è un '\n', divido la stringa in due parti
    String firstLine = message.substring(0, newlinePos);
    String secondLine = message.substring(newlinePos + 1); // +1 per escludere il '\n'
    
    // Centratura per la prima riga
    int len1 = firstLine.length();
    int pos1 = (16 - len1) / 2;  // Calcola la posizione centrale della riga
    lcd.setCursor(pos1, 0);
    lcd.print(firstLine);
    
    // Centratura per la seconda riga
    int len2 = secondLine.length();
    int pos2 = (16 - len2) / 2;  // Calcola la posizione centrale della riga
    lcd.setCursor(pos2, 1);
    lcd.print(secondLine);
    
  } else if (message.length() <= 20) {
    // Se la stringa è inferiore o uguale a 20 caratteri, centratela sulla seconda riga
    int len = message.length();
    int pos = (16 - len) / 2;  // Calcola la posizione centrale della riga
    lcd.setCursor(pos, 1);
    lcd.print(message);

  } else if (message.length() <= 40) {
    int len = message.length();
    int minDist = 40;  // Inizializza con la lunghezza massima della stringa
    int splitPos = -1;  // Posizione dove fare il "a capo"

    // Trova la posizione dello spazio più vicino al centro (massimo 20 caratteri per la prima riga)
    for (int i = 0; i < len && i < 20; i++) {
        if (message[i] == ' ') {
            int dist = abs(i - 10);  // Distanza dal centro della riga (10 è il centro)
            if (dist < minDist) {
                minDist = dist;
                splitPos = i;
            }
        }
    }

    // Se troviamo un buon punto di split, dividi la stringa
    if (splitPos != -1) {
        String firstLine = message.substring(0, splitPos);
        String secondLine = message.substring(splitPos + 1);  // Dopo lo spazio

        // Centra la prima riga sulla prima riga del display (massimo 20 caratteri)
        int pos1 = (16 - firstLine.length()) / 2;
        lcd.setCursor(pos1, 0);
        lcd.print(firstLine);

        // Se la seconda riga è maggiore di 20 caratteri, dividila
        if (secondLine.length() > 20) {
            String secondPart = secondLine.substring(0, 20);  // La prima parte della seconda riga
            String thirdPart = secondLine.substring(20);  // La seconda parte, che va su una terza riga

            // Centra la seconda riga
            int pos2 = (16 - secondPart.length()) / 2;
            lcd.setCursor(pos2, 1);
            lcd.print(secondPart);

            // Centra la terza riga
            int pos3 = (16 - thirdPart.length()) / 2;
            lcd.setCursor(pos3, 2);
            lcd.print(thirdPart);
        } else {
            // Centra la seconda riga sulla seconda riga del display
            int pos2 = (16 - secondLine.length()) / 2;
            lcd.setCursor(pos2, 1);
            lcd.print(secondLine);
        }

    } else {
        // Se non ci sono spazi, fai un "a capo" fisso dopo 20 caratteri
        String firstLine = message.substring(0, 20);
        String secondLine = message.substring(20);

        // Centra la prima riga
        int pos1 = (16 - firstLine.length()) / 2;
        lcd.setCursor(pos1, 0);
        lcd.print(firstLine);

        // Centra la seconda riga
        int pos2 = (16 - secondLine.length()) / 2;
        lcd.setCursor(pos2, 1);
        lcd.print(secondLine);
    }
  }
}

void configureLabels(String configString) {
  int labelCount = 0;
  int start = 0;

  // Estrai ogni etichetta e aggiornala nell'array labels
  while (labelCount < 8 && start < configString.length()) {
    int pos = configString.indexOf(' ', start);
    if (pos == -1) pos = configString.length();
    labels[labelCount] = configString.substring(start, pos);
    start = pos + 1;
    labelCount++;
  }
  numBars = labelCount;  // Numero di barre impostato da config
}

void displayData(String dataString) {
  lcd.clear();
  int values[8];
  int valueCount = 0;
  int start = 0;
  // Parsing dei valori
  while (valueCount < numBars && start < dataString.length()) {
    int pos = dataString.indexOf(' ', start);
    if (pos == -1) pos = dataString.length();
    values[valueCount] = dataString.substring(start, pos).toInt();
    start = pos + 1;
    valueCount++;
  }
  // Se ci sono valori in più (oltre numBars), l'ultimo viene usato per la retroilluminazione
  if (start < dataString.length()) {
    int backlightLevel = dataString.substring(start).toInt();  // L'ultimo valore per la retroilluminazione
    backlightLevel = constrain(backlightLevel, 0, 100);  // Limita il valore tra 0 e 100
    int pwmValue = map(backlightLevel, 0, 100, 0, 255);  // Mappa a 0-255 per il PWM
    analogWrite(backlightPin, pwmValue);  // Imposta la retroilluminazione
  }

  // Visualizza le barre sul display LCD
  for (int i = 0; i < 4; i++) {
    int indexLeft = i * 2;
    int indexRight = indexLeft + 1;
    int barLength;
    if (numBars > 4) {
      // Se il numero di barre è dispari e siamo sull'ultima riga, centra la barra
      if (numBars % 2 != 0 && indexLeft >= numBars - 1 && i < 3) {
        lcd.setCursor(5, i);
        int constrLValue = constrain(values[indexLeft], 0, 100);
        if (showLabels) {
          lcd.print(labels[indexLeft]);
          barLength = map(constrLValue, 0, 100, 0, 8);
        } else {
          barLength = map(constrLValue, 0, 100, 0, 10);
        }
        for (int j = 0; j < barLength; j++) lcd.write(byte(255));
        int maxBarLength = showLabels ? 8 : 10;  // Lunghezza massima in base alla presenza delle etichette
        for (int j = barLength; j < maxBarLength; j++) {
            lcd.write(byte(0));  // Carattere personalizzato a semitrasparenza
        }
      } else {
        // Posiziona la barra sinistra
        if (indexLeft < numBars) {
          lcd.setCursor(0, i);
          int constrLeftValue = constrain(values[indexLeft], 0, 100);
          if (showLabels) {
            lcd.print(labels[indexLeft]);
            barLength = map(constrLeftValue, 0, 100, 0, 8);  // Barra sinistra massima di 8 caratteri
          } else {
            barLength = map(constrLeftValue, 0, 100, 0, 10);  // Barra sinistra massima di 10 caratteri
          }
          for (int j = 0; j < barLength; j++) lcd.write(byte(255));
          int maxBarLength = showLabels ? 8 : 10;  // Lunghezza massima in base alla presenza delle etichette
          for (int j = barLength; j < maxBarLength; j++) {
              //lcd.write(byte(0));  // Carattere personalizzato
              if (j == 0) {
                lcd.write(byte(2)); //carattere personalizzato di inizio
              } else if (j == maxBarLength-1) {
                lcd.write(byte(3)); //carattere personalizzato di fine
              } else {
                lcd.write(byte(1)); //carattere personalizzato intermedio
              }
          }
        }

        // Posiziona la barra destra
        if (indexRight < numBars) {
          lcd.setCursor(10, i);
          int constrRightValue = constrain(values[indexRight], 0, 100);
          if (showLabels) {
            lcd.print(labels[indexRight]);
            barLength = map(constrRightValue, 0, 100, 0, 8);  // Barra destra massima di 8 caratteri
          } else {
            barLength = map(constrRightValue, 0, 100, 0, 10);  // Barra destra massima di 10 caratteri
          }
          for (int j = 0; j < barLength; j++) lcd.write(byte(255));
          int maxBarLength = showLabels ? 8 : 10;  // Lunghezza massima in base alla presenza delle etichette
          for (int j = barLength; j < maxBarLength; j++) {
              if (j == 0) {
                lcd.write(byte(2)); //carattere personalizzato di inizio
              } else if (j == maxBarLength-1) {
                lcd.write(byte(3)); //carattere personalizzato di fine
              } else {
                lcd.write(byte(1)); //carattere personalizzato intermedio
              }
          }
        }
      }
    } else {
      lcd.setCursor(0, i);
      int constrainedValue = constrain(values[i], 0, 100);
      if (showLabels) {
        lcd.print(labels[i]);
        barLength = map(constrainedValue, 0, 100, 0, 18);
      } else {
        barLength = map(constrainedValue, 0, 100, 0, 20);
      }
      for (int j = 0; j < barLength; j++) lcd.write(byte(255));
    }
  }
}
