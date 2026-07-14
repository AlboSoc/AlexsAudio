// GLOW GOLF Script by Alex Johansson, Sheffield, 2023
// AUDIO INTEGRATION — ESP-NOW trigger packets for AlexsAudio sound server

#include <Arduino.h>
#include "MultiTapButton.h"
#include "SimpleTimer.h"
#include "audio_trigger_sender.h"
#include <FastLED.h>
#include <MapHandler.cpp>

#define DEBUG Serial.printf("line %d\n", __LINE__);

#define BUTTON_1_PIN 14
#define PIN_1 33
#define LDR_PIN 34
#define NUMPIXELS_1 250

#define TYPE 1
#define CLUSTER 2
#define HOLEPOS 3
#define MAGNIFIER 2.5

#define GRASS 0
#define ROUGH 1
#define SAND 2
#define WATER 3
#define HOLE 5

#define STRENGTH_TIMESCALE 40.0f
#define STRENGTH_MAX 100
#define INPUT_THRESHOLD 5

// Sound event identifiers
#define SOUND_PUTT 0
#define SOUND_WIN 1
#define SOUND_HOLE 2
#define SOUND_WATER 3
#define SOUND_SWING 4
#define SOUND_SAND 5
#define SOUND_GRASS 6
#define SOUND_LOSE 7

bool soundArmed[8] = {false, true, true, true, true, true, true, true};
AudioTriggerSender audioTriggerSender;

// MultiTapButton button(BUTTON_1_PIN, LOW);
CRGB leds[NUMPIXELS_1];
byte levelClusterSize[20];
byte levelTerrainType[20];

CHSV roughGreen(88, 200, 100);
CHSV waterBlue(160, 255, 255);
CHSV grassGreen(96, 255, 255);
CHSV sandYellow(64, 255, 255);
CHSV ballWhite(255, 255, 255);

byte shotCount = 0;
const int LDRPIN = LDR_PIN;
int potValue;
float scale = 0;
float currentValue = 0;
float changeRate = 0;

bool gameEnd;
int32_t golfStrength;
uint32_t startHold;
int idleTicker;
int velocity;
byte mapName[NUMPIXELS_1];
int ballPosition = 0;
int holePosition;
int direction = 1;
int strength;
bool isPressingButton = false;
byte holeNumber = 1;
bool randomMode = true;
bool randomMaps = true;
bool demo = false;
bool firstInput = false;
byte demoLevelCounter = 0;
int demoTimer = 0;
float handlePower = 0;
int lastBallPosition = 0;

// Forward declarations
void launchBall(int shotStrength, int ballLocation);
void refreshPixels();
void ballOutOfBounds();
void mapMaker();
void landCheck();
void rollBall(int shotStrength, int ballLocation, int delayCheck);
void setHolePosition(int holePosition);
void spawnBall();
void resetGame();
void flashHole(byte flashCounter);
void demoMode();
void restoreMapSection(int startX, int count);
void ballInputLogic();
void waterDeath(int ballDeathPos);
void winState(int ballWinPos);
void checkBallDirection(int holePosition);
void generateSetMaps();
void pullMapData(byte holeID, byte courseID);
CRGB getColour(int mapName);
void levelReset();
void triggerSound(byte soundID);
void armSound(byte soundID);
void armAllSounds();
bool audioConnected();

// =============================================================================
// AUDIO FUNCTIONS
// =============================================================================

void setupAudio()
{
	if (audioTriggerSender.begin())
	{
		audioTriggerSender.printBanner();
	}
	else
	{
		Serial.println("Audio trigger: ESP-NOW unavailable.");
	}
}

void triggerSound(byte soundID)
{
	if (soundID < 1 || soundID > 7)
	{
		return;
	} // Bounds check
	if (!audioConnected())
	{
		return;
	} // Silent fail if sender is not ready
	if (!soundArmed[soundID])
	{
		return;
	} // Already fired, not re-armed yet

	if (audioTriggerSender.sendPlaySound(soundID))
	{
		soundArmed[soundID] = false;
		Serial.print("Audio trigger: sound ");
		Serial.println(soundID);
	}
}

void armSound(byte soundID)
{
	if (soundID < 1 || soundID > 7)
	{
		return;
	}
	soundArmed[soundID] = true;
}

void armAllSounds()
{
	for (byte i = 1; i <= 7; i++)
	{
		soundArmed[i] = true;
	}
}

