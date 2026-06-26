/* IoT Car Parking System with Serial Debugging - Fixed Version */
// Blynk credentials
#define BLYNK_TEMPLATE_ID "TMPL67mBOovAK"
#define BLYNK_TEMPLATE_NAME "Car Parking System"
#define BLYNK_AUTH_TOKEN "R8KzSv0x2KHYfQS0QPcsLy5CHNygLgb1"

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <ESP32Servo.h>
#include <LiquidCrystal_I2C.h>

char auth[] = BLYNK_AUTH_TOKEN;
char ssid[] = "Expecto Patronum";
char pass[] = "expecto1";

// Hardware pins
#define IR_ENTRY 34    // Entry gate sensor
#define IR_EXIT 35     // Exit gate sensor
#define IR_SLOT1 14   // Parking slot 1 sensor
#define IR_SLOT2 27   // Parking slot 2 sensor
#define SERVO_PIN 32   // Servo control pin

LiquidCrystal_I2C lcd(0x27, 16, 2); // I2C LCD
Servo gateServo;

// Blynk virtual pins
#define V_LED_S1 V0
#define V_LED_S2 V1
#define V_Slot_Left V2

#define BLYNK_GREEN "#23C48E"
#define BLYNK_RED "#D3435C"

// Variables for debouncing and gate control
bool gateOpen = false;
unsigned long lastTriggerTime = 0;
const unsigned long gateOpenTime = 3000; // 3 seconds
const unsigned long debounceTime = 500;  // Increased debounce time to 500ms

void setup() {
  Serial.begin(115200);
  Serial.println("System Initializing...");

  // Sensor setup with pull-up resistors
  pinMode(IR_ENTRY, INPUT_PULLUP);
  pinMode(IR_EXIT, INPUT_PULLUP);
  pinMode(IR_SLOT1, INPUT_PULLUP);
  pinMode(IR_SLOT2, INPUT_PULLUP);
  
  // Verify sensors are working (should read HIGH with nothing present)
  Serial.println("Testing sensors...");
  Serial.print("Entry: "); 
  Serial.println(digitalRead(IR_ENTRY));
  Serial.print("Exit: "); 
  Serial.println(digitalRead(IR_EXIT));
  Serial.print("Slot1: "); 
  Serial.println(digitalRead(IR_SLOT1));
  Serial.print("Slot2: "); 
  Serial.println(digitalRead(IR_SLOT2));
  
  if(digitalRead(IR_ENTRY) != HIGH || digitalRead(IR_EXIT) != HIGH) {
    Serial.println("ERROR: Entry/Exit sensors not reading properly!");
    while(1); // Halt if sensors aren't working
  }
  Serial.println("Sensors Initialized");

  // Servo setup
  gateServo.attach(SERVO_PIN);
  gateServo.write(0);
  Serial.println("Servo Initialized (Closed Position)");

  // LCD setup
  lcd.init();
  lcd.backlight();
  lcd.print("Initializing...");
  Serial.println("LCD Ready");

  // Connect to Blynk
  Blynk.begin(auth, ssid, pass);
  lcd.clear();
  lcd.print("Blynk Connected!");
  Serial.println("Blynk Connected!");
  delay(2000);
}

void loop() {
  Blynk.run();
  
  // Read sensors with debouncing
  bool entryTrig = digitalRead(IR_ENTRY) == LOW;
  bool exitTrig = digitalRead(IR_EXIT) == LOW;
  bool slot1 = digitalRead(IR_SLOT1) == LOW;
  bool slot2 = digitalRead(IR_SLOT2) == LOW;
  
  // Debug output
  Serial.print("Sensors - Entry: ");
  Serial.print(entryTrig);
  Serial.print(" Exit: ");
  Serial.print(exitTrig);
  Serial.print(" Slot1: ");
  Serial.print(slot1);
  Serial.print(" Slot2: ");
  Serial.println(slot2);

  // Update Blynk
  Blynk.virtualWrite(V_LED_S1, slot1);
  Blynk.virtualWrite(V_LED_S2, slot2);
  Blynk.setProperty(V_LED_S1, "color", slot1 ? BLYNK_RED : BLYNK_GREEN);
  Blynk.setProperty(V_LED_S2, "color", slot2 ? BLYNK_RED : BLYNK_GREEN);
  
  // Calculate available slots
  int availableSlots = 2 - (slot1 + slot2);
  Blynk.virtualWrite(V_Slot_Left, availableSlots);

  // Gate control logic with debouncing and cooldown
  unsigned long currentTime = millis();
  
  if (!gateOpen) {
    if ((entryTrig && availableSlots > 0) || exitTrig) {
      Serial.print("Trigger condition met! Entry:");
      Serial.print(entryTrig);
      Serial.print(" Exit:");
      Serial.print(exitTrig);
      Serial.print(" Available:");
      Serial.println(availableSlots);
      
      if (currentTime - lastTriggerTime > debounceTime) {
        openGate();
        lastTriggerTime = currentTime;
      }
    }
  } else {
    if (currentTime - lastTriggerTime >= gateOpenTime) {
      closeGate();
    }
  }

  // Update LCD
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Slots Left: ");
  lcd.print(availableSlots);
  
  lcd.setCursor(0, 1);
  lcd.print("S1:");
  lcd.print(slot1 ? "Full " : "Empty");
  lcd.print(" S2:");
  lcd.print(slot2 ? "Full" : "Empty");

  delay(200);
}

void openGate() {
  if (!gateOpen) {
    gateServo.write(90); // Open gate
    gateOpen = true;
    Serial.println("Gate opened");
  }
}

void closeGate() {
  if (gateOpen) {
    gateServo.write(0); // Close gate
    gateOpen = false;
    Serial.println("Gate closed");
  }
}
