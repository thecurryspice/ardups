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
    {
        timer = _timer;
        setPoint = millis();
    }

    uint64_t getTimer() const
        return timer;

    uint64_t ticksLeft() const  // ms left until next trigger
    {
        if(!timer)
            return 0;
        return (setPoint + timer - millis());
    }

private:
    uint64_t setPoint, timer;
    bool state;
};