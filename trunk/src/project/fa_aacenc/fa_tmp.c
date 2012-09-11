#include <stdio.h>
#include <stdlib.h>

#define min(a,b)  ( (a) < (b) ? (a) : (b) )
#define max(a,b)  ( (a) > (b) ? (a) : (b) )



/* Returns the maximum bitrate per channel for that sampling frequency */
unsigned int MaxBitrate(unsigned long sampleRate)
{
    /*
     *  Maximum of 6144 bit for a channel
     */
    return (unsigned int)(6144.0 * (double)sampleRate/(double)1024+ .5);
}

/* Returns the minimum bitrate per channel for that sampling frequency */
unsigned int MinBitrate()
{
    return 8000;
}


/* Max prediction band for backward predictionas function of fs index */
const int MaxPredSfb[] = { 33, 33, 38, 40, 40, 40, 41, 41, 37, 37, 37, 34, 0 };

int GetMaxPredSfb(int samplingRateIdx)
{
    return MaxPredSfb[samplingRateIdx];
}



/* Calculate bit_allocation based on PE */
unsigned int BitAllocation(double pe, int short_block)
{
    double pew1;
    double pew2;
    double bit_allocation;

    if (short_block) {
        pew1 = 0.6;
        pew2 = 24.0;
    } else {
        pew1 = 0.3;
        pew2 = 6.0;
    }
    bit_allocation = pew1 * pe + pew2 * sqrt(pe);
    bit_allocation = min(max(0.0, bit_allocation), 6144.0);

    return (unsigned int)(bit_allocation+0.5);
}

/* Returns the maximum bit reservoir size */
unsigned int MaxBitresSize(unsigned long bitRate, unsigned long sampleRate)
{
    return 6144 - (unsigned int)((double)bitRate/(double)sampleRate*(double)1024);
}
