/*****************************************************************************
 *  CastleLinkLive Library for Arduino - CastleLinkLive.h
 *  Copyright (C) 2012  Matteo Piscitelli (matteo@picciux.it)
 *  Rewritten low-level from a high-level work of Capaverde at rcgroups.com
 * 
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *  For further info, check http://code.google.com/p/castlelinklive4arduino/
 *
 *  SAFETY NOTICE
 *  Always keep in mind that an electric motor can be dangerous for you, for 
 *  people and for things. It can start at any time if there is power. 
 *  Castle Creations ESC are very good ones, and have many security
 *  strategies to avoid accidental and unwanted motor start.
 *  This program and CastleLinkLive library also try to keep things as safe as
 *  possible, but using them together with an Arduino (or similar) board 
 *  connected to an electric power system adds another possible point of 
 *  failure to your motor control chain.
 *  So please stay always on the safe side. If you have any doubts, ask
 *  other modelers to help.
 *  It's your responsibility to keep things safe. Developers of this software
 *  can't be considered liable for any possible damage will result from its use.
 *  
 *  SVN $Id$ 
 ******************************************************************************/

/** \file CastleLinkLive.h
    \brief Main libray header file. Include it in your program to enable and use
    the library (if you installed the library in Arduino IDE and choose "Sketch|Import Library..."
    the IDE will automatically include it for you)
*/

/** \mainpage 
    \section Intro
    \copydetails CastleLinkLiveLib
    
    \section Details
    Since CastleLinkLive protocol is based on precise time measurements,
    the library makes extensive use of hardware timers and interrupts.
    As a result, some standard arduino functions won't work as expected:
    - PWM will not work on any pin controlled by the timer used by the library 
      (on ATmega168/328 Arduinos, these are pin 9 and 10) and related functions
      (i.e. analogWrite) must not be called on these pins.
    - ExternalInterrupt capable pins (on ATmega168/328 Arduinos they are pins 2 and 3)
      will be used by the library to drive and receive data from the ESC(s), so they 
      must not be used by the program.
    - One pin is used to read throttle signal generated by a standard RC receiver, 
      and must not be used by the program. However that pin can be chosen (almost) 
      arbitrary.
    - The led pin (usually pin 13 on Arduino) is driven by the library, unless globally 
      disabled at compile time for the library. The led will blink at increasing speed
      based on detected/generated throttle level, and will glow steadily at full throttle.
    Global library compile time configurations are kept in CastleLinkLive_config.h
    
    \section Installation
    Copy directory "CastleLinkLive" in your arduino system-wide library folder or your
    sketchbook library folder and restart Arduino software: you'll find a "CastleLinkLive"
    library under Arduino menu "Sketch|Import Library...".
    
    \section Usage
    See \link CastleLinkLiveLib main library class documentation \endlink for usage
    
    Under "File|Examples|CastleLinkLive" menu in your Arduino software you can also find
    some example programs.
    
    \section License
     This library is free software: you can redistribute it and/or modify
     it under the terms of the GNU General Public License as published by
     the Free Software Foundation, either version 3 of the License, or
     (at your option) any later version. See full license details at 
     http://www.gnu.org/licenses/gpl.html
    
*/

#ifndef CastleLinkLive_h
#define CastleLinkLive_h

#ifndef _STDDEF_H
#include "stddef.h"
#endif
#include "pins_arduino.h"
#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif


#include "CastleLinkLive_config.h"

#if defined (__AVR_ATmega168__) || defined(__AVR_ATmega328P__)
#define MAX_ESCS 2
#elif defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
//#define MAX_ESCS 2
#error "Arduino MEGA is not supported ATM"
#elif defined (__AVR_ATmega8__)
//#define MAX_ESCS 1
#error "Old Arduinos ATmega8-based are not supported ATM"
#else
#define MAX_ESCS 0
#error "MCU not supported"
#endif

