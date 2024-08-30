#include "D7S.h"

//constructor/destroyer
D7SClass::D7SClass() {
   for (int i = 0; i < 4; i++) {
      _handlers[i] = NULL;
   }

   _events = 0;
}

//Begin
//initialize Wire
void D7SClass::begin() {
   WireD7S.begin();
}

//Status
//return the state
d7s_status D7SClass::getState() {
   return (d7s_status) (read8bit(0x10, 0x00) & 0x07);
}

d7s_axis_state D7SClass::getAxisInUse() {
   return (d7s_axis_state) (read8bit(0x10, 0x01) & 0x03);
}

//Settings
//threshold of the sensor (0x00 = standard | 0x01 = sensitive)
void D7SClass::setThreshold(d7s_threshold threshold) {
   //check threshold validity
   if (threshold < 0 || threshold > 1) {
      return;
   }
   uint8_t reg = read8bit(0x10, 0x04);
   //new register value with the threshold
   reg = (((reg >> 4) << 1) | (threshold & 0x01)) << 3;
   //update register
   write8bit(0x10, 0x04, reg);
}

//change the axis selection mode
void D7SClass::setAxis(d7s_axis_settings axisMode) {
   //check if axisMode is valid
   if (axisMode < 0 or axisMode > 4) {
      return;
   }
   //read the CTRL register at 0x1004
   uint8_t reg = read8bit(0x10, 0x04);
   //new register value with the threshold
   reg = (axisMode << 4) | (reg & 0x0F);
   //update register
   write8bit(0x10, 0x04, reg);
}

//instantaneus data
//get instantaneus PGV (during an earthquake) (m/s)
float D7SClass::getInstantaneusPGV() {
   return ((float) read16bit(0x20, 0x00)) / 1000;
}

//get instantaneus PGA (during an earthquake) (m/s^2)
float D7SClass::getInstantaneusPGA() {
   return ((float) read16bit(0x20, 0x02)) / 1000;
}

//identify the intensity 
uint8_t D7SClass::getIntensity() {
    float pga = getInstantaneusPGA();

    //define thresholds for the PHIVOLCS intensity scale
    if (pga == 0) {return 0;}                           //Intensity 0
    if (pga > 0 && pga < 0.01) {return 1;}              //Intensity I
    else if (pga >= 0.01 && pga < 0.02) {return 2;}     //Intensity II
    else if (pga >= 0.02 && pga < 0.05) {return 3;}     //Intensity III
    else if (pga >= 0.05 && pga < 0.10) {return 4;}     //Intensity IV
    else if (pga >= 0.10 && pga < 0.25) {return 5;}     //Intensity V
    else if (pga >= 0.25 && pga < 0.50) {return 6;}     //Intensity VI
    else if (pga >= 0.50 && pga < 1.00) {return 7;}     //Intensity VII
    else if (pga >= 1.00 && pga < 2.50) {return 8;}     //Intensity VIII
    else if (pga >= 2.50 && pga < 5.00) {return 9;}     //Intensity IX
    else{
        return 10;}                                     //Intensity X
}

//clear memory
//delete initializzazion data
void D7SClass::clearInstallationData() {
   write8bit(0x10, 0x05, 0x08);
}

//delete offset data
void D7SClass::clearLastestOffsetData() {
   write8bit(0x10, 0x05, 0x04);
}

//delete selftest data
void D7SClass::clearSelftestData() {
   write8bit(0x10, 0x05, 0x02);
}

//delete all data
void D7SClass::clearAllData() {
   write8bit(0x10, 0x05, 0x0F);
}

//initialization
//initialize the d7s
void D7SClass::initialize() {
   write8bit(0x10, 0x03, 0x02);
}

//selftest
//(OK/ERROR)
void D7SClass::selftest() {
   write8bit(0x10, 0x03, 0x04);
}

//self-diagnostic test (OK/ERROR)
d7s_mode_status D7SClass::getSelftestResult() {
   return (d7s_mode_status) ((read8bit(0x10, 0x02) & 0x07) >> 2);
}

//offset aquisition
//start offset acquisition and return the rersult (OK/ERROR)
void D7SClass::acquireOffset() {
   write8bit(0x10, 0x03, 0x03);
}

//return the result of offset acquisition test (OK/ERROR)
d7s_mode_status D7SClass::getAcquireOffsetResult() {
   return (d7s_mode_status) ((read8bit(0x10, 0x02) & 0x0F) >> 3);
}

//shutoff event
uint8_t D7SClass::isInCollapse() {
   readEvents();
   return (_events & 0x02) >> 1;
}

//return true if the shutoff condition is met
uint8_t D7SClass::isInShutoff() {
   readEvents();
   return _events & 0x01;
}

//reset shutoff/collapse events
void D7SClass::resetEvents() {
   read8bit(0x10, 0x02);
   _events = 0;
}

//earthquake detection
//return true if an earthquake is occuring
uint8_t D7SClass::isEarthquakeOccuring() {
   return getState() == NORMAL_MODE_NOT_IN_STANBY;
}

//sensor ready
uint8_t D7SClass::isReady() {
   return getState() == NORMAL_MODE;
}

//interrupt
//enable interrupt INT1 on specified pin
void D7SClass::enableInterruptINT1(uint8_t pin) {
   pinMode(pin, INPUT_PULLUP);
   attachInterrupt(digitalPinToInterrupt(pin), isr1, FALLING);
}

