#include <Arduino.h>
#include <FastLED.h>

#define GRASS 0
#define ROUGH 1
#define SAND 2
#define WATER 3
#define HOLE 5

#define TYPE 1
#define CLUSTER 2
#define HOLE 3



class mapGenerator
{
public:
    byte holeID;
    byte courseID;
    byte loopCount;
    byte clusterSizeID;
    byte clusterTypeID;

    mapGenerator(byte holeIdentifier, byte courseIdentifier)
        : holeID(holeIdentifier), courseID(courseIdentifier) {}

    void setup()
    {
        
    }

    int mapInfoFeeder(byte count, byte infoChoice) const
    {
        byte selectedCluster;
        byte selectedType;
        int holePos;
        // String statsCombine = "Fish ID: " + String(fishID) + "Fish Location: " + String(location) + "Fish Weight: " + String(weight);
        switch(holeID)
        {
            case 1:
            
            break;
        }
        if (holeID == 1)
        {
            byte levelType[] = {GRASS, SAND, WATER};
            byte levelCluster[] = {190, 20, 40};
            selectedCluster = levelCluster[count];
            selectedType = levelType[count];
            holePos = 140;
        }
        else if (holeID == 2)
        {
            byte levelType[] = {GRASS, ROUGH, GRASS, ROUGH, GRASS, ROUGH, SAND, GRASS};
            byte levelCluster[] = {50, 30, 80, 40, 45, 5, 10, 5, 100};
            selectedCluster = levelCluster[count];
            selectedType = levelType[count];
            holePos = 230;
        }
        else if (holeID == 3)
        {
            byte levelType[] = {GRASS, WATER, GRASS, SAND, GRASS, SAND, ROUGH, WATER, SAND, WATER};
            byte levelCluster[] = {20, 10, 100, 90, 30, 10, 5, 40, 100};
            selectedCluster = levelCluster[count];
            selectedType = levelType[count];
            holePos = 200;
        }
        else if (holeID == 4)
        {
            byte levelType[] = {GRASS, ROUGH, GRASS, WATER, SAND, GRASS, ROUGH, WATER};
            byte levelCluster[] = {50, 20, 30, 40, 10, 90, 15, 100};
            selectedCluster = levelCluster[count];
            selectedType = levelType[count];
            holePos = 190;
        }
        else if (holeID == 6)
        {
            byte levelType[] = {GRASS, ROUGH, GRASS, ROUGH, GRASS, SAND, GRASS, ROUGH, SAND, GRASS, SAND};
            byte levelCluster[] = {40, 10, 35, 15, 30, 10, 30, 15, 10, 35, 100};
            selectedCluster = levelCluster[count];
            selectedType = levelType[count];
            holePos = 240;
        }
        else if (holeID == 5)
        {
            byte levelType[] = {WATER, GRASS, ROUGH, SAND, GRASS, ROUGH, GRASS, ROUGH, GRASS, SAND, WATER};
            byte levelCluster[] = {5, 40, 15, 20, 50, 5, 5, 10, 45, 20, 100};
            selectedCluster = levelCluster[count];
            selectedType = levelType[count];
            holePos = 200;
        }
        else if (holeID == 7)
        {
            byte levelType[] = {GRASS, WATER, GRASS, SAND, WATER, GRASS, ROUGH, SAND, ROUGH,};
            byte levelCluster[] = {30, 30, 20, 20, 10, 100, 20, 30, 60};
            selectedCluster = levelCluster[count];
            selectedType = levelType[count];
            holePos = 220;
        }
        else if (holeID == 8)
        {
            byte levelType[] = {GRASS, ROUGH, WATER, ROUGH, GRASS, ROUGH, WATER, ROUGH, WATER, GRASS, ROUGH, WATER, ROUGH, WATER, ROUGH, WATER, ROUGH};
            byte levelCluster[] = {35, 5, 20, 5, 25, 10, 5, 30, 10, 50, 30, 20, 30, 5, 100, 100, 10};
            selectedCluster = levelCluster[count];
            selectedType = levelType[count];
            holePos = 240;
        }
        else if (holeID == 9)
        {
            byte levelType[] = {GRASS, ROUGH, GRASS, WATER, ROUGH, WATER, SAND, GRASS, SAND, ROUGH, WATER, WATER, SAND, GRASS, WATER};
            byte levelCluster[] = {35, 5, 20, 5, 15, 15, 20, 15, 5, 10, 30, 5, 5, 30, 100};
            selectedCluster = levelCluster[count];
            selectedType = levelType[count];
            holePos = 230;
        }
        if (infoChoice == TYPE)
        {
            return selectedType;
        }
        else if (infoChoice == CLUSTER)
        {
            return selectedCluster;
        }
        else if (infoChoice == HOLE)
        {
            return holePos;
        }
        else
        {
            return 0;
        }
    }
};