bool audioConnected()
{
	return audioTriggerSender.isReady();
}

// =============================================================================
// SETUP — audio trigger sender initialises here
// =============================================================================

void setup()
{
	FastLED.addLeds<NEOPIXEL, PIN_1>(leds, NUMPIXELS_1);
	FastLED.setBrightness(255);
	Serial.begin(9600);

	// pinMode(BUTTON_1_PIN, INPUT);
	pinMode(LDRPIN, INPUT);
	pinMode(BUILTIN_LED, OUTPUT);
	digitalWrite(BUILTIN_LED, HIGH);
	FastLED.clear();
	FastLED.show();

	setupAudio();

	resetGame();
}

// =============================================================================
// MAIN LOOP
// =============================================================================

void loop()
{
	if (gameEnd)
	{
		if (golfStrength > 0)
		{
			golfStrength = 0;
		}
		else
		{
			resetGame();
		}
	}

	potValue = analogRead(LDRPIN);
	scale = potValue / 25;
	// Serial.println(scale);
	changeRate = scale - currentValue;
	currentValue = scale;

	if (scale > INPUT_THRESHOLD)
	{
		isPressingButton = true;
		if (!firstInput)
		{
			firstInput = true;
		}
		demo = false;

		if (handlePower < scale)
		{
			handlePower = scale;
			int powerScale = round(handlePower / (10 / MAGNIFIER));
			CRGB scaleColour;
			if (handlePower > 60 / MAGNIFIER)
			{
				scaleColour = CRGB::Red;
			}
			else
			{
				scaleColour = CRGB::Purple;
			}

			if (direction == 1)
			{
				for (int j = 0; j < powerScale; j++)
				{
					leds[(ballPosition + 2) + j] = scaleColour;
				}
			}
			else
			{
				for (int j = 0; j < powerScale; j++)
				{
					leds[(ballPosition - 3) - j] = scaleColour;
				}
			}
			FastLED.show();
		}
	}
	else
	{
		isPressingButton = false;
		golfStrength = handlePower;
		handlePower = 0;
		// Serial.print(golfStrength);
		ballInputLogic();
	}

	if (!isPressingButton && firstInput)
	{
		if (idleTicker > 5000)
		{
			idleTicker = 0;
			levelReset();
			gameEnd = true;
		}
		else
		{
			idleTicker++;
		}
	}
	else
	{
		if (!firstInput && !demo && holeNumber == 1)
		{
			if (demoTimer > 3000)
			{
				demo = true;
				demoTimer = 0;
				resetGame();
			}
			else
			{
				demoTimer++;
			}
		}
		idleTicker = 0;
	}
}

// =============================================================================
// GAME LOGIC
// =============================================================================

void levelReset()
{
	if (randomMode)
	{
		randomMaps = true;
	}
	else
	{
		randomMaps = false;
	}
	holeNumber = 1;
}

void refreshPixels()
{
	for (int i = 0; i < NUMPIXELS_1; i++)
	{
		leds[i] = getColour(mapName[i]);
		if (mapName[i] == 0)
		{
			leds[i] = grassGreen;
		}
		else if (mapName[i] == 1)
		{
			leds[i] = roughGreen;
		}
		else if (mapName[i] == 2)
		{
			leds[i] = CRGB::Yellow;
		}
		else if (mapName[i] == 3)
		{
			leds[i] = CRGB::Blue;
		}
		else if (mapName[i] == 4)
		{
			leds[i] = CRGB::White;
		}
		else if (mapName[i] == 5)
		{
			leds[i] = CRGB::Black;
		}
		FastLED.show(); // Intentional per-pixel show — creates course fill-in transition
	}
}

void demoMode()
{
	randomMaps = true;
	golfStrength = 0;
	for (int i = 0; i < 50; i++)
	{
		golfStrength = random(1, 75);
		triggerSound(SOUND_SWING);
		launchBall(golfStrength, ballPosition);

		delay(random(750, 2000));
		armAllSounds();
		if (gameEnd)
		{
			break;
		}
		if (analogRead(LDRPIN) / 25 > INPUT_THRESHOLD)
		{
			break;
		}
	}
}

