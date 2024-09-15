#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Arduino.h>
#include "HX711.h"

LiquidCrystal_I2C lcd(0x27, 16, 2);

byte rupeeSymbol[8] = {
  B11111,
  B00010,
  B11111,
  B00010,
  B01100,
  B00100,
  B00010,
  B00001
};

struct GroceryItem {
  String name;
  int quantity;
  int price;
};

GroceryItem groceries[] = {
  {"Milk", 0, 60},
  {"Toor Dal", 0, 150},
  {"Oil", 0, 150},
  {"Sugar", 0, 60}
};

#define NUM_GROCERIES sizeof(groceries) / sizeof(groceries[0])
#define relay1 5
#define relay2 4
#define ledPin1 6  
#define ledPin2 8  
#define buzzerPin 4
#define RST_PIN 9
#define SDA_PIN 10
#define BUTTON_PIN 5 
const int LOADCELL_DOUT_PIN = 2;
const int LOADCELL_SCK_PIN = 3;
HX711 scale;

MFRC522 mfrc522(SDA_PIN, RST_PIN);

int temp = 0;
int total = 0;
bool addMode = true;
float balance = 0.0; 

void performAction(int pin);
void addGrocery(int index, const char* name, int price);
void remGrocery(int index, const char* name, int price);
void displayMessage(const char* message);
void rfid();
void sendAllGroceries();
void sendSMSResponse(const char* message1, const char* message2 = "", const char* message3 = "");
void check();

void setup() {
  lcd.init();
  lcd.backlight();
  Serial.begin(9600); 
  SPI.begin();
  mfrc522.PCD_Init();

  pinMode(relay1, OUTPUT);
  pinMode(relay2, OUTPUT);
  pinMode(ledPin1, OUTPUT);  
  pinMode(ledPin2, OUTPUT);  
  pinMode(BUTTON_PIN, INPUT_PULLUP); 
  pinMode(buzzerPin, OUTPUT);
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  scale.set_scale(2280.f);   
  scale.tare(); 
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.createChar(0, rupeeSymbol);
  lcd.setCursor(0, 0);
  lcd.print(" SMART SHOPPING ");
  lcd.setCursor(0, 1);
  lcd.print("   CART SYSTEM ");
  delay(2000);

  
  Serial.println("AT"); 
  delay(1000);
  Serial.println("AT+CMGF=1"); 
  delay(1000);
}

void loop() {
  rfid();
  if (digitalRead(BUTTON_PIN) == LOW) {
    delay(50); 
    if (digitalRead(BUTTON_PIN) == LOW) {
      addMode = !addMode; 
      lcd.clear();
      lcd.setCursor(0, 0);
      performAction(ledPin2);  
      lcd.print(addMode ? "Add Product" : "Remove Product");
    }
  }
  if (temp == 1) {
    temp = 0;
    delay(1000);
  }
  check(); 
}
void rfid() {
  if (!mfrc522.PICC_IsNewCardPresent()) {
    return;
  }
  if (!mfrc522.PICC_ReadCardSerial()) {
    return;
  }
  
  String content = "";
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    content.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
    content.concat(String(mfrc522.uid.uidByte[i], HEX));
  }
  content.toUpperCase();
  if (addMode) {
    if (content.substring(1) == "3D 92 56 62") {
      performAction(ledPin1);  
      addGrocery(0, "Milk", 60);
      displayMessage("Milk + Coffee powder: 10% Off");
      performAction(ledPin1);
    } else if (content.substring(1) == "03 8C F9 2F") {
      performAction(ledPin1);
      addGrocery(1, "Toor Dal", 150);
      displayMessage("3kg Dal + Sambar Masala: 15% Off");
      performAction(ledPin1);
    } else if (content.substring(1) == "86 7D D8 FA") {
      performAction(ledPin1);
      addGrocery(2, "Oil", 150);
      displayMessage("1L oil + Mustard: 8% Off");
      performAction(ledPin1);
    } else if (content.substring(1) == "FC 12 44 44") {
      performAction(ledPin1);
      addGrocery(3, "Sugar", 60);
      displayMessage("1kg Sugar + Vermicelli: 12% Off");
      performAction(ledPin1);
    } else {
      lcd.clear();
      performAction(ledPin2);  
      digitalWrite(buzzerPin, HIGH);
      delay(1000);
      lcd.setCursor(0, 0);
      lcd.print("Unknown  Product");
      performAction(ledPin2); 
    }
  } else {
    if (content.substring(1) == "3D 92 56 62") {
      performAction(ledPin2);
      remGrocery(0, "Milk", 60);
      displayMessage("Milk + Coffee powder: 10% Off");
      performAction(ledPin2);
    } else if (content.substring(1) == "03 8C F9 2F") {
      performAction(ledPin2);
      remGrocery(1, "Toor Dal", 150);
      displayMessage("3kg Dal + Sambar Masala: 15% Off");
      performAction(ledPin2);
    } else if (content.substring(1) == "86 7D D8 FA") {
      performAction(ledPin2);
      remGrocery(2, "Oil", 150);
      displayMessage("1L oil + Mustard: 8% Off");
      performAction(ledPin2);
    } else if (content.substring(1) == "FC 12 44 44") {
      performAction(ledPin2);
      remGrocery(3, "Sugar", 60);
      displayMessage("1kg Sugar + Vermicelli: 12% Off");
      performAction(ledPin2);
    } else {
      lcd.clear();
      performAction(ledPin2);
      digitalWrite(buzzerPin, HIGH);
      delay(1000);
      lcd.setCursor(0, 0);
      lcd.print("Unknown  Product");
      performAction(ledPin2); 
    }
  }
  performAction(ledPin2); 
  lcd.setCursor(0,0);
  displayWeight(); 
  rfid();
  lcd.setCursor(0, 1);
  lcd.print("TotalAmount:");
  lcd.print(total);
  lcd.write((byte)0);
  performAction(ledPin2); 
  
}
void displayWeight() {
  float weight = scale.get_units(5); 
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Weight:");
  lcd.print(weight,2);  
  lcd.print("g");       
  delay(1000);           
}

