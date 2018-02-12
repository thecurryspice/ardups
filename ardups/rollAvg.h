// Class to calculate Rolling Average
class RollAvg
{
private:
    uint8_t totalReadings = 64;
    uint32_t readings[64];
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
        for (int i = totalReadings-1; i > 0; i--)
            readings[i] = readings[i-1];
        readings[0] = value;
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