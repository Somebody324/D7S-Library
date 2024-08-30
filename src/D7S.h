#ifndef D7S_H
#define D7S_H

#include <Arduino.h>
#include <Wire.h>

//d7s i2c address
#define D7S_ADDRESS 0x55 

//d7s state
typedef enum d7s_status {
   NORMAL_MODE = 0x00,
   NORMAL_MODE_NOT_IN_STANBY = 0x01, //earthquake in progress
   INITIAL_INSTALLATION_MODE = 0x02,
   OFFSET_ACQUISITION_MODE = 0x03,
   SELFTEST_MODE = 0x04
};

//d7s axis settings
typedef enum d7s_axis_settings {
   FORCE_YZ = 0x00,
   FORCE_XZ = 0x01,
   FORXE_XY = 0x02,
   AUTO_SWITCH = 0x03,
   SWITCH_AT_INSTALLATION = 0x04 
};

//axis state
typedef enum d7s_axis_state {
   AXIS_YZ = 0x00,
   AXIS_XZ = 0x01,
   AXIS_XY = 0x02
};

//d7s threshold settings
typedef enum d7s_threshold {
   THRESHOLD_HIGH = 0x00,
   THRESHOLD_LOW = 0x01
};

//message status (selftes, offset acquisition)
typedef enum d7s_mode_status {
   D7S_OK = 0,
   D7S_ERROR = 1
};

//events handled externaly by the using using an handler (the d7s int1, int2 must be connected to interrupt pin)
typedef enum d7s_interrupt_event {
   START_EARTHQUAKE = 0, //int 2
   END_EARTHQUAKE = 1, //int 2
   SHUTOFF_EVENT = 2, //int 1
   COLLAPSE_EVENT = 3 //int 1
};


//class D7S
class D7SClass {

   public: 

      //constructor/destroyer
      D7SClass(); 

      //Begin
      void begin(); 

      //Status
      d7s_status getState(); //return the currect state
      d7s_axis_state getAxisInUse(); //return the current axis in use

      //Settings
      void setThreshold(d7s_threshold threshold); //change the threshold in use
      void setAxis(d7s_axis_settings axisMode); //change the axis selection mode

      //instantaneus data
      float getInstantaneusPGV(); //get instantaneus PGV (during an earthquake) (m/s)
      float getInstantaneusPGA(); //get instantaneus PGA (during an earthquake) (m/s^2)
      uint8_t getIntensity(); //get the intensity of the earthquake

      //clear memory
      void clearInstallationData(); //delete initializzazion data
      void clearLastestOffsetData(); //delete offset data
      void clearSelftestData(); //delete selftest data
      void clearAllData(); //delete all data

      //initialization
      void initialize(); //initialize the d7s (start the initial installation mode)

      //selftest
      void selftest(); //trigger self-diagnostic test
      d7s_mode_status getSelftestResult(); //return the result of self-diagnostic test (OK/ERROR)

      //offset aquisition
      void acquireOffset(); //trigger offset acquisition
      d7s_mode_status getAcquireOffsetResult(); //return the result of offset acquisition test (OK/ERROR)

      //shutoff event
      uint8_t isInCollapse(); //return true if the collapse condition is met (it's the sencond bit of _events)
      uint8_t isInShutoff(); //return true if the shutoff condition is met (it's the first bit of _events)
      void resetEvents(); //reset shutoff/collapse events

      //earthquake detection
      uint8_t isEarthquakeOccuring(); //return true if an earthquake is occuring

      //sensor ready
      uint8_t isReady();

      //interrupt
      void enableInterruptINT1(uint8_t pin); //enable interrupt INT1 on specified pin
      void enableInterruptINT2(uint8_t pin); //enable interrupt INT2 on specified pin
      void startInterruptHandling(); //start interrupt handling
      void stopInterruptHandling(); //stop interrupt handling
      void registerInterruptEventHandler(d7s_interrupt_event event, void (*handler) ()); //assing the handler to the specific event
      void registerInterruptEventHandler(d7s_interrupt_event event, void (*handler) (float, float, float)); //assing the handler to the specific event

   private:
      //handler array (it cointaint the pointer to the user defined array)
      void (*_handlers[4]) ();

      //variable to track event (first bit => SHUTOFF, second bit => COLLAPSE)
      uint8_t _events;

      //enable interrupt handling
      uint8_t _interruptEnabled;

      //read
      uint8_t read8bit(uint8_t regH, uint8_t regL); //read 8 bit from the specified register
      uint16_t read16bit(uint8_t regH, uint8_t regL); //read 16 bit from the specified register

      //write
      void write8bit(uint8_t regH, uint8_t regL, uint8_t val); //write 8 bit to the register specified

      void readEvents(); //read the event (SHUTOFF/COLLAPSE) from the EVENT register

      //event handler
      void int1(); //handle the int 1 events
      void int2(); //handle the int 2 events

      //ISR handler
      static void isr1(); //it handle the FALLING event that occur to the INT1 D7S pin (glue routine)
      static void isr2(); //it handle the CHANGE event thant occur to the INT2 D7S pin (glue routine)
};

extern D7SClass D7S;

#endif
