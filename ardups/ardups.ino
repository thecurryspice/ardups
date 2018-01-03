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

// Timer class to keep track of different timeouts
class Timer
{
public:
    Timer(int _timer)   // value in ms
    {
        timer = _timer;
        setPoint = millis();
        state = false;
    }

    ~Timer() {}

    void update()
    {
        if(!timer)
            return;
        if(millis() - ticks > timer && !state)  // no toggling
            state = true;
    }
    bool timeout()
    {
        if(!timer)
            return false;
        if(state)
        {
            state = false;  // state is reset only if checked.
            return true;
        }
    }

    void setTimer(int _timer)
        timer = _timer;

    uint64_t getTimer() const
        return timer;

    uint64_t ticksLeft() const  // ms left until next trigger
    {
        if(!timer)
            return 0;
        return (millis() - setPoint);
    }

private:
    uint64_t setPoint, timer;
    bool state;
};

// default Timer params
Timer shutdown(0);
Timer startup(0);

// Class to calculate Rolling Average
class RollAvg
{
public:
    // total
    RollAvg(int _totalReadings)
        {int totalReadings = _totalReadings;}

    void push(float value) const
    {
        for (int i = totalReadings-1; i > 0; --i)
            readings[i] = readings[i-1];
        readings[0] = value;
    }
    float getAvg()
    {
        for (int i = 0; i < totalReadings; --i)
            sum += readings[i];
        return float(sum/totalReadings);
    }
    void reset()
    {
        for (int i = 0; i < totalReadings; ++i)
            readings[i] = 0;
    }

    void setTotalReadings(int _totalReadings)
        {totalReadings = _totalReadings;}

    int getTotalReadings()
        return totalReadings;

private:
    int totalReadings;
    int readings[totalReadings];
};

RollAvg avgPowerOut(255);
RollAvg avgCurrent(255);

// to manage output power
class PowerDriver
{
public:
    PowerDriver(uint8_t _pin)
    {
        pin = _pin;
        pinMode(pin, OUTPUT);
        drive = 255;
    }
    void update()
    {
        if(!maxPower)
            return
        if(millis() - setPoint > relax)
        {
            current =  CURRSENS;
            supplyVolts = float(1609.7*SUPPLYVOLT*3.3)/1023;
            outPower = (supplyVolts*current)/1000;
            if(outPower > maxPower)
            {
                digitalWrite(DEMANDIND, HIGH);
                // we don't want to lose CV condition, do we?
                if(supplyVolts > 4800)
                    drive--;
            }
            else
            {
                // this is needed to ensure that sharp spikes can be provided before toning down the output
                // the drive values will keep oscillating between two points when limiting output power
                // as soon as the output power drops, drive gradually turns fully on
                digitalWrite(DEMANDIND, LOW);
                drive = max(drive++,255);
            }
            MOSDRIVE(drive);
        }
    }
    void limit(int _maxPower)
        maxPower = _maxPower;
    // higher slope means faster decrease
    void setSlope(float _slope)
    {
        slope = _slope;
        relax = constrain(100-(100/_slope), 5, 100); //set min and max correction time here
    }
private:
    uint8_t pin, relax, drive;
    uint64_t setPoint;
    int maxPower;
    float slope;
};

PowerDriver PowerDrive(MOSPIN);

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
}

// will run after every loop
void serialEvent()
{
    while (Serial.available())
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
        Serial.println(F("limitPowerSlope   :   determines slope of decrease in output power"));
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
            startup.setTimer(1000*secs)
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
            Serial.println("Output Power Limit Disabled")
        else if(_maxPower <= 12000)
        {
            maxPower = _maxPower;
            Serial.println("Output Power Limit Enabled");
        }
        else
            Serial.println("Valid Range: 0 - 12000 mW");
    }

    // power decrease slope
    else if((inputString.substring(0,14)).equalsIgnoringCase("limitPowerSlope"))
    {
        uint8_t length = inputString.length();
        float _slope = abs(startup((inputString.substring(16,length-1)).toFloat()));
        if(_slope <= 0 || _slope >= 1)
            Serial.println("Valid Range: Float between 0 and 1")
        else
        {
            PowerDrive.setSlope(_slope);
            Serial.println("Power Decrease Slope Changed");
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