//enable interrupt INT2 on specified pin
void D7SClass::enableInterruptINT2(uint8_t pin) {
   pinMode(pin, INPUT_PULLUP);
   attachInterrupt(digitalPinToInterrupt(pin), isr2, CHANGE);
}

//start interrupt handling
void D7SClass::startInterruptHandling() {
   _interruptEnabled = 1;
}

//stop interrupt handling
void D7SClass::stopInterruptHandling() {
   _interruptEnabled = 0;
}

//assing the handler to the specific event
void D7SClass::registerInterruptEventHandler(d7s_interrupt_event event, void (*handler) ()) {
   if (event < 0 || event > 3) {
      return;
   }
   _handlers[event] = handler;
}

void D7SClass::registerInterruptEventHandler(d7s_interrupt_event event, void (*handler) (float, float, float)) {
  registerInterruptEventHandler(event, (void (*)()) handler);
}


//do not touch
//read 8 bit from the specified register
uint8_t D7SClass::read8bit(uint8_t regH, uint8_t regL) {

   //debug
   #ifdef DEBUG
      Serial.println("--- read8bit ---");
      Serial.print("REG: 0x");
      Serial.print(regH, HEX);
      Serial.println(regL, HEX);
   #endif

   //setting up i2c connection (if you wont use the default i2c pins)
   WireD7S.beginTransmission(D7S_ADDRESS);
   
   //write register address
   WireD7S.write(regH); 
   delay(10); 
   WireD7S.write(regL); 
   delay(10); 
   
   uint8_t status = WireD7S.endTransmission(false);

   //debug
   #ifdef DEBUG
      Serial.print("[RE-START]: ");
      Serial.println(status);
   #endif

   //if the status != 0 there is an error
   if (status != 0) {
      //retry
      return read8bit(regH, regL);
   }

   //request 1 byte
   WireD7S.requestFrom(D7S_ADDRESS, 1);
   //wait until the data is received
   while (WireD7S.available() < 1)
      ;
   //read the data
   uint8_t data = WireD7S.read();

   #ifdef DEBUG
      Serial.println("--- read8bit ---");
   #endif
  
   return data;
}

//read 16 bit from the specified register
uint16_t D7SClass::read16bit(uint8_t regH, uint8_t regL) {

   //DEBUG
   #ifdef DEBUG
      Serial.println("--- read16bit ---");
      Serial.print("REG: 0x");
      Serial.print(regH, HEX);
      Serial.println(regL, HEX);
   #endif

   //setting up i2c connection
   WireD7S.beginTransmission(D7S_ADDRESS);
   
   //write register address
   WireD7S.write(regH); //register address high
   delay(10); //delay to prevent freezing
   WireD7S.write(regL); //register address low
   delay(10); //delay to prevent freezing
   
   //send RE-START message
   uint8_t status = WireD7S.endTransmission(false);

   //DEBUG
   #ifdef DEBUG
      Serial.print("[RE-START]: ");
      //send RE-START message
      Serial.println(status);
   #endif

   //if the status != 0 there is an error
   if (status != 0) {
      return read16bit(regH, regL);
   }

   //request 2 byte
   WireD7S.requestFrom(D7S_ADDRESS, 2);
   //wait until the data is received
   while (WireD7S.available() < 2)
      ;
   //read the data
   uint8_t msb = WireD7S.read();
   uint8_t lsb = WireD7S.read();

   #ifdef DEBUG
      Serial.println("--- read16bit ---");
   #endif

   return (msb << 8) | lsb;
}

//write 8 bit to the register specified
void D7SClass::write8bit(uint8_t regH, uint8_t regL, uint8_t val) {
   //DEBUG
   #ifdef DEBUG
      Serial.println("--- write8bit ---");
   #endif

   //i2c connection
   WireD7S.beginTransmission(D7S_ADDRESS);
   
   //write register address
   WireD7S.write(regH); //register address high
   delay(10); //delay to prevent freezing
   WireD7S.write(regL); //register address low
   delay(10); //delay to prevent freezing
   
   //write data
   WireD7S.write(val);
   delay(10);
   uint8_t status = WireD7S.endTransmission(true);

   //DEBUG
   #ifdef DEBUG
      Serial.print("[STOP]: ");
      Serial.println(status);
      Serial.println("--- write8bit ---");
   #endif
}

//reading events
void D7SClass::readEvents() {
   uint8_t events = read8bit(0x10, 0x02) & 0x03;
   _events |= events;
}

//interrupt
void D7SClass::int1() {  //int1 events
   interrupts();
   if (_interruptEnabled) {
      if (isInShutoff()) {
         if (_handlers[2]) {
            _handlers[2](); //shutoff
         }
      } else {
         if (_handlers[3]) {
            _handlers[3](); //collapse
         }
      }
   }
}

void D7SClass::int2() {  //int2 events
   interrupts();
   if (_interruptEnabled) {
      if (isEarthquakeOccuring()) {
         if (_handlers[0]) {
            _handlers[0](); //START_EARTHQUAKE EVENT
         }
      } else { //earthquake ended
         if (_handlers[1]) {
            ((void (*)(float, float, float)) _handlers[1])(getLastestSI(0), getLastestPGA(0), getLastestTemperature(0)); //END_EARTHQUAKE EVENT
         }
      }
   }
}

//it handle the FALLING event that occur to the INT1 D7S pin 
void D7SClass::isr1() {
   D7S.int1();
}

//it handle the CHANGE event thant occur to the INT2 D7S pin 
void D7SClass::isr2() {
   D7S.int2();
}

D7SClass D7S;