void ballOutOfBounds()
{
	// [AUDIO] Ball left the course
	triggerSound(SOUND_LOSE);

	for (int i = 0; i < NUMPIXELS_1; i++)
	{
		leds[NUMPIXELS_1 - i] = roughGreen;
		FastLED.show();
	}
	delay(100);
	gameEnd = true;
}

void winState(int ballWinPos)
{
	gameEnd = true;

	// [AUDIO] Ball in hole — fires before animation so sound lands with the visual
	triggerSound(SOUND_HOLE);
	delay(1800);
	triggerSound(SOUND_WIN);

	if (holeNumber >= 9)
	{
		randomMaps = true;
	}
	else
	{
		if (!demo && !randomMode)
		{
			holeNumber++;
		}
		else
		{
			randomMaps = true;
		}
	}

	// Ball rolls into hole — direction-dependent
	if (direction == -1)
	{
		for (int i = 0; i < 4; i++)
		{
			leds[holePosition - i] = CRGB::White;
			leds[holePosition - i + 1] = CRGB::Black;
			FastLED.show();
			delay(150);
		}
		// leds[(holePosition - 1) - 3] = CRGB::White;
		// leds[(holePosition - 1) - 3 - 1] = CRGB::Black;
		// FastLED.show();
		// delay(150);
	}
	else
	{
		for (int i = 4; i > 0; i--)
		{
			leds[(holePosition - 1) - i] = CRGB::White;
			leds[(holePosition - 1) - i - 1] = CRGB::Black;
			FastLED.show();
			delay(150);
		}
		// delay(150);
		// leds[(holePosition - 1) - 3] = CRGB::White;
		// leds[(holePosition - 1) - 3 - 1] = CRGB::Black;
		// FastLED.show();
		
	}

	delay(350);

	// Flash hole once per shot taken — par indicator
	byte flashCount = min(shotCount, (byte)9);
	for (byte s = 0; s < flashCount; s++)
	{
		for (int i = 0; i < 5; i++)
		{
			leds[holePosition - i] = CRGB::White;
		}
		FastLED.show();
		delay(300);
		for (int i = 0; i < 5; i++)
		{
			leds[holePosition - i] = CRGB::Black;
		}
		FastLED.show();
		delay(200);
	}

	delay(300);

	// Black expands outward from hole eating the course
	int expandUp = holePosition + 1;
	int expandDown = holePosition - 5;
	bool upDone = false;
	bool downDone = false;

	while (!upDone || !downDone)
	{
		leds[holePosition - 2] = CRGB::White;
		if (expandUp < NUMPIXELS_1)
		{
			leds[expandUp] = CRGB::Black;
			expandUp++;
		}
		else
		{
			upDone = true;
		}
		if (expandDown >= 0)
		{
			leds[expandDown] = CRGB::Black;
			expandDown--;
		}
		else
		{
			downDone = true;
		}
		FastLED.show();
		delay(4);
	}

	delay(400);

	// White pixel travels from hole back to start
	for (int i = holePosition; i >= 0; i--)
	{
		leds[i] = CRGB::White;
		if (i < NUMPIXELS_1 - 1)
		{
			leds[i + 1] = CRGB::Black;
		}
		FastLED.show();
		delay(6);
	}
}

void waterDeath(int ballDeathPos)
{
	gameEnd = true;

	// [AUDIO] Ball hits water — fires at moment of impact before flood animation
	triggerSound(SOUND_WATER);
	delay(850);
	triggerSound(SOUND_LOSE);

	for (int i = 0; i < NUMPIXELS_1 - ballDeathPos; i++)
	{
		leds[ballDeathPos + i] = CRGB::Blue;
		FastLED.show();
	}
	for (int i = 0; i < ballDeathPos; i++)
	{
		leds[ballDeathPos - i] = CRGB::Blue;
		FastLED.show();
	}
	delay(500);
}

