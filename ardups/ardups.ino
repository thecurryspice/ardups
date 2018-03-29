#include "powerDriver.h"
#include "rollAvg.h"
#include "timer.h"
#include "definitions.h"

int current, battVolts, supplyVolts, battCurrent, measureTimer = 1000;
uint64_t lastReadAt = 0;
float outPower, maxPower = 7000;
String inputString = "";
boolean stringComplete, mainsOn;

// default Timer params
Timer shutdown(0);
Timer startup(0);

RollAvg avgPowerOut;
RollAvg avgCurrent;

PowerDriver PowerDrive(MOSPIN, 10, 12000);

// sets up a 31.25 kHz PWM on Timer2
void setupFastPWM(uint16_t n)
{
    // COM2x1 : Clear OC2x on Compare Match, set OC2x at BOTTOM
    // WGM21|WGM20 : Fast PWM mode
    TCCR2A = _BV(COM2A1) | _BV(COM2B1) | _BV(WGM21) | _BV(WGM20);
    
    if(n == 1)
    {
        // clear preescalar bits
        TCCR2B &= 0b11111000;
        // prescalar 1
        TCCR2B |= 0b00000001;
        n = 1;
    }
    else if(n == 2)
    {
        // clear preescalar bits
        TCCR2B &= 0b11111000;
        // prescalar 8
        TCCR2B |= 0b00000010;
        n = 8;
    }
    else if(n == 3)
    {
        // clear preescalar bits
        TCCR2B &= 0b11111000;
        // prescalar 64
        TCCR2B |= 0b00000011;
        n = 64;
    }
    else if(n == 4)
    {
        // clear preescalar bits
        TCCR2B &= 0b11111000;
        // prescalar 256
        TCCR2B |= 0b00000100;
        n = 256;
    }
    else if(n == 5)
    {
        // clear preescalar bits
        TCCR2B &= 0b11111000;
        // prescalar 1024
        TCCR2B |= 0b00000101;
        n = 1024;
    }
    Serial.print("Setting PWM frequency to ");
    // n/2 + 1 gives the time period in us
    Serial.print(String(62500.0/n));
    Serial.println(" Hz");
    // give some time for PWM to stabilise
    delay(1);
}

// extracts configuration state
void config()
{
    // shutdown schedule
    uint64_t timeToSD = shutdown.ticksLeft();
    //Serial.println(timeToSD);
    // for storing output message
    String outString;
    if(timeToSD)
    {
        int hrs = timeToSD/3600000;
        int mins = (timeToSD % 3600000)/60000;
        int secs = (timeToSD % 60000)/1000;
        int ms = timeToSD % 1000;
        outString = "Scheduled at "+String(hrs)+" hours, "+String(mins)+" minutes, "+String(secs)+"."+String(ms)+" seconds from now\n";
    }
    Serial.print(F("Shutdown: "));
    Serial.print((timeToSD == 0) ?"Not Scheduled\n":outString);
    // startup schedule
    uint64_t timeToSU = startup.ticksLeft();
    if(timeToSU)
    {
        int hrs = timeToSU/3600000;
        int mins = (timeToSU % 3600000)/60000;
        int secs = (timeToSU % 60000)/1000;
        int ms = timeToSU % 1000;
        outString = "Scheduled at "+String(hrs)+" hours, "+String(mins)+" minutes, "+String(secs)+"."+String(ms)+" seconds from now\n";
    }
    Serial.print(F("Startup: "));
    Serial.print((timeToSU == 0) ?"Not Scheduled\n":outString);
    // measure interval
    Serial.print(F("Monitoring Interval: "));
    Serial.print(measureTimer);
    Serial.print(F(" ms\nPower Limiting: "));
    // Power limiting
    Serial.println((maxPower)?("Enabled: ")+String(maxPower)+(" mW"):"Disabled");
}

// will run after every loop
void serialEvent()
{
    while(Serial.available())
    {
        // some delay is necessary, otherwise the received command might break up into characters
        delay(50);
        char inChar = (char)Serial.read();
        // using newline as delimiter
        if (inChar == '\n')
        {
            stringComplete = true;
            break;
        }
        inputString += inChar;
    }
    if(inputString == "")
    {
        // if it was just a 'return' key, don't mark inputString as complete
        stringComplete = false;
    }
}