void addGrocery(int index, const char* name, int price) {
    if (index >= 0 && index < NUM_GROCERIES) {
        groceries[index].quantity++;
        total += price; 

        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(name);
        lcd.print(" Added");
        lcd.setCursor(0, 1);
        lcd.print("Price: ");
        lcd.write((byte)0);
        lcd.print(price);
        delay(1000);

        
        float weightBefore = scale.get_units(5);
        delay(500); 
        float weightAfter = scale.get_units(5);

        if (weightAfter > weightBefore) {
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Unscanned Item!");
            delay(2000); 
        }

        digitalWrite(relay2, HIGH);
        digitalWrite(ledPin1, HIGH);  
        delay(1000);
        digitalWrite(relay2, LOW);
        digitalWrite(ledPin1, LOW);  
    }
}

void remGrocery(int index, const char* name, int price) {
    if (index >= 0 && index < NUM_GROCERIES) {
        groceries[index].quantity--;
        total -= price;  

        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(name);
        lcd.print(" Removed");
        lcd.setCursor(0, 1);
        lcd.print("Price: ");
        lcd.write((byte)0);
        lcd.print(price);
        delay(1000);

        float weightBefore = scale.get_units(5);
        delay(500); 
        float weightAfter = scale.get_units(5);

        if (weightBefore > weightAfter) {
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Unscanned Item!");
            delay(2000); 
        }
        digitalWrite(relay2, HIGH);
        digitalWrite(ledPin2, HIGH);  
        delay(1000);
        digitalWrite(relay2, LOW);
        digitalWrite(ledPin2, LOW);  
    }
}

void performAction(int pin) {
  digitalWrite(pin, HIGH); 
  delay(200);
  digitalWrite(pin, LOW);  
  delay(200);
  digitalWrite(pin, HIGH); 
  digitalWrite(buzzerPin, HIGH);
  delay(200);
  digitalWrite(pin, LOW);  
  digitalWrite(buzzerPin, LOW); 
  delay(200);
}

void displayMessage(const char* message) {
  for (int i = 0; i < strlen(message); i++) {
    lcd.clear();
    lcd.setCursor(0, 1);
    lcd.print(message + i);
    delay(250);
  }
}

void sendAllGroceries() {
  String message = "Grocery List:\n";
  for (int i = 0; i < NUM_GROCERIES; i++) {
    if (groceries[i].quantity > 0) {
      message += groceries[i].name + ": " + String(groceries[i].quantity) + " Qty, ";
    }
  }
  message += "\nTotal Amount: " + String(total);
  sendSMSResponse(message.c_str());
}

void check() {
  if (Serial.available()) {
    String str = Serial.readString();
    if (str.startsWith("Cost")) {
      sendSMSResponse("Your Total Bill Was", String(total).c_str());
    }
  }
}
void sendSMSResponse(const char* message1, const char* message2 = "", const char* message3 = "") {
  if (Serial.availableForWrite()) {
    Serial.println("AT+CMGS=\"+919344459498\""); 
    delay(1000);
    Serial.println(message1);
    delay(1000);
    if (strlen(message2) > 0) {
      Serial.println(message2);
      delay(1000);
    }
    if (strlen(message3) > 0) {
      Serial.println(message3);
      delay(1000);
    }
    Serial.write(26); 
    delay(1000);
  }
}
