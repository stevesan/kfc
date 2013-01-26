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

	enum WarningType { Warning_None, Warning_NoBottom, Warning_BottomWrong, NumWarnings } warningType;

	bool side2connected[4];

	void init(CubeID cube)
	{
			vid.initMode(BG0_SPR_BG1);
			vid.attach(cube);

			// set up arrow sprite positions
			int hw = Arrows.pixelWidth()/2;
			int hh = Arrows.pixelHeight()/2;
			int cx = 64;
			int cy = 64;
			Int2 p;
			p.set(cx-hw, 0);
			vid.sprites[TOP+1].move(p);
			p.set(128-2*hw, cy-hh);
			vid.sprites[RIGHT+1].move( p );
			p.set(cx-hw, 128-2*hh);
			vid.sprites[BOTTOM+1].move( p );
			p.set(0, cy-hh);
			vid.sprites[LEFT+1].move( p );

			// temp
			for( int side = 0; side < NUM_SIDES; side++ )
			{
				vid.sprites[side+1].setImage( Arrows, 0 );
			}
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

		vid.bg1.setMask(BG1Mask::filled(vec(0,14), vec(16,2)));

/*
		int seedIdx = (int)seedType % SeedSprites.numFrames();
		vid.sprites[0].setImage( SeedSprites, seedIdx );
					const Float2 center = { 64 - 3.5f, 64 - 3.5f };
		vid.sprites[0].move( center );
		*/

		// setup arrow sprites depending on road type
		for( int side = 0; side < NUM_SIDES; side++ )
		{
			if( !isSideConnectable((Side)side) )
			{
				vid.sprites[side+1].hide();
			}
		}
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
		switch( warningType )
		{
			case Warning_NoBottom:
				writeText("NO BOTTOM!             ");
				break;
			case Warning_BottomWrong:
				writeText("Wrong side!            ");
				break;
			default:
				writeText("                       ");
				break;
		}

		// update side graphics
		for( int side = 0; side < 4; side++ )
		{
			if( isSideConnectable((Side)side) )
			{
				if( isSideConnected((Side)side) )
					vid.sprites[side+1].setImage( Arrows, 1 );
				else
					vid.sprites[side+1].setImage( Arrows, 0 );
			}
		}
	}

	void writeText(const char *str)
	{
			// Text on BG1, in the 16x2 area we allocated
			vid.bg1.text(vec(0,14), Font, str);
	}

	int getNbor(int side) { return getNbor((Side)side); }

	int getNbor(Side side)
	{
		if( vid.virtualNeighbors().hasCubeAt(side) )
		{
			return vid.virtualNeighbors().cubeAt(side);
		}
		else
			return -1;
	}

	int getBotNbor()
	{
		return getNbor(BOTTOM);
	}

	bool isSideConnectable(Side side)
	{
		switch(roadType)
		{
			case Road_I: return side == BOTTOM || side == TOP;
			case Road_T: return side == LEFT || side == RIGHT || side == BOTTOM;
			case Road_Cross:
			default: return true;
		}
	}

	bool isSideConnected(Side side)
	{
		return side2connected[side];
	}

	Side getSideOf(int otherId)
	{
		return vid.virtualNeighbors().sideOf(otherId);
	}

	void resetConnected()
	{
		for( int i = 0; i < 4; i++ )
			side2connected[i] = false;
	}

	void preUpdate()
	{
		resetConnected();
		hideChicken();
	}

	void onCubeTouching(int otherId, Side side)
	{
		if( isSideConnectable(side) )
		{
			side2connected[side] = true;		
		}
	}

private:

	VideoBuffer vid;

};

#endif
