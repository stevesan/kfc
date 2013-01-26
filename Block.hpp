#ifndef _BLOCK_HPP
#define _BLOCK_HPP

#include <sifteo.h>
using namespace Sifteo;

#include "assets.gen.h"

//----------------------------------------
//  A single block, it can be of any type. Ie. a road, water, etc.
//----------------------------------------

class Block
{

public:

	enum RoadType { Road_I, Road_T, Road_Cross, NumRoads } roadType;

	bool side2HasCar[4];
	int side2CarRange[4];

	enum SeedType { Seed_NotReady, Seed_Ready, Seed_Rotten, NumSeeds } seedType;

	void init(CubeID cube)
	{
			vid.initMode(BG0_SPR_BG1);
			vid.attach(cube);
	}

	void randomize( Random& rand )
	{
		roadType = (RoadType)rand.randint( 0, NumRoads-1 );
		seedType = (SeedType)rand.randint( 0, NumSeeds-1 );

		for( int i = 0; i < 4; i++ )
		{
			side2HasCar[i] = rand.chance(0.5f);
			side2CarRange[i] = rand.randint(1,3);
		}

		// Our background is 18x18 to match BG0, and it seamlessly tiles
		int bgIdx = (int)roadType % RoadBgs.numFrames();
		vid.bg0.image(vec(0,0), RoadBgs, bgIdx);

/*
		int seedIdx = (int)seedType % SeedSprites.numFrames();
		vid.sprites[0].setImage( SeedSprites, seedIdx );
					const Float2 center = { 64 - 3.5f, 64 - 3.5f };
		vid.sprites[0].move( center );
		*/
	}

	void showChicken( Float2 pos )
	{
		vid.sprites[0].setImage( ChickenSprites, 0 );
		vid.sprites[0].move( pos );
	}

	void hideChicken()
	{
		vid.sprites[0].hide();
	}

	void update( TimeDelta timeStep )
	{
	}

	int getBotNbor()
	{
		if( vid.virtualNeighbors().hasCubeAt(BOTTOM) )
		{
			return vid.virtualNeighbors().cubeAt(BOTTOM);
		}
		else
			return -1;
	}

	Side getSideOf(int other)
	{
		return vid.virtualNeighbors().sideOf(other);
	}

private:

	VideoBuffer vid;

};

#endif