// edits variables for one measure cycle (one loop)
void measure(bool printReadings)
{
    // update all variables
    current =  CURRSENS*2.2;
    // keep this format to not let the register value exceed the maximum float value
    battVolts = float(1609.7*3.3/1023)*BATTVOLT;
    supplyVolts = float(1609.7*3.3/1023)*SUPPLYVOLT;

    outPower = (float(supplyVolts)/1000)*current;
    battCurrent = (float(supplyVolts)/battVolts)*current;
    
    // update rolling averages
    avgCurrent.push(current);
    avgPowerOut.push(outPower);
    
    if(printReadings)
    {
        Serial.print(F("Battery Voltage: "));
        Serial.print(battVolts);
        Serial.print(F("mV\t"));
        Serial.print(F("Output Voltage: "));
        Serial.print(supplyVolts);
        Serial.print(F("mV\t"));
        Serial.print(F("Current: "));
        Serial.print(current);
        Serial.print(F("mA\t"));
        Serial.print(F("Avg Current: "));
        Serial.print(avgCurrent.getAvg());
        Serial.print(F("mA\t"));
        Serial.print(F("Power: "));
        Serial.print(outPower);
        Serial.print(F("mW\t"));
        Serial.print(F("Avg Power: "));
        Serial.print(avgPowerOut.getAvg());
        Serial.print(F("mW\t"));
        Serial.println(PowerDrive.getDuty());
    }
}

// cli manager
void manCli()
{
    // help
    if(inputString.equalsIgnoreCase("help") || inputString.equals("?"))
    {
        Serial.println(F("stats             :   show power metrics and statistics"));
        Serial.println(F("config            :   show configuration"));
        Serial.println(F("resetStats        :   reset all statistics"));
        Serial.println(F("shutdown <sec>    :   disrupts power after specified seconds"));
        Serial.println(F("startup <sec>     :   supplies power after specified seconds after shutdown"));
        Serial.println(F("limitPower <mW>   :   limits power to specified mW in CV mode"));
        Serial.println(F("responseTime <ms> :   determines update rate of output power control loop"));
        Serial.println(F("readInterv <ms>   :   sets interval between two readings"));
        Serial.println(F("dnd <on/off>      :   turn off all indicator lights except mains"));
        Serial.println(F("PWMfreq <1-5>     :   sets switching frequency of the output driver"));

        //Serial.println(F("setTime <hh:mm>   :   set local time"));
        //Serial.println(F("setPwrAvgTime     :   set sample space (minutes) for average power"));
        //Serial.println(F("setCurrAvgTime    :   set sample space (minutes) for average current"));
        Serial.println(F("help <?>          :   display this message"));
    }

    // stats
    else if(inputString.equalsIgnoreCase("stats"))
    {
        Serial.print(F("Status: "));
        Serial.print(BATTCHRGPIN?"On Mains":"Discharging");
        Serial.print(F("\nOutput Voltage: "));
        Serial.print(supplyVolts);
        Serial.print(F(" mV\nOutput Current: "));
        Serial.print(current);
        Serial.print(F(" mA\nPower Draw: "));
        Serial.print(outPower);
        Serial.print(F(" mW\nBattery Voltage: "));
        Serial.print(battVolts);
        Serial.print(F(" mV\nBattery Load: "));
        Serial.print(battCurrent);
        Serial.print(F(" mA\nAverage Power Supply: "));
        Serial.print(avgPowerOut.getAvg());
        Serial.print(F(" mW\nAverage Current Draw: "));
        Serial.print(avgCurrent.getAvg());
        Serial.println(F(" mA"));
    }

    // resetStats
    else if((inputString.substring(0,10)).equalsIgnoreCase("resetStats"))
    {
        avgPowerOut.reset();
        avgCurrent.reset();
        Serial.println("All buffers cleared");
    }

    // config
    else if(inputString.equalsIgnoreCase("config"))
        {config();}

    // shutdown
    else if((inputString.substring(0,8)).equalsIgnoreCase("shutdown"))
    {
        uint64_t secs = 0;
        uint8_t length = inputString.length();
        secs = abs((inputString.substring(9,length)).toInt());
        if(!secs)
        {
            shutdown.resetTimer();
            Serial.println("Shutdown timer cleared");
        }
        else
        {
            shutdown.setTimer(1000*secs);
            Serial.println(F("Shutdown timer set"));
        }
    }

    // startup
    else if((inputString.substring(0,7)).equalsIgnoreCase("startup"))
    {
        // check whether shutdown has been scheduled
        if(shutdown.ticksLeft())
        {
            uint8_t length = inputString.length();
            uint64_t secs = abs((inputString.substring(8,length).toInt()));
            if(!secs)
            {
                startup.resetTimer();
                Serial.println(F("Startup timer cleared"));
            }
            else
            {
                startup.setTimer(1000*secs + shutdown.ticksLeft());
                Serial.println(F("Startup timer set"));
            }
        }
        else
            Serial.println(F("Set Shutdown timer first"));
    }

    // power limiting
    else if((inputString.substring(0,10)).equalsIgnoreCase("limitPower"))
    {
        uint8_t length = inputString.length();
        int _maxPower = abs((inputString.substring(11,length).toInt()));
        if(!_maxPower)
        {
            PowerDrive.setPowerLimit(12000);
            Serial.println(F("Output Power Limit Disabled"));
            maxPower = 0;
        }
        else if(_maxPower >= 3000 && _maxPower <= 12000)
        {
            maxPower = _maxPower;
            PowerDrive.setPowerLimit(_maxPower);
            Serial.println(F("Output Power Limit Enabled"));
        }
        else
            Serial.println(F("Output Power Limit could not be enabled\nValid Range: 3000 - 12000 mW"));
    }

    // update rate of output power control loop
    else if((inputString.substring(0,12)).equalsIgnoreCase("responseTime"))
    {
        uint8_t length = inputString.length();
        float _responseTime = abs((inputString.substring(13,length).toInt()));
        if(_responseTime < 3 || _responseTime > 100)
            Serial.println(F("Valid Range: 3 - 100 ms"));
        else
        {
            PowerDrive.setResponseTime(_responseTime);
            Serial.println(F("Response Time Set"));
        }
    }

    // measure interval
    else if((inputString.substring(0,10)).equalsIgnoreCase("readInterv"))
    {
        uint8_t length = inputString.length();
        if((inputString.substring(11,length)).equalsIgnoreCase("realtime"))
        {
            while(!Serial.available())
            {
                measure(true);
                PowerDrive.update();
            }
        }
        else
        {
            float _measureTimer = abs((inputString.substring(11,length).toInt()));
            if(_measureTimer < 250)
                Serial.println(F("Valid Range: Above 250 milliseconds"));
            else
            {
                measureTimer = _measureTimer;
                Serial.println(F("Reading Interval Changed"));
            }
        }
    }

    // set switching frequency
    else if((inputString.substring(0,7)).equalsIgnoreCase("PWMfreq"))
    {
        uint8_t length = inputString.length();
        if((inputString.substring(8,length)) == '?')
        {
            Serial.println(F("Warning!\nChanging these values can result in unexpected instability.\nProceed with caution."));
            Serial.println(F("High frequency ==> Decreased Efficiency, Increased Stability"));
            Serial.println(F("Low frequency ==> Increased Efficiency, Decreased Stability"));
            Serial.println(F("1. 62.5 kHz\n2. 7.81 kHz\n3. 976.56 Hz\n4. 244.14 Hz\n5. 61.04 Hz"));
        }
        else
        {
            int n = abs((inputString.substring(8,length)).toInt());
            setupFastPWM(constrain(n, 1, 5));
        }
    }

    // dnd
    else if((inputString.substring(0,3)).equalsIgnoreCase("dnd"))
    {
        // the actual use of this function is to disable interrupts being sent over to the Pi.
        // there are no interrupts right now.
        uint8_t length = inputString.length();
        if((inputString.substring(4,length)).equalsIgnoreCase("on"))
        {
            PowerDrive.indicator(false);
            Serial.println(F("DND Enabled"));
        }
        if((inputString.substring(4,length)).equalsIgnoreCase("off"))
        {
            PowerDrive.indicator(true);
            Serial.println(F("DND Disabled"));
        }
    }

    // default
    else
        Serial.println(F("Unrecognised Command!"));

    // leave a one line space
    Serial.println();
}

