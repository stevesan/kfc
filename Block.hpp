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

	Block() : isActive(false), hasSeed(false), seedSpriteId(-1), carSID(-1)
	{
	}

	void onGameOver(int seconds)
	{
		String<64> out;
		out << "LASTED " << seconds << " SECS";
		writeText( out );
	}

	bool spriteInUse[8];

	Side carSide;

	bool isActive;

	bool hasSeed;
	int seedSpriteId;
	int carSID;
	enum SeedType { Seed_Normal, Seed_Green, Seed_Red, Seed_None, NumSeeds } seedType;

	void propagateActive(Block* blocks)
	{
		isActive = true;

		for( int s = 0; s < NUM_SIDES; s++ )
		{
			int nbor = getNbor(s);

			if( nbor != -1 && blocks[nbor].isActive == false )
			{
				// ok, is it connected on the right side?
				if( blocks[nbor].canConnectTo(cubeId)
					&& canConnectTo(nbor) )
				{
					blocks[nbor].propagateActive(blocks);
				}
			}
		}
	}

	int activateSprite(const PinnedAssetImage& img)
	{
		logSprites();
		for( int sid = 0; sid < arraysize(spriteInUse); sid++ )
		{
			if( !spriteInUse[sid] )
			{
				spriteInUse[sid] = true;
				vid.sprites[sid].setImage( img, 0 );
				logSprites();
				//LOG("turning on sprite %d\n", i);
				return sid;
			}
		}
				logSprites();
		return -1;
	}

	void updateSprite(int sid, Float2 pos)
	{
		if( sid == -1 ) return;
		vid.sprites[sid].move(pos);
	}

	void updateSprite( int sid, const PinnedAssetImage& img, int frame )
	{
		if( sid == -1 ) return;
		vid.sprites[sid].setImage( img, frame );
	}

	void logSprites()
	{
	//LOG("cube %d: ", cubeId);
		for( int i = 0; i < arraysize(spriteInUse); i++ )
		{
			//LOG("%d, ", spriteInUse[i]);
		}
		//LOG("\n");
	}

	void deactivateSprite(int sid)
	{
	//LOG("cube %d deac %d\n", cubeId, sid);
		if( sid == -1 ) return;

		if(spriteInUse[sid])
		{
			spriteInUse[sid] = false;
			vid.sprites[sid].hide();
		}
	}

	void init(CubeID cube)
	{
			cubeId = cube;
			vid.initMode(BG0_SPR_BG1);
			vid.attach(cube);

			// set up arrow sprite positions
			/*
			vid.sprites[TOP+1].move( getSpritePos(getSidePos(TOP), Arrows, true) );
			vid.sprites[BOTTOM+1].move( getSpritePos(getSidePos(BOTTOM), Arrows, true) );
			vid.sprites[RIGHT+1].move( getSpritePos(getSidePos(RIGHT), Arrows, true) );
			vid.sprites[LEFT+1].move( getSpritePos(getSidePos(LEFT), Arrows, true) );
			*/

			for( int i = 0; i < arraysize(spriteInUse); i++ )
				spriteInUse[i] = false;
	}

	Side getExitSide(Side enterSide)
	{
		if( roadType == Road_I )
		{
			return oppositeSide(enterSide);
		}
		else if( roadType == Road_L )
		{
			if( enterSide == TOP )
				return LEFT;
			else
				return TOP;
		}
		return BOTTOM;
	}

	void takeSeed()
	{
		if( hasSeed )
		{
			hasSeed = false;
			deactivateSprite(seedSpriteId);
			seedSpriteId = -1;
		}
	}

	void randomize( Random& rand )
	{
		if( seedSpriteId != -1 )
		{
			deactivateSprite(seedSpriteId);
			seedSpriteId = -1;
		}

		if( carSID != -1 )
		{
			deactivateSprite(carSID);
			carSID = -1;
		}

		roadType = (RoadType)rand.randint( 0, NumRoads-1 );

		seedType = (SeedType)rand.randint( 0, NumSeeds-1 );

		carSide = NO_SIDE;
		/*
		carSide = (Side)rand.randint( 0, (int)NUM_SIDES );
		if(carSide == NUM_SIDES) carSide = NO_SIDE;
		if( carSide != NO_SIDE )
		{
			carSID = activateSprite(Car);
			updateSprite( carSID, Car, (int)oppositeSide(carSide) );
			updateSprite( carSID, toSpritePos( getSidePos(carSide), Car, true) );
		}
		*/

		// text mask
		vid.bg1.setMask(BG1Mask::filled(vec(0,14), vec(16,2)));

		// TEMP hasSeed = rand.randint(0,1);
		hasSeed = seedType != Seed_None;

		if(hasSeed)
		{
			if( seedType == Seed_Normal )
				seedSpriteId = activateSprite(Seed);
			else if( seedType == Seed_Red )
				seedSpriteId = activateSprite(RedSeed);
			else if( seedType == Seed_Green )
				seedSpriteId = activateSprite(GreenSeed);

			//LOG("cube %d seed sprite = %d\n", cubeId, seedSpriteId);
			Float2 c = {64, 64};
			vid.sprites[seedSpriteId].move(toSpritePos(c,Seed));
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

	//LOG("block %d is active? %d\n", cubeId, isActive);
		if(roadType == Road_I )
			vid.bg0.image(vec(0,0), isActive ? RoadI : RoadIgray, 0);
		else if(roadType == Road_L )
			vid.bg0.image(vec(0,0), isActive ? RoadL : RoadLgray, 0);


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
			case Road_L: return side == TOP || side == LEFT;
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
		Side mySide = getSideOf(other.cubeId);
		Side otherSide = other.getSideOf(cubeId);

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


	enum WarningType { Warning_None, Warning_NoBottom, Warning_BottomWrong, NumWarnings } warningType;

	enum TouchState { Touch_WrongSide, Touch_Good, Touch_None };

	VideoBuffer vid;
	int cubeId;

	TouchState side2touch[4];


};

#endif
