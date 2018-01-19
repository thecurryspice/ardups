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
