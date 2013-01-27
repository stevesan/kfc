/*
 * Sifteo SDK Example.
 */

#include <sifteo.h>
using namespace Sifteo;

#include "assets.gen.h"

#include "Utils.hpp"
#include "Block.hpp"

//----------------------------------------
//  
//----------------------------------------
static const unsigned gNumBlocksBuffered = 6;
static const unsigned gMaxNumChicks = 24;
Block gBlocks[ gNumBlocksBuffered ];

Random gRandom;

//----------------------------------------
//  
//----------------------------------------
static AssetSlot MainSlot = AssetSlot::allocate()
    .bootstrap(GameAssets);

static Metadata M = Metadata()
    .title("Chicken Run, GGJ2013")
    .package("com.sifteo.sdk.stars", "1.0")
    .icon(Icon)
    .cubeRange(gNumBlocksBuffered);

class Entity
{
	public:


		bool isChicken;
		int spriteId;	// within the block's sprites
		Float2 pos;
		Float2 dir;
		float speed;
		const PinnedAssetImage* img;

		int blockId;
		enum { Entity_Unused, Entity_Alive, Entity_Dead } state;
		Side enterSide;
		bool reachedCenter;

		Entity() :
			isChicken(false),
			spriteId(-1), img(NULL), blockId(-1), state(Entity_Unused)
			{
			}

		void reset(Block* blocks)
		{
			if( blockId != -1 && spriteId != -1)
			{
				blocks[blockId].deactivateSprite(spriteId);
			}

			spriteId = -1;
			img = NULL;
			blockId = -1;
			state = Entity_Unused;
		}

		void moveToBlock( Block* blocks, int newBid )
		{
			state = Entity_Alive;
			if( blockId != -1 )
			{
				blocks[blockId].deactivateSprite(spriteId);
			}

			int oldBid = blockId;
			blockId = newBid;
			Block& newBlock = blocks[blockId];

			spriteId = newBlock.activateSprite(*img);

			if( oldBid != -1 )
				enterSide = newBlock.getSideOf(oldBid);
			else
				enterSide = TOP;

			dir = -1 * sideDirection(enterSide);

			pos = centerPos(
					getSidePos(enterSide),
					*img, true );

			reachedCenter = false;
		}

		void update( Block* blocks, float dt)
		{
			if( state != Entity_Alive )
				return;

			Block& blk = blocks[blockId];
			pos += dt * speed * dir;
			blk.updateSprite( spriteId, pos );

			// Chicken reached the center?
			if( !reachedCenter )
			{
				Float2 center = {64, 64};

				if( (pos-centerPos(center,*img)).len() < 2 )
				{
					reachedCenter = true;

					if( blk.hasSeed )
					{
						//rv.gotSeed = true;
						LOG("Got seed\n");
					}

					// Redirect towards the exit
					dir = sideDirection( blk.getExitSide(enterSide) );
					LOG("new dir = %f %f\n", dir.x, dir.y);
				}
			}

			// Did we exit?
			Side exitSide = NO_SIDE;
			if( pos.x < 0 )
				exitSide = LEFT;
			else if( pos.x > 128.0 )
				exitSide = RIGHT;
			else if( pos.y < 0 )
				exitSide = TOP;
			else if( pos.y > 128.0 )
				exitSide = BOTTOM;

			// Chicken walking to next block?
			if( exitSide != NO_SIDE )
			{
				if( blk.isSideConnected(exitSide) )
				{
					moveToBlock( blocks, blk.getNbor(exitSide) );
				}
				else
				{
					// game over! do nothing for now
					pos = centerPos(
							getSidePos(oppositeSide(exitSide)),
							*img, true );
					state = Entity_Dead;
				}
			}
		}

		bool isDead() { return state == Entity_Dead; }
};


class State
{
public:

	enum { State_Playing, State_GameOver } state;
	Entity chicken;
	Entity chicks[3];
	Block blocks[gNumBlocksBuffered];
	int lastChickenBlock;
	int numSeeds;

	State() :
		state(State_Playing) ,
		lastChickenBlock(0)
		{
		}

	void reset()
	{
    for (unsigned i = 0; i < arraysize(blocks); i++)
		{
        blocks[i].init(i);
				blocks[i].randomize(gRandom);
		}

		chicken.reset(blocks);
		for( int i = 0; i < 3; i++ )
		{
			chicks[i].reset(blocks);
		}

		chicken.isChicken = true;
		chicken.speed = 50.0;
		chicken.img = &Chicken;
		chicken.moveToBlock( blocks, lastChickenBlock );

		for( int i = 0; i < arraysize(chicks); i++ )
		{
			chicks[i].speed = 50.0;
			chicks[i].img = &Chick;
			//chicks[i].moveToBlock( blocks, 0);
			//chicks[i].pos.y += (i+1)*32;
		}

		numSeeds = 0;

		state = State_Playing;
	}

	void update(float dt)
	{
		chicken.update(blocks, dt);
		lastChickenBlock = chicken.blockId;

		for( int i = 0; i < arraysize(chicks); i++ )
			chicks[i].update(blocks, dt);

		// check death
		bool anyDead = chicken.isDead();
		for( int i = 0; i < arraysize(chicks); i++ )
		{
			anyDead = anyDead || chicks[i].isDead();
		}

		if( anyDead )
		{
			state = State_GameOver;
			for( int i = 0; i < arraysize(blocks); i++ )
			{
				blocks[i].onGameOver();
			}
		}
		else
		{
			// update is-connected-to-chicken state
			for( int i = 0; i < arraysize(blocks); i++ )
			{
				blocks[i].isActive = false;
			}
			blocks[chicken.blockId].propagateActive(blocks);

			// check if any blocks used by chicks are in-active
			for( int i = 0; i < arraysize(chicks); i++ )	
			{
				if( chicks[i].state != Entity::Entity_Alive )
					continue;
				if( blocks[ chicks[i].blockId ].isActive == false )
				{
					LOG("chick %d died due to block %d in active\n", i, chicks[i].blockId );
					state = State_GameOver;
				}
			}

			// update blocks and touching
			for (unsigned i = 0; i < arraysize(blocks); i++)
			{
				Block& block = blocks[i];
				// update connected states
				block.preUpdate();

				for( int side = 0; side < NUM_SIDES; side++ )
				{
					int otherId = block.getNbor(side);

					if( otherId != -1 )
					{
						block.onCubeTouching(blocks[otherId]);
					}
				}

				block.update(dt);
			}

		}
	}

	bool isGameOver()
	{
		return state == State_GameOver;
	}

	bool isPlaying()
	{
		return state == State_Playing;
	}

private:
};

void main()
{
		AudioChannel chan(0);
		chan.play(Music, AudioChannel::REPEAT);

		State state;
		float gameOverTime = 0.0;
		state.reset();
    
    TimeStep ts;
    while (1)
		{
			float dt = (float)ts.delta();

			if( state.isPlaying() )
			{
				state.update( dt );

				if( state.isGameOver() )
				{
					gameOverTime = 0.0;
				}
			}
			else if( state.isGameOver() )
			{
				gameOverTime += dt;
				if( gameOverTime > 5.0 )
				{
					LOG("game over time = %f\n", gameOverTime);
					state.reset();
				}
			}

			System::paint();
			ts.next();
    }
}