// if (holeID == 1)
//         {
//             byte levelType[] = {GRASS, GRASS};
//             byte levelCluster[] = {222, 222};
//             selectedCluster = levelCluster[count];
//             selectedType = levelType[count];
//             holePos = 100;
//         }
//         else if (holeID == 2)
//         {
//             byte levelType[] = {GRASS, ROUGH, GRASS, ROUGH};
//             byte levelCluster[] = {200, 30, 160, 40};
//             selectedCluster = levelCluster[count];
//             selectedType = levelType[count];
//             holePos = 400;
//         }
//         else if (holeID == 3)
//         {
//             byte levelType[] = {GRASS, SAND, GRASS, SAND, GRASS};
//             byte levelCluster[] = {100, 90, 120, 30, 100};
//             selectedCluster = levelCluster[count];
//             selectedType = levelType[count];
//             holePos = 400;
//         }
//         else if (holeID == 4)
//         {
//             byte levelType[] = {GRASS, WATER, GRASS, WATER, GRASS, WATER};
//             byte levelCluster[] = {110, 40, 110, 20, 80, 100};
//             selectedCluster = levelCluster[count];
//             selectedType = levelType[count];
//             holePos = 300;
//         }
//         else if (holeID == 5)
//         {
//             byte levelType[] = {GRASS, ROUGH, GRASS, ROUGH, GRASS, SAND, GRASS, ROUGH, SAND, GRASS, SAND};
//             byte levelCluster[] = {80, 20, 70, 30, 60, 20, 60, 30, 20, 70, 100};
//             selectedCluster = levelCluster[count];
//             selectedType = levelType[count];
//             holePos = 400;
//         }
//         else if (holeID == 6)
//         {
//             byte levelType[] = {GRASS, ROUGH, SAND, GRASS, ROUGH, GRASS, ROUGH, GRASS, ROUGH, WATER};
//             byte levelCluster[] = {80, 30, 40, 100, 10, 10, 20, 90, 20, 100};
//             selectedCluster = levelCluster[count];
//             selectedType = levelType[count];
//             holePos = 400;
//         }
//         else if (holeID == 7)
//         {
//             byte levelType[] = {GRASS, WATER, GRASS, SAND, ROUGH, GRASS, ROUGH, SAND, ROUGH, GRASS};
//             byte levelCluster[] = {30, 30, 20, 20, 10, 100, 20, 30, 30, 150};
//             selectedCluster = levelCluster[count];
//             selectedType = levelType[count];
//             holePos = 400;
//         }
//         else if (holeID == 8)
//         {
//             byte levelType[] = {GRASS, WATER, GRASS, ROUGH, GRASS, ROUGH, WATER, GRASS};
//             byte levelCluster[] = {80, 20, 80, 30, 100, 30, 20, 100};
//             selectedCluster = levelCluster[count];
//             selectedType = levelType[count];
//             holePos = 400;
//         }
//         else if (holeID == 9)
//         {
//             byte levelType[] = {GRASS, ROUGH, GRASS, WATER, ROUGH, WATER, SAND, GRASS, SAND, ROUGH, GRASS, ROUGH, SAND, GRASS, WATER};
//             byte levelCluster[] = {70, 10, 40, 10, 30, 20, 40, 30, 10, 20, 60, 10, 10, 60, 60};
//             selectedCluster = levelCluster[count];
//             selectedType = levelType[count];
//             holePos = 380;
//         }