/** \cond */
#define LIBRARY_VERSION 0.1.0
/** \endcond */

#ifndef MAX_ESCS
#define MAX_ESCS 2
#endif

/** \name CastleLinkLive Data Frames Identifiers

    CastleLinkLive protocol provides telemetry data by sending it back subdivided in 
    various sequential data frames, each frame corresponding to a specific telemetry value.
    These macros are used as identifiers for the various data frames.
*/
/**@{*/
#define FRAME_RESET            -1  /**< \brief frame identifier for the reset frame. 

                                         This special frame contains no data, and is used to signal
                                         the start of a data sequence */
#define FRAME_REFERENCE        0   /**< \brief frame identifier for reference-time frame 
                                         
                                         This special frame is used as a reference for calibrating
                                         times: it contains the 1-unit time as provided by the ESC */
#define FRAME_VOLTAGE          1   /**< \brief frame identifier for voltage frame */
#define FRAME_RIPPLE_VOLTAGE   2   /**< \brief frame identifier for ripple frame */
#define FRAME_CURRENT          3   /**< \brief frame identifier for current frame */
#define FRAME_THROTTLE         4   /**< \brief frame identifier for throttle frame */
#define FRAME_OUTPUT_POWER     5   /**< \brief frame identifier for output power frame */
#define FRAME_RPM              6   /**< \brief frame identifier for rpm frame */
#define FRAME_BEC_VOLTAGE      7   /**< \brief frame identifier for BEC voltage frame */
#define FRAME_BEC_CURRENT      8   /**< \brief frame identifier for BEC current frame */
#define FRAME_TEMP1            9   /**< \brief frame identifier for temperature1 frame */
#define FRAME_TEMP2           10   /**< \brief frame identifier for temperature2 frame */

#define DATA_FRAME_CNT        11   /**< \brief number of data frames (not counting the reset frame) */
/**@}*/

/** \brief This value can be used as "throttlePinNumber" argument to begin function
    to indicate that library itself has to generate throttle signal
    
    This is also the default value if you call begin without provinding 
    the second parameter.
    @see CastleLinkLiveLib::begin(uint8_t nESC)
    @see CastleLinkLiveLib::begin(uint8_t nESC, int throttlePinNumber)
*/
#define GENERATE_THROTTLE     -1

/** \brief Structure for ESC telemetry data time measurements.
    
    This structures stores time measurements representing
    telemetry data, as provided by the ESC. Complete human readable
    data is derived by calculations based on this.
    
    See CastleLinkLiveLib::getData(uint8_t index,CASTLE_ESC_DATA *dataHolder) for calculation details.
    
    @see uint8_t CastleLinkLiveLib::getData (uint8_t index, CASTLE_RAW_DATA *dataHolder) 
*/
typedef struct castle_raw_data_struct {
  
   /** \brief Array containing time (timer ticks) measurements for all ESC data frames
   */
   uint16_t ticks[DATA_FRAME_CNT];
} CASTLE_RAW_DATA;

/** \brief Structure to hold ESC telemetry data

    This structure holds ESC telemetry data as provided by the ESC itself. It's obtained from
    time-measurement data contained in CASTLE_RAW_DATA structure by calculations executed when the
    program calls uint8_t CastleLinkLiveLib::getData (uint8_t index, CASTLE_ESC_DATA *dataHolder)
    @see uint8_t CastleLinkLiveLib::getData (uint8_t index, CASTLE_ESC_DATA *dataHolder) 
*/
typedef struct castle_esc_data_struct {
  float voltage;         /**< \brief Battery voltage in Volts */
  float rippleVoltage;   /**< \brief Ripple voltage in Volts */
  float current;         /**< \brief Current drawn by motor in Amperes */
  float throttle;        /**< \brief throttle pulse duration as seen by the ESC (in milliseconds) */
  float outputPower;     /**< \brief power level the ESC is driving the motor with. 
                              Value goes from 0.0 for idle to 1.0 for full throttle. */
  float RPM;             /**< \brief Round per minutes the motor is spinning at. This values 
  represents ELECTRICAL rpm, that are not shaft/prop rpm.
  
  You can calculate shaft rpm with following formula:
                              \f[ sRPM = eRPM / MP * 2 \f]
      
  where: 
  
  \f$ sRPM \f$ is shaft rpm,
  
  \f$ eRPM \f$ is electrical rpm,
  
  and \f$ MP \f$ is the number of magnetic poles in the motor. */  
  float BECvoltage;      /**< \brief Voltage at the BEC (Battery Eliminator Circuit) in Volts */
  float BECcurrent;      /**< \brief Current drawn by servos and any other device powered by the BEC in Amperes */
  float temperature;     /**< \brief Temperature of the ESC in degree Celsius */
  
} CASTLE_ESC_DATA;

/** \brief CastleLinkLive4Arduino Library Class

    The library purpose is to get live telemetry data from
    Castle Creations ESCs with CastleLinkLive protocol
    available and enabled (version 2.0).
    
    \warning SAFETY NOTICE
    Always keep in mind that an electric motor can be dangerous 
    for you, for people and for things. It can start at any time if there is power. 
    Castle Creations ESC are very good ones, and have many security
    strategies to avoid accidental and unwanted motor start.
    CastleLinkLive library also try to keep things as safe as
    possible, but using them together with an Arduino (or similar) board 
    connected to an electric power system adds another possible point of 
    failure to your motor control chain.
    So please stay always on the safe side. If you have any doubts, ask
    other modelers to help.
    It's your responsibility to keep things safe. Developers of this software
    can't be considered liable for any possible damage will result from its use.
    
    \version 0.1.0
    \author Matteo Piscitelli
    \date 2012
    \copyright Matteo Piscitelli    
 
*/
class CastleLinkLiveLib {
  public:
   /** \brief Class constructor. A CastleLinkLiveLib object named \em CastleLinkLive \em
       is pre-instantiated by the library, so don't call this. 
       
       Instead, in your program use directly the CastleLinkLive variable as in following code snippet:
       
       \code
       void setup() {
         < program specific setup code >
         CastleLinkLive.init();
         CastleLinkLive.begin(1, GENERATE_THROTTLE); 
         ...
       }
       \endcode
   */
   CastleLinkLiveLib();

   /** \brief Initialization function: should be called during program setup and before any 
       call to "begin(...)".
   */
   void init();
   
   /** \brief Starts the library with default configuration
       Default configuration is:
       - 1 ESC connected
       - Software (library) generated throttle signal
       
       This is equivalent to call 
       \code begin(1, GENERATE_THROTTLE) \endcode
   */
   uint8_t begin();
   
   /** \brief Starts the library indicating the number of ESC connected
       Throttle signal will be software generated by the library
       @param [in] nESC the number of ESC(s) connected (up to 2)

       This is equivalent to call 
       \code begin(nESC, GENERATE_THROTTLE) \endcode
   */
   uint8_t begin(uint8_t nESC);
   
   /** \brief Starts the library indicating the number of ESC connected and the arduino pin that will
       be used to read throttle signal.
       @param [in] nESC the number of ESC(s) connected (up to 2)
       @param [in] throttlePinNumber any valid Arduino pin (except the already used ones) or GENERATE_THROTTLE 
       macro to let the library generate the throttle signal itself.
       @see GENERATE_THROTTLE
   */
   uint8_t begin(uint8_t nESC, int throttlePinNumber);

