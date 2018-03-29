// Class to calculate Rolling Average
class RollAvg
{
private:
    uint8_t totalReadings = 16, currReading = 0;
    uint32_t readings[16];
    uint32_t sumReadings = 0;
public:
    // total
/*
    RollAvg(int _totalReadings)
        {int totalReadings = _totalReadings;}
*/
    RollAvg(){}

    //void setValue(uint8_t _index, int value)
    void push(float value)
    {
        readings[currReading] = value;
        currReading++;
        // loop over if sample space is exceeded
        if(currReading > totalReadings - 1)
            currReading = 0;            
    }
    float getAvg()
    {
        sumReadings = 0.0;
        for (int i = 0; i < totalReadings; i++)
            sumReadings += readings[i];
        return (float(sumReadings)/totalReadings);
    }
    void reset()
    {
        for (int i = 0; i < totalReadings; i++)
            readings[i] = 0;
    }
    uint32_t getSum()
    {
        sumReadings = 0;
        for (int i = 0; i < totalReadings; i++)
            sumReadings += readings[i];
        return sumReadings;
    }
/*
    void setTotalReadings(int _totalReadings)
        {totalReadings = _totalReadings;}
*/
    int getTotalReadings() const
    {
        return totalReadings;
    }
};