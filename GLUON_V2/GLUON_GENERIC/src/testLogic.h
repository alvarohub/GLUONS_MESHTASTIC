
#include "BaseLogic.h"

// DECLARATION:

class TestLogic : public BaseLogic {

public:
	TestLogic() : BaseLogic("TEST") {}

	void init(bool firstTimeBuild)
	{
		if (firstTimeBuild) {

			setUpdateMode(ANY_INLET_BANG);

		} else {

			loadUpdateMode();

		}

		testBeepTimer = millis();

	}

	virtual void compute() override;
	virtual void evolve() override;

private:

	uint32_t testBeepTimer;
};

// ACTUAL IMPLEMENTATION (can put it on this file because I will only include once in main)

void TestLogic::evolve()
{

	// if ( millis() - testBeepTimer > 1000 ) {
	//
	// 	myChirpSpeaker.chirpUp();
	//
	// 	testBeepTimer = millis();
	// }


}

void TestLogic::compute()
{


}
