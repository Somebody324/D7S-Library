#include <D7S.h>
#include <LoRa.h>

bool earthquakeState = false; 
int consecutiveEarthquakeReadings = 0;
int consecutiveStrongReading = 0;
const int requiredConsecutiveReadings = 3;

const int buzzer = 6;

unsigned long buzzerPreviousMillis = 0;
const unsigned long buzzerInterval = 500;

void setup() {
  Serial.begin(9600);
  pinMode(buzzer, OUTPUT);

  while (!Serial) {
    ; //wait for it to connect
  }

  //STARTING
  Serial.print("\nStarting D7S communications...");
  D7S.begin();
  //wait for d7s connection
  while (!D7S.isReady()) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("STARTED");

  //SETTINGS
  //setting the D7S to switch the axis at initialization time
  Serial.println("Setting D7S sensor to switch axis at initialization time.");
  D7S.setAxis(SWITCH_AT_INSTALLATION);

  //INITIALIZATION
  Serial.println("Initializing the D7S sensor");
  delay(2000);
  Serial.print("Initializing...");
  D7S.initialize();
  //wait until the D7S is ready
  while (!D7S.isReady()) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("INITIALIZED!");
  Serial.println("\nListening for earthquakes!");
}

void loop() {
  /*
  Serial.print("\n\tInstantaneous SI: ");
  Serial.print(D7S.getInstantaneusSI(), 5);
  Serial.println(" [m/s]");
  */

  Serial.print("\n\tInstantaneous PGA: ");
  Serial.print(D7S.getInstantaneusPGA(), 4);
  Serial.println(" [m/s^2]");

  Serial.print("\tIntensity Level: ");
  Serial.println(D7S.getIntensity());

  //checks if there is an earthquake
  if (D7S.getInstantaneusPGA() >= 0.01) {
    consecutiveEarthquakeReadings++;
    if (consecutiveEarthquakeReadings >= requiredConsecutiveReadings) {
      earthquakeState = true;
      consecutiveEarthquakeReadings = 0;
    }
  }
  else{
    consecutiveEarthquakeReadings = 0;
    earthquakeState = false;
  }

  //checks if the earthquake is strong before setting off the alarm
  if (earthquakeState) {
    Serial.println("An Earthquake has been Detected");
    if (D7S.getInstantaneusPGA() >= 0.05){
      consecutiveStrongReading++;
      unsigned long currentMillis = millis();
      
      if (consecutiveStrongReading >= requiredConsecutiveReadings) {
        if (currentMillis - buzzerPreviousMillis >= buzzerInterval) {
          //save the last time you toggled the buzzer
          buzzerPreviousMillis = currentMillis;
          
          //if the buzzer is off, turn it on, and vice versa
          if (digitalRead(buzzer) == LOW) {
            tone(buzzer, 5); // Adjust frequency as needed
          } else {
            noTone(buzzer);
          }
        }
      } else {
        noTone(buzzer);
        buzzerPreviousMillis = currentMillis; // Reset the timing
      }
    }
    else{
      consecutiveStrongReading = 0;
      noTone(buzzer);
    }
  } else {
    noTone(buzzer);
  }

  //wait 500ms before checking again
  delay(500);
}
