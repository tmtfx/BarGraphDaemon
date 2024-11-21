/*
MIT License

Copyright (c) 2024 Fabio Tomat (TmTFx)

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
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

byte EmptyBarChar[8] = {
  0b00000,  // Riga 1
  0b00000,  // Riga 2
  0b00000,  // Riga 3
  0b00000,  // Riga 4
  0b00000,  // Riga 5
  0b00000,  // Riga 6
  0b00000,  // Riga 7
  0b00000   // Riga 8
};

byte ZeroBarGraphChar[8] = {
  0b00000,  // Riga 1
  0b00000,  // Riga 2
  0b00000,  // Riga 3
  0b00000,  // Riga 4
  0b00000,  // Riga 5
  0b00000,  // Riga 6
  0b00000,  // Riga 7
  0b11111   // Riga 8
};

byte HalfBarGraphChar[8] = {
  0b00000,  // Riga 1
  0b00000,  // Riga 2
  0b00000,  // Riga 3
  0b00000,  // Riga 4
  0b11111,  // Riga 5
  0b11111,  // Riga 6
  0b11111,  // Riga 7
  0b11111   // Riga 8
};
/*
byte HalfUBarGraphChar[8] = {
  0b11111,  // Riga 1
  0b11111,  // Riga 2
  0b11111,  // Riga 3
  0b11111,  // Riga 4
  0b00000,  // Riga 5
  0b00000,  // Riga 6
  0b00000,  // Riga 7
  0b00000   // Riga 8
};*/

class FIFO {
private:
    static const int size = 20;
    int buffer[size] = {0};

public:
    FIFO() {
        // Inizializza tutti gli elementi a 0 (opzionale già fatto sopra, per sicurezza ripeto)
        for (int i = 0; i < size; i++) {
            buffer[i] = 0;
        }
    }
    void push(int value) {
        if (value < 0 || value > 100) {
            return;
        }
        for (int i = 0; i < size-1; i++) {
          buffer[i]=buffer[i+1];
        }
        
        buffer[size-1] = value;
    }
    void get(int output[size]) {
        for (int i = 0; i < size; ++i) {
            output[i] = buffer[i];
        }
    }
};

String labels[8] = {"1:", "2:", "3:", "4:", "5:", "6:", "7:", "8:"};  // Etichette predefinite
int numBars = 4;
bool showLabels = true;
FIFO* fifo = new FIFO();

void setup() {
  pinMode(backlightPin, OUTPUT);
  lcd.begin(20, 4);
  lcd.createChar(0, semiTransparentChar);
  lcd.createChar(1, InnerBarChar);
  lcd.createChar(2, StartBarChar);
  lcd.createChar(3, EndBarChar);
  lcd.createChar(4, EmptyBarChar);
  lcd.createChar(5,ZeroBarGraphChar);
  lcd.createChar(6,HalfBarGraphChar);
  Serial.begin(115200);
  analogWrite(backlightPin, 128);
}

void loop() {
  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');
    int commandType = input.substring(0, 1).toInt();

    if (commandType == 0) {
      displayData(input.substring(2));
    } else if (commandType == 1) {
      configureLabels(input.substring(2));
    } else if (commandType == 2) {
      showLabels = !showLabels;
    } else if (commandType == 3) {
      String message = input.substring(2);
      if (message.length() > 0) {
        displayMessage(message);
        delay(3000);
      }
      lcd.clear();
      analogWrite(backlightPin, 0);
    } else if (commandType == 4) {
      lcd.clear();
    } else if (commandType == 5) {
      displayGraph(input.substring(2));
    }
  }
}

