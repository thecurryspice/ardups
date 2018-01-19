#define MOSPIN 3
#define BATTCHRGPIN 4
#define DEMANDIND 8
#define CURRPIN A0
#define BATTVOLTPIN A1
#define SUPPLYVOLTPIN A2
#define CURRSENS analogRead(CURRPIN)
#define BATTVOLT analogRead(BATTVOLTPIN)
#define SUPPLYVOLT analogRead(SUPPLYVOLTPIN)
#define MOSDRIVE(x) analogWrite(MOSPIN,x)

int current, battVolts, supplyVolts, battCurrent, measureTimer = 1000;
float outPower, maxPower;
String inputString = "";
boolean stringComplete, mainsOn;

// default Timer params
Timer shutdown(0);
Timer startup(0);

RollAvg avgPowerOut(255);
RollAvg avgCurrent(255);

PowerDriver PowerDrive(MOSPIN);

// will run after every loop
void serialEvent()
{
    while(Serial.available())
    {
        char inChar = (char)Serial.read();
        inputString += inChar;
        if (inChar == '\n')
        stringComplete = true;
    }
}

void measure()
{
    current =  CURRSENS;
    battVolts = float(1609.7*BATTVOLT*3.3)/1023;
    supplyVolts = float(1609.7*SUPPLYVOLT*3.3)/1023;

    outPower = (supplyVolts*current)/1000;
    battCurrent = (supplyVolts*current)/battVolts;

    avgCurrent.push(current);
    avgPowerOut.push(outPower);
}

void manCli()
{
    // help
    if(inputString.equalsIgnoringCase("help") || inputString.equals("?"))
    {
        Serial.println(F("shutdown <sec>    :   disrupts power after specified seconds"));
        Serial.println(F("startup <sec>     :   supplies power after specified seconds after shutdown"));
        Serial.println(F("limitPower <mW>   :   limits power to specified mW in CV mode"));
        Serial.println(F("responseTime <ms> :   determines update rate of output power control loop"));
        Serial.println(F("readInterv <ms>   :   sets interval between two readings"));
        Serial.println(F("dnd <bool>        :   turn off all indicator lights except mains"));
        //Serial.println(F("setTime <hh:mm>   :   set local time"));
        //Serial.println(F("setPwrAvgTime     :   set sample space (minutes) for average power"));
        //Serial.println(F("setCurrAvgTime    :   set sample space (minutes) for average current"));
        Serial.println(F("stats             :   show power metrics and statistics"));
        Serial.println(F("config            :   show configuration"));
        Serial.println(F("help <?>          :   display this message"));
    }

    // stats
    else if(inputString.equalsIgnoringCase("stats"))
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
        Serial.print(battCurrent + random(0,50));
        Serial.print(F(" mA\nAverage Power Supply: "));
        Serial.print(avgPowerOut.getAvg());
        Serial.print(F(" mW\nAverage Current Draw: "));
        Serial.print(avgCurrent.getAvg());
        Serial.println(F(" mA"));
    }

    // config
    else if(inputString.equalsIgnoringCase("config"))
    {
        // shutdown schedule
        uint64_t timeToSD = shutdown.ticksLeft();
        String outString = "";
        if(!timeToSD)
        {
            int hrs = timeToSD/3600000;
            int mins = (timeToSD % 3600000)/60000;
            int secs = (timeToSD % 60000)/1000;
            int ms = timeToSD % 1000;
            outString = "Scheduled for "+String(hrs)+"h "+String(mins)+"m "+String(secs)+"."+String(ms)+"s from now"
        }
        Serial.print(F("Shutdown: "));
        Serial.print((timeToSD == 0) ?"Not Scheduled":outString);
        // startup schedule
        timeToSU = startup.ticksLeft();
        timeToSD += timeToSU;
        String outString = "";
        if(!timeToSU)
        {
            int hrs = timeToSD/3600000;
            int mins = (timeToSD % 3600000)/60000;
            int secs = (timeToSD % 60000)/1000;
            int ms = timeToSD % 1000;
            outString = "Scheduled for "+String(hrs)+"h "+String(mins)+"m "+String(secs)+"."+String(ms)+"s from now"
        }
        Serial.print(F("Startup: "));
        Serial.print((timeToSU == 0) ?"Not Scheduled":outString);
        // measure interval
        Serial.print(F("\nMonitoring Interval: "));
        Serial.print(measureTimer);
        Serial.print(F("milliseconds\nPower Limiting: "));
        // Power limiting
        Serial.print((maxPower)?("Enabled: ")+String(maxPower)+(" mW"):"Disabled");
    }

    // shutdown
    else if((inputString.substring(0,7)).equalsIgnoringCase("shutdown"))
    {
        uint8_t length = inputString.length();
        int secs = abs((inputString.substring(9,length-1)).toInt());
        shutdown.setTimer(1000*secs);
    }

    // startup
    else if((inputString.substring(0,6)).equalsIgnoringCase("startup"))
    {
        sdval = shutdown.getTimer();
        // check whether shutdown has been scheduled
        if(sdval != 0)
        {
            uint8_t length = inputString.length();
            int secs = sdval + abs(startup((inputString.substring(8,length-1)).toInt()));
            startup.setTimer(1000*secs);
            Serial.println("Startup timer set");
        }
        else
            Serial.println("Set Shutdown timer first");
    }

    // power limiting
    else if((inputString.substring(0,9)).equalsIgnoringCase("limitPower"))
    {
        uint8_t length = inputString.length();
        int _maxPower = abs(startup((inputString.substring(11,length-1)).toInt()));
        if(!_maxPower)
        {
            PowerDrive.setPowerLimit(12000);
            Serial.println("Output Power Limit Disabled")
        }
        else if(_maxPower >= 3000 && _maxPower <= 12000)
        {
            PowerDrive.setPowerLimit(_maxPower);
            Serial.println("Output Power Limit Enabled");
        }
        else
            Serial.println("Output Power Limit could not be enabled\nValid Range: 3000 - 12000 mW");
    }

    // update rate of output power control loop
    else if((inputString.substring(0,14)).equalsIgnoringCase("setResponseTime"))
    {
        uint8_t length = inputString.length();
        float _responseTime = abs(startup((inputString.substring(16,length-1)).toInt()));
        if(_responseTime < 3500 || _responseTime > 12000)
            Serial.println("Valid Range: 3500 - 12000")
        else
        {
            PowerDrive.setResponseTime(_responseTime);
            Serial.println("Response Time Set");
        }
    }

    // measure interval
    else if((inputString.substring(0,9)).equalsIgnoringCase("readInterv"))
    {
        uint8_t length = inputString.length();
        float _measureTimer = abs(startup((inputString.substring(11,length-1)).toInt()));
        if(_measureTimer < 250)
            Serial.println("Valid Range: Above 250 milliseconds")
        else
        {
            measureTimer = _measureTimer;
            Serial.println("Reading Interval Changed");
        }
    }

    // default
    else
        Serial.println("Unrecognised Command!");
}

void setup()
{
    Serial.begin(115200);
    pinMode(DEMANDIND, OUTPUT);
    pinMode(MOSPIN, OUTPUT);
    analogWrite(MOSPIN, 255);
    inputString.reserve(50);
}

void loop()
{
    randomSeed(CURRPIN);
    if(stringComplete)
        manCli();
    if(millis() - lastReadAt > measureTimer)
    {
        measure();
        lastReadAt = millis();
    }
    PowerDrive.update();
    serialEvent();
}