void landCheck()
{
	// Impact flicker
	for (int i = 0; i < 3; i++)
	{
		leds[(ballPosition - 1) + i] = CRGB::Black;
	}
	FastLED.show();
	delay(30);
	restoreMapSection(ballPosition - 1, 3);

	if (ballPosition > NUMPIXELS_1)
	{
		ballOutOfBounds();
	}
	else
	{
		if (mapName[ballPosition] == 0)
		{
			// [AUDIO] Ball lands on grass
			triggerSound(SOUND_GRASS);
			rollBall(golfStrength / 4, ballPosition, 2 + golfStrength / 5);
		}
		else if (mapName[ballPosition] == 1)
		{
			// [AUDIO] Rough uses grass landing sound — close enough
			triggerSound(SOUND_GRASS);
			rollBall(golfStrength / 4, ballPosition, 2 + golfStrength / 5);
		}
		else if (mapName[ballPosition] == 2)
		{
			// [AUDIO] Ball lands in sand
			triggerSound(SOUND_SAND);
			rollBall(golfStrength / 15, ballPosition, 2 + golfStrength / 5);
		}
		else if (mapName[ballPosition] == 3)
		{
			waterDeath(ballPosition); // Audio fired inside waterDeath()
			gameEnd = true;
		}
		else if (mapName[ballPosition] == 5)
		{
			winState(ballPosition); // Audio fired inside winState()
			gameEnd = true;
		}
	}
	// armAllSounds();
	return;
}

// PLAYER INPUT
void ballInputLogic()
{
	handlePower = 0;
	if (golfStrength > 0)
	{
		lastBallPosition = ballPosition;
		if (!demo)
		{
			shotCount++;
		}

		// [AUDIO] Swing — fires at moment of release before ball moves

		int ballOffset;
		if (direction == 1)
		{
			ballOffset = -4;
		}
		else
		{
			ballOffset = 1;
		}

		for (int i = 0; i < 4; i++)
		{
			leds[ballPosition + ballOffset + i] = CRGB::White;
		}
		FastLED.show();
		delay(5);
		FastLED.show();
		restoreMapSection(ballPosition + ballOffset, 5);

		if (golfStrength > 60 / MAGNIFIER)
		{
			triggerSound(SOUND_SWING);
			launchBall(golfStrength / (3 - MAGNIFIER), ballPosition);
		}
		else
		{
			triggerSound(SOUND_PUTT);
			rollBall(golfStrength / 1.25, ballPosition, 2 + golfStrength / 2.5);
		}
		armAllSounds();
	}
}

void restoreMapSection(int startX, int count)
{
	for (int i = startX; i < startX + count; i++)
	{
		if (i < 0 || i > NUMPIXELS_1)
		{
			continue;
		}
		leds[i] = getColour(mapName[i]);
		FastLED.show();
	}
}

void checkBallDirection(int holePosition)
{
	if (ballPosition > holePosition + 1)
	{
		direction = -1;
	}
	else if (ballPosition < holePosition)
	{
		direction = 1;
	}
}

void launchBall(int shotStrength, int ballLocation)
{
	velocity = shotStrength;
	for (int i = 0; i < shotStrength; i++)
	{
		if (gameEnd)
		{
			break;
		}
		else
		{
			velocity--;
			leds[ballLocation] = CRGB::White;

			// Dim ghost trail
			int trailPos = ballLocation - direction;
			if (trailPos >= 0 && trailPos < NUMPIXELS_1)
			{
				leds[trailPos] = CRGB(15, 50, 15);
			}

			leds[ballLocation - direction] = getColour(mapName[ballLocation - direction]);

			if (direction == 1)
			{
				ballLocation++;
			}
			else
			{
				ballLocation--;
			}

			if (!gameEnd)
			{
				ballPosition = ballLocation;
			}
		}
		delay(10 / (1 + velocity / 10));
		leds[ballLocation + 1] = CRGB::White;
		leds[ballLocation - 1] = CRGB::White;
		FastLED.show();
	}
	if (!gameEnd)
	{
		landCheck();
	}
}

CRGB getColour(int mapName)
{
	if (mapName == GRASS)
		return grassGreen;
	if (mapName == ROUGH)
		return roughGreen;
	if (mapName == SAND)
		return CRGB::Yellow;
	if (mapName == WATER)
		return CRGB::Blue;
	if (mapName == HOLE)
		return CRGB::Black;
	if (mapName == 4)
		return grassGreen;
	return CRGB::Pink;
}

