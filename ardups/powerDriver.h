# include "definitions.h"
// Class to manage power drives
class PowerDriver
{
private:
    uint8_t pin, responseTime, duty;
    uint64_t setPoint;
    int maxPower;
    bool showIndicator = true;
public:
    PowerDriver(uint8_t _pin, uint8_t responseTime, int _maxPower)
    {
        pin = _pin;
        pinMode(pin, OUTPUT);
        duty = 255;
    }

    uint8_t getDuty()
    {
        return duty;
    }

    // pwm duty
    void setDuty(uint8_t _duty)
    {
        duty = _duty;
        analogWrite(pin, duty);
    }

    // update the control loop
    void update()
    {
        if(!maxPower)
            return;
        if(millis() - setPoint > responseTime)
        {
            int current =  CURRSENS;
            int supplyVolts = float(1609.7*SUPPLYVOLT*3.3)/1023;
            int outPower = (supplyVolts*current)/1000;
            if(outPower > maxPower)
            {
                if(showIndicator)
                    digitalWrite(CONSTVOLT, HIGH);
                // we don't want to lose CV condition, do we?
                if(supplyVolts > 4900)
                    duty--;
            }
            else
            {
                // this is needed to ensure that sharp spikes can be provided after toning down the output.
                // the (mos)drive values will keep oscillating between two points when limiting output power.
                // as soon as the output power drops, drive gradually turns fully on.
                digitalWrite(CONSTVOLT, LOW);
                duty = max(duty++,255);
            }
            setDuty(duty);
            setPoint = millis();
        }
    }
    
    // max power in mW
    void setPowerLimit(int _maxPower)
    {
        maxPower = _maxPower;
    }

    // determines update rate of control loop
    void setResponseTime(float _responseTime)
    {
        responseTime = constrain(_responseTime,3,100);
    }

    // whether to use indicator or not
    void indicator(bool _showIndicator)
    {
        showIndicator = _showIndicator;
    }
};