   /** \brief Starts the library indicating the number of ESC connected, the arduino pin that will
       be used to read throttle signal, minimum and maximum pulse duration for throttle signal
       @param [in] nESC the number of ESC(s) connected (up to 2)
       @param [in] throttlePinNumber any valid Arduino pin (except the already used ones) or GENERATE_THROTTLE 
       macro to let the library generate the throttle signal itself.
       @param [in] throttleMin specifies throttle signal pulse duration corresponding to idle/brake
       (in microseconds: default value is 1000, and can be changed in CastleLinkLive_config.h).
       @param [in] throttleMax specifies throttle signal pulse duration corresponding to full throttle
       (in microseconds: default value is 2000, and can be changed in CastleLinkLive_config.h).
   */
   uint8_t begin(uint8_t nESC, int throttlePinNumber, uint16_t throttleMin, uint16_t throttleMax);
   
   /** \brief Sets throttle value to drive the ESC(s) when in software generated throttle
       
       Sets throttle value to drive the ESC(s) when in software generated throttle
       Beware that, when software generating throttle, as a safety
       measure, CastleLinkLive library expects throttle to be continuosly set 
       at a rate faster than ~1 Hz (1 time per second); failure to do so will 
       result in CastleLinkLive to stop generating throttle signal and raising 
       throttle failure event until setThrottle is called again.
       @see begin
       @see attachThrottlePresenceHandler
       
       @param [in] throttle the throttle level the library should drive the ESC(s) at,
       ranging from 0 (idle/brake) to 100 (full throttle).
   */
   void setThrottle(uint8_t throttle);

   /** \brief Arms the throttle on the library
   
       Until armed, the library will not generate throttle signals (in software
       generated mode) nor bypass external throttle signal (in external throttle mode):
       basically, until the library is armed, ESC(s) will not receive any throttle signal
   */
   void throttleArm();
   
   /** \brief Disarms the throttle on the library   
       @see throttleArm
   */
   void throttleDisarm();
   
   /** \brief Attach a program-defined function to be called by the libray whenever it detects
       throttle signal failure/recovery.
       
       Function has to be void and accept only one parameter, which specifies if
       throttle is present and valid or not.
       
       Sample prototype:       
       \code 
       void throttleEvent(boolean valid)
       \endcode
   */
   void attachThrottlePresenceHandler(void (*ptHandler) (uint8_t) );
   
   /** \brief Gets human-readable parsed data for the index-ESC from the library. This data is
       calculated by time-measurements contained in a CASTLE_RAW_DATA, as detailed in following
       source code.
       
       \snippet CastleLinkLive/CastleLinkLive.cpp ESC data calculation details

       @param [in] index indicates which ESC we want data for. First ESC index is 0
       @param [out] dataHolder is a pointer to a CASTLE_ESC_DATA structure to receive
       the data
       @return 0 if data is not available, or a positive number otherwise
   */
   uint8_t getData(uint8_t index, CASTLE_ESC_DATA *dataHolder);
   
   /** \brief Gets raw data (time measurements) for the index-ESC from the library.

       @param [in] index indicates which ESC we want data for. First ESC index is 0
       @param [out] dataHolder is a pointer to a CASTLE_RAW_DATA structure to receive
       the data
       @return 0 if data is not available, or a positive number otherwise
   */
   uint8_t getData(uint8_t index, CASTLE_RAW_DATA *dataHolder);

#if (LED_DISABLE == 0)
   /** \brief Turns off or on Arduino led. If the throttle is armed the library
       controls the led and this function is silently ignored.
       @see throttleArm
   */
   void setLed(uint8_t on);
#endif

  private:
   int _throttlePinNumber;
   uint8_t _nESC;

   volatile uint8_t *_pcicr;
   uint8_t _pcie;
   volatile uint8_t *_pcmsk;
   uint8_t _pcint;
   volatile uint8_t *_throttlePortModeReg;
   
   volatile uint8_t _throttle;

   void _init_data_structure(int i);
   void _timer_init();
   uint8_t _setThrottlePinRegisters();
   uint8_t _copyDataStructure(uint8_t index, CASTLE_RAW_DATA *dest );
};

/** \brief Global pre-istantiated object to be used by the program */
extern CastleLinkLiveLib CastleLinkLive;

#endif //def CastleLinkLive_h