void rollBall(int shotStrength, int ballLocation, int delayCheck)
{
	if (direction == 1)
	{
		restoreMapSection(ballPosition + 1, 11);
	}
	else if (direction == -1)
	{
		restoreMapSection(ballPosition - 11, 11);
	}

	leds[ballLocation] = CRGB::White;
	FastLED.show();
	velocity = 0;

	for (int i = 0; i < shotStrength; i++)
	{
		velocity++;
		if (gameEnd)
		{
			break;
		}
		else
		{
			leds[ballLocation] = CRGB::White;
			leds[ballLocation - direction] = getColour(mapName[ballLocation - direction]);
		}

		if (ballPosition > NUMPIXELS_1)
		{
			ballOutOfBounds();
		}

		if (mapName[ballLocation] == 3)
		{
			waterDeath(ballPosition); // Audio fired inside waterDeath()
			ballPosition = 10 + random(-4, 4);
			ballLocation = ballPosition;
			gameEnd = true;
		}
		if (mapName[ballLocation] == 2)
		{
			// [AUDIO] Rolling into sand — only fires if not already disarmed from aerial landing
			triggerSound(SOUND_SAND);
			ballLocation = ballPosition;
			delayCheck = 1;
			shotStrength = 1;
		}
		if (mapName[ballLocation] == 1)
		{
			ballLocation = ballPosition;
			delayCheck = 1;
			shotStrength = floor(shotStrength / 2);
		}
		if (mapName[ballLocation] == 5)
		{
			winState(ballPosition); // Audio fired inside winState()
			ballPosition = 10 + random(-4, 4);
			ballLocation = ballPosition;
			gameEnd = true;
		}

		if (direction == 1)
		{
			ballLocation++;
			ballPosition = ballLocation;
		}
		else if (direction == -1)
		{
			ballLocation--;
			ballPosition = ballLocation;
		}

		delay((100 * velocity) / delayCheck);
		FastLED.show();
	}
	checkBallDirection(holePosition);
	if (!gameEnd)
	{
		flashHole(1);
	}
}

// =============================================================================
// RESET
// =============================================================================

void resetGame()
{
	if (demo)
	{
		if (demoLevelCounter < 1)
		{
			randomMaps = true;
			demoLevelCounter++;
		}
		else
		{
			demo = false;
			if (randomMode)
			{
				randomMaps = true;
			}
			else
			{
				randomMaps = false;
			}
			demoLevelCounter = 0;
		}
	}

	demoTimer = 0;
	handlePower = 0;
	golfStrength = 0;
	direction = 1;
	idleTicker = 0;
	firstInput = false;
	shotCount = 0;

	// Re-arm all sounds for the new hole
	armAllSounds();

	FastLED.clear();
	launchBall(0, ballPosition);

	if (!randomMaps && !randomMode)
	{
		pullMapData(holeNumber, 1);
		generateSetMaps();
		setHolePosition(holePosition);
	}
	else
	{
		holePosition = NUMPIXELS_1 - random(round(NUMPIXELS_1 / 12.5), round(NUMPIXELS_1 / 3));
		mapMaker();
		setHolePosition(holePosition);
	}

	refreshPixels();
	ballPosition = 10 + random(0, 4);
	spawnBall();
	leds[ballPosition] = CRGB::White;
	delay(10);
	flashHole(3);
	gameEnd = false;

	if (demo)
	{
		demoMode();
	}
}

// =============================================================================
// MAP / HOLE SETUP
// =============================================================================

void setHolePosition(int holePosition)
{
	for (int i = holePosition - 10; i < holePosition + 10; i++)
	{
		mapName[i] = GRASS;
	}
	mapName[holePosition] = HOLE;
	mapName[holePosition - 1] = HOLE;
	mapName[holePosition - 2] = HOLE;
	mapName[holePosition - 3] = HOLE;
	mapName[holePosition - 4] = HOLE;
}