void displayMessage(String message) {
  lcd.clear();
  int newlinePos = message.indexOf('\n');
  
  if (newlinePos != -1) {
    String firstLine = message.substring(0, newlinePos);
    String secondLine = message.substring(newlinePos + 1);

    int len1 = firstLine.length();
    int pos1 = (16 - len1) / 2;
    lcd.setCursor(pos1, 0);
    lcd.print(firstLine);
    
    int len2 = secondLine.length();
    int pos2 = (16 - len2) / 2;
    lcd.setCursor(pos2, 1);
    lcd.print(secondLine);
    
  } else if (message.length() <= 20) {
    int len = message.length();
    int pos = (16 - len) / 2;
    lcd.setCursor(pos, 1);
    lcd.print(message);

  } else if (message.length() <= 40) {
    int len = message.length();
    int minDist = 40;
    int splitPos = -1;

    for (int i = 0; i < len && i < 20; i++) {
        if (message[i] == ' ') {
            int dist = abs(i - 10);
            if (dist < minDist) {
                minDist = dist;
                splitPos = i;
            }
        }
    }

    if (splitPos != -1) {
        String firstLine = message.substring(0, splitPos);
        String secondLine = message.substring(splitPos + 1);

        int pos1 = (16 - firstLine.length()) / 2;
        lcd.setCursor(pos1, 0);
        lcd.print(firstLine);

        if (secondLine.length() > 20) {
            String secondPart = secondLine.substring(0, 20);
            String thirdPart = secondLine.substring(20);

            int pos2 = (16 - secondPart.length()) / 2;
            lcd.setCursor(pos2, 1);
            lcd.print(secondPart);

            int pos3 = (16 - thirdPart.length()) / 2;
            lcd.setCursor(pos3, 2);
            lcd.print(thirdPart);
        } else {
            int pos2 = (16 - secondLine.length()) / 2;
            lcd.setCursor(pos2, 1);
            lcd.print(secondLine);
        }

    } else {
        String firstLine = message.substring(0, 20);
        String secondLine = message.substring(20);

        int pos1 = (16 - firstLine.length()) / 2;
        lcd.setCursor(pos1, 0);
        lcd.print(firstLine);

        int pos2 = (16 - secondLine.length()) / 2;
        lcd.setCursor(pos2, 1);
        lcd.print(secondLine);
    }
  }
}

void configureLabels(String configString) {
  int labelCount = 0;
  int start = 0;

  while (labelCount < 8 && start < configString.length()) {
    int pos = configString.indexOf(' ', start);
    if (pos == -1) pos = configString.length();
    labels[labelCount] = configString.substring(start, pos);
    start = pos + 1;
    labelCount++;
  }
  numBars = labelCount;
}

void displayGraph(String dataString) {
  int value;
  bool valueGot = false;
  int start = 0;
  
  while (!valueGot && start < dataString.length()) {
    int pos = dataString.indexOf(' ', start);
    if (pos == -1) pos = dataString.length();
    value = dataString.substring(start, pos).toInt();
    start = pos + 1;
    valueGot=true;
  }

  if (start < dataString.length()) {
    int backlightLevel = dataString.substring(start).toInt();
    backlightLevel = constrain(backlightLevel, 0, 100);
    int pwmValue = map(backlightLevel, 0, 100, 0, 255);
    analogWrite(backlightPin, pwmValue);
  }

  int constrValue = constrain(value, 0, 100);
  
  fifo->push(constrValue);
  int data[20];
  fifo->get(data);
  for (int col = 0; col < 20; col++) {
    int value = data[col];
    int fullBlocks = value / 25; //25% = 1/4 siccome ci sono 4 quadrati pieni su ogni colonna
    int remainder = value % 25;

    for (int row = 3; row >= 0; row--) {
        lcd.setCursor(col, row);

        if (fullBlocks > 0) {
            // Se ci sono righe completamente piene, usa il carattere pieno
            lcd.write(byte(255));
            fullBlocks--;
        } else if (remainder > 0) {
            // Se c'è un resto, scegli il carattere corretto
            if (row == 3) { // Resto sulla quarta riga
                if (remainder < 7) {
                    lcd.write(byte(5)); // Mezzo quadratino vuoto
                } else if (remainder < 13) {
                    lcd.write(byte(6)); // Mezzo quadratino pieno
                } else {
                    lcd.write(byte(255)); // Quadratino pieno
                }
            } else {
                if (remainder < 13) {
                    lcd.write(byte(6)); // Mezzo quadratino pieno
                } else {
                    lcd.write(byte(255)); // Quadratino pieno
                }
                //lcd.write(4); // Spazio vuoto per righe superiori
            }
            remainder = 0; // Consuma il resto
        } else {
            // Righe vuote
            lcd.write(4);
        }
    }
  }
}

void displayData(String dataString) {
  //lcd.clear();
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
      if ( i < numBars ){
        lcd.setCursor(0, i);
        int constrainedValue = constrain(values[i], 0, 100);
        if (showLabels) {
          lcd.print(labels[i]);
          barLength = map(constrainedValue, 0, 100, 0, 18);
        } else {
          barLength = map(constrainedValue, 0, 100, 0, 20);
        }
        for (int j = 0; j < barLength; j++) lcd.write(byte(255));
        int maxBarLength = showLabels ? 18 : 20;
        for (int j = barLength; j < maxBarLength; j++) lcd.write(byte(4));
      }
    }
  }
}