void setup()
{
    pinMode(CONSTVOLT, OUTPUT);
    pinMode(MOSPIN, OUTPUT);
    setupFastPWM(1);
    PowerDrive.setDuty(0);
    Serial.begin(115200);
    Serial.println(F("Preparing Buffers..."));
    for(int i = 0; i < 64; i++)
        measure(false);
    inputString.reserve(50);
    Serial.println(F("Done\nPowering up..."));
    PowerDrive.setDuty(200);
    PowerDrive.setPowerLimit(maxPower);
    Serial.println(F("Done"));
    config();
}

void loop()
{
    if(stringComplete)
    {
        Serial.print(">>");
        Serial.println(inputString);
        manCli();
    }
    if(millis() - lastReadAt > measureTimer)
    {
        measure(false);
        lastReadAt = millis();
    }
    PowerDrive.update();
    if(shutdown.timeout())
    {
        Serial.println(F("Shutting Down Power..."));
        PowerDrive.setDuty(0);
        Serial.println(F("Done"));
        shutdown.resetTimer();
        // find timeToSU
        uint64_t timeToSU = startup.ticksLeft();
        String outString;
        if(timeToSU)
        {
            int hrs = timeToSU/3600000;
            int mins = (timeToSU % 3600000)/60000;
            int secs = (timeToSU % 60000)/1000;
            int ms = timeToSU % 1000;
            outString = "Scheduled at "+String(hrs)+" hours, "+String(mins)+" minutes, "+String(secs)+"."+String(ms)+" seconds from now\n";
        }
        // display timeToSU
        Serial.print(F("Startup: "));
        Serial.print((timeToSU == 0) ?"Not Scheduled\n":outString);
        while(!startup.timeout());
        startup.resetTimer();
        Serial.println(F("Powering up..."));
        PowerDrive.setDuty(200);
        Serial.println(F("Done"));
        config();
    }
    inputString = "";
    stringComplete = false;
    serialEvent();
}