void flashHole(byte flashCounter)
{
	// Black pulse travels from ball toward hole — direction cue
	{
		int pulsePos = ballPosition;
		int pulseDir = (holePosition > ballPosition) ? 1 : -1;
		int pulseSteps = abs(holePosition - ballPosition);
		int pulseTravel = min(pulseSteps, 40);
		delay(300);

		for (int p = 0; p < pulseTravel; p++)
		{
			leds[pulsePos] = CRGB::Black;
			if (p > 0)
			{
				leds[pulsePos - pulseDir] = getColour(mapName[pulsePos - pulseDir]);
			}
			FastLED.show();
			delay(8);
			pulsePos += pulseDir;
		}
		restoreMapSection(ballPosition, min(pulseTravel + 1, NUMPIXELS_1 - ballPosition));
	}

	for (byte i = 0; i < flashCounter; i++)
	{
		leds[holePosition + 1] = CRGB::Red;
		leds[holePosition] = CRGB::Red;
		leds[holePosition - 1] = CRGB::Red;
		leds[holePosition - 2] = CRGB::Red;
		leds[holePosition - 3] = CRGB::Red;
		leds[holePosition - 4] = CRGB::Red;
		leds[holePosition - 5] = CRGB::Red;
		FastLED.show();
		delay(250);
		leds[holePosition + 1] = mapName[holePosition + 1];
		leds[holePosition] = CRGB::Black;
		leds[holePosition - 1] = CRGB::Black;
		leds[holePosition - 2] = CRGB::Black;
		leds[holePosition - 3] = CRGB::Black;
		leds[holePosition - 4] = CRGB::Black;
		leds[holePosition - 5] = mapName[holePosition - 5];
		FastLED.show();
		delay(150);
	}
	if (ballPosition >= 10)
	{
		leds[ballPosition] = CRGB::White;
		FastLED.show();
	}
}

void spawnBall()
{
	// [AUDIO] Removed from here — swing sound belongs to ballInputLogic(), not spawn
	direction = 1;
	mapName[ballPosition] = 4;

	CRGB spawnSequence[] = {CRGB::Yellow, CRGB(255, 100, 0), CRGB::White};
	for (int phase = 0; phase < 3; phase++)
	{
		for (byte i = 0; i < 5; i++)
		{
			leds[(ballPosition - 2) + i] = spawnSequence[phase];
		}
		FastLED.show();
		delay(80);
	}

	for (byte j = 0; j < 3; j++)
	{
		leds[(ballPosition + 3) - j] = grassGreen;
		leds[(ballPosition - 3) + j] = grassGreen;
		delay(100);
		leds[ballPosition] = CRGB::White;
		FastLED.show();
	}
}

void mapMaker()
{
	int mapNameOffset = 0;
	for (int i = 0; i < 20; i++)
	{
		mapName[i] = 0;
		mapNameOffset++;
	}

	for (int i = 20; i < NUMPIXELS_1 - 20; i++)
	{
		int terrainRandom = random(0, 7);
		int terrainCluster;
		if (terrainRandom == 1)
		{
			terrainCluster = random(10, 20);
			for (int p = i; p < terrainCluster + i; p++)
			{
				mapName[mapNameOffset] = 1;
				mapNameOffset++;
			}
		}
		else if (terrainRandom == 2)
		{
			terrainCluster = random(6, 16);
			for (int p = i; p < terrainCluster + i; p++)
			{
				mapName[mapNameOffset] = 2;
				mapNameOffset++;
			}
		}
		else if (terrainRandom == 3)
		{
			terrainCluster = random(4, 14);
			for (int p = i; p < terrainCluster + i; p++)
			{
				mapName[mapNameOffset] = 3;
				mapNameOffset++;
			}
		}
		else
		{
			terrainCluster = random(12, 24);
			for (int p = i; p < terrainCluster + i; p++)
			{
				mapName[mapNameOffset] = 0;
				mapNameOffset++;
			}
		}
		i = i + terrainCluster - 1;
	}
}

void pullMapData(byte holeID, byte courseID)
{
	mapGenerator mapChosen(holeID, courseID);
	for (byte i = 0; i < 20; i++)
	{
		levelClusterSize[i] = mapChosen.mapInfoFeeder(i, CLUSTER);
		levelTerrainType[i] = mapChosen.mapInfoFeeder(i, TYPE);
	}
	holePosition = mapChosen.mapInfoFeeder(0, HOLEPOS);
}

void generateSetMaps()
{
	int clusterStartpoint = 0;
	int mapOffset = 0;
	for (byte i = 0; i < 15; i++)
	{
		for (int j = clusterStartpoint; j < levelClusterSize[i] + clusterStartpoint; j++)
		{
			if (j < NUMPIXELS_1)
			{
				if (levelTerrainType[i] == 0)
				{
					mapName[j] = 0;
				}
				else if (levelTerrainType[i] == 1)
				{
					mapName[j] = 1;
				}
				else if (levelTerrainType[i] == 2)
				{
					mapName[j] = 2;
				}
				else if (levelTerrainType[i] == 3)
				{
					mapName[j] = 3;
				}
				mapOffset++;
			}
		}
		clusterStartpoint = mapOffset;
	}
}
