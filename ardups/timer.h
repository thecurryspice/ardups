// Timer class to keep track of different timeouts
class Timer
{
private:
    uint64_t setPoint, timer;
    bool state;
public:
    Timer(unsigned long _timer)   // value in ms
    {
        timer = _timer;
        setPoint = millis();
        state = false;
    }

    ~Timer() {}

    // check whether the 
    void update()
    {
        if(!timer)
            return;
        if(millis() - setPoint > timer && !state)  // no toggling
        {
            return true;
            state = true;
        }
        else
        {
            state = false;
            return false;
        }
    }
    
    bool timeout()
    {
        if(!timer)
            return false;
        update();
        return state;
    }

    void setTimer(unsigned long _timer)
    {
        timer = _timer;
        setPoint = millis();
    }

    unsigned long getTimer() const
    {
        return timer;
    }

    unsigned long ticksLeft() const  // ms left until next trigger
    {
        if(!timer)
            return 0;
        return (setPoint + timer - millis());
    }
};