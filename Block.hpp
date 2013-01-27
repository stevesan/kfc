#ifndef _BLOCK_HPP
#define _BLOCK_HPP

#include <sifteo.h>
using namespace Sifteo;

#include "assets.gen.h"

//----------------------------------------
//  A single block, it can be of any type. Ie. a road, water, etc.
//----------------------------------------

const int kTurnSprite = 6;

class Block
{

public:

	Block() : isActive(false)
	{
	}

	void onGameOver()
	{
		writeText("Game over!!!");
		isActive = false;
	}

	bool spriteInUse[8];

	bool isActive;

	bool hasSeed;
	int seedSpriteId;

	void propagateActive(Block* blocks)
	{
		isActive = true;

		for( int s = 0; s < NUM_SIDES; s++ )
		{
			int nbor = getNbor(s);

			if( nbor != -1 && blocks[nbor].isActive == false )
			{
				// ok, is it connected on the right side?
				if( blocks[nbor].canConnectTo(id)
					&& canConnectTo(nbor) )
				{
					blocks[nbor].propagateActive(blocks);
				}
			}
		}
	}

	int activateSprite(const PinnedAssetImage& img)
	{
		for( int i = 0; i < 8; i++ )
		{
			if( !spriteInUse[i] )
			{
				spriteInUse[i] = true;
				vid.sprites[i].setImage( img, 0 );
				LOG("turning on sprite %d\n", i);
				return i;
			}
		}
		return -1;
	}

	void updateSprite(int i, Float2 pos)
	{
		vid.sprites[i].move(pos);
	}

	void deactivateSprite(int i)
	{
		if(spriteInUse[i] )
		{
			spriteInUse[i] = false;
			vid.sprites[i].hide();
		}
	}

	void init(CubeID cube)
	{
			id = cube;
			vid.initMode(BG0_SPR_BG1);
			vid.attach(cube);

			// set up arrow sprite positions
			/*
			vid.sprites[TOP+1].move( centerPos(getSidePos(TOP), Arrows, true) );
			vid.sprites[BOTTOM+1].move( centerPos(getSidePos(BOTTOM), Arrows, true) );
			vid.sprites[RIGHT+1].move( centerPos(getSidePos(RIGHT), Arrows, true) );
			vid.sprites[LEFT+1].move( centerPos(getSidePos(LEFT), Arrows, true) );
			*/

			for( int i = 0; i < 4; i++ )
			{
				spriteInUse[i] = false;
			}
	}

	Side getExitSide(Side enterSide)
	{
		if( roadType == Road_I )
		{
			return oppositeSide(enterSide);
		}
		else if( roadType == Road_L )
		{
			if( enterSide == BOTTOM )
				return LEFT;
			else
				return BOTTOM;
		}
		return BOTTOM;
	}

	void randomize( Random& rand )
	{
		if( seedSpriteId != -1 )
		{
			deactivateSprite(seedSpriteId);
		}

		roadType = (RoadType)rand.randint( 0, NumRoads-1 );
		seedType = (SeedType)rand.randint( 0, NumSeeds-1 );

		for( int i = 0; i < 4; i++ )
		{
			side2HasCar[i] = rand.chance(0.5f);
			side2CarRange[i] = rand.randint(1,3);
		}

		vid.bg1.setMask(BG1Mask::filled(vec(0,14), vec(16,2)));

		hasSeed = rand.randint(0,1);

		if(hasSeed)
		{
			seedSpriteId = activateSprite(Seed);
			Float2 c = {64, 64};
			vid.sprites[seedSpriteId].move(c);
		}

/*
		int seedIdx = (int)seedType % SeedSprites.numFrames();
		vid.sprites[0].setImage( SeedSprites, seedIdx );
					const Float2 center = { 64 - 3.5f, 64 - 3.5f };
		vid.sprites[0].move( center );
		*/

		// setup arrow sprites depending on road type
		/*
		for( int side = 0; side < NUM_SIDES; side++ )
		{
			if( !isSideConnectable((Side)side) )
				vid.sprites[side+1].hide();
		}
		*/
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

		int bgStart = isActive ? 0 : 2;
		vid.bg0.image(vec(0,0), RoadBgs, bgStart + roadType);

		// update side graphics
		/*
		for( int side = 0; side < 4; side++ )
		{
			if( !isSideConnectable(side) )
				continue;

			vid.sprites[side+1].setImage( Arrows, side2touch[side] );
		}
		*/
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
			case Road_L: return side == LEFT || side == BOTTOM;
			default: return false;
		}
	}

	bool isSideConnectable(int side)
	{
		return isSideConnectable((Side)side);
	}

	bool isSideConnected(Side side)
	{
		return side2touch[side] == Touch_Good;
	}

	Side getSideOf(int otherId)
	{
		return vid.virtualNeighbors().sideOf(otherId);
	}

	bool canConnectTo( int otherId )
	{
		return isSideConnectable( vid.virtualNeighbors().sideOf(otherId) );
	}

	void resetTouchState()
	{
		for( int i = 0; i < 4; i++ )
			side2touch[i] = Touch_None;
	}

	void preUpdate()
	{
		resetTouchState();
	}

	void onCubeTouching(Block& other)
	{
		Side mySide = getSideOf(other.id);
		Side otherSide = other.getSideOf(id);

		if( isSideConnectable(mySide) )
		{
			if( other.isSideConnectable(otherSide) )
			{
				side2touch[mySide] = Touch_Good;		
			}
			else
				side2touch[mySide] = Touch_WrongSide;
		}
		else
			side2touch[mySide] = Touch_None;
	}

private:

	enum RoadType { Road_I, Road_L, NumRoads } roadType;

	enum SeedType { Seed_NotReady, Seed_Ready, Seed_Rotten, NumSeeds } seedType;

	enum WarningType { Warning_None, Warning_NoBottom, Warning_BottomWrong, NumWarnings } warningType;

	enum TouchState { Touch_WrongSide, Touch_Good, Touch_None };

	VideoBuffer vid;
	int id;

	bool side2HasCar[4];
	int side2CarRange[4];
	TouchState side2touch[4];


};

#endif
