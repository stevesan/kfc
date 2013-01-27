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
static const float gHatchTime = 0.8;
static const int gSeedsPerHatch = 1;
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

	enum {Type_Chicken, Type_Chick, NumTypes} type;
		int spriteId;	// within the block's sprites
		Float2 centerPos;
		Float2 dir;
		float speed;

		int blockId;
		enum { Entity_Unused, Entity_Hatching, Entity_Alive, Entity_Dead } state;
		Side enterSide;
		bool reachedCenter;
		float hatchingTime = 0.0;

		Entity() :
			type(Type_Chicken),
			spriteId(-1), blockId(-1), state(Entity_Unused)
			{
			}

		const PinnedAssetImage& getCurrentImage()
		{
			if( type == Type_Chicken ) return Chicken;
			else return Chick;
		}

		void reset(Block* blocks)
		{
			if( blockId != -1 && spriteId != -1)
			{
				blocks[blockId].deactivateSprite(spriteId);
			}

			spriteId = -1;
			blockId = -1;
			state = Entity_Unused;
		}

		Float2 getCenter()
		{
			return centerPos;
		}

		Float getSpritePos()
		{
			return getSpritePos( centerPos, getCurrentImage() );
		}

		void moveToBlock( Block* blocks, int newBid, bool isHatch = false )
		{
			if( isHatch )
			{
				state = Entity_Hatching;
				type = Type_Chick;
				hatchingTime = 0.0;
			}
			else
			{
				state = Entity_Alive;
			}

			if( blockId != -1 )
			{
				blocks[blockId].deactivateSprite(spriteId);
			}

			int oldBid = blockId;
			blockId = newBid;
			Block& newBlock = blocks[blockId];

			spriteId = newBlock.activateSprite(getCurrentImage());
			newBlock.updateSprite(spriteId, pos);

			if( oldBid != -1 )
				enterSide = newBlock.getSideOf(oldBid);
			else
				enterSide = TOP;

			dir = -1 * sideDirection(enterSide);

			if( !isHatch )
			{
				pos = getSpritePos(
						getSidePos(enterSide),
						getCurrentImage(), true );
			}

			reachedCenter = false;
		}

		class UpdateResult
		{
		public:
			bool gotSeed;
			bool leftBlock;
			int oldBlockId;
			bool didHatch;

			UpdateResult() :
				gotSeed(false),
				leftBlock(false),
				oldBlockId(-1),
				didHatch( false)
			{
			}
		};

		virtual UpdateResult update( Block* blocks, float dt)
		{
			UpdateResult rv;

			if( state == Entity_Hatching )
			{
				hatchingTime += dt;

				if( hatchingTime > gHatchTime )
				{
					blocks[blockId].updateSprite( spriteId, getCurrentImage(), 0 );
					state = Entity_Alive;
					rv.didHatch = true;
				}
			}
			else if( state == Entity_Alive )
			{
				Block& blk = blocks[blockId];
				pos += dt * speed * dir;
				blk.updateSprite( spriteId, pos );

				// Chicken reached the center?
				if( !reachedCenter )
				{
					Float2 center = {64, 64};
					float centerDist = dot(center, dir);
					float entityDist = dot(dir, getCenter());

					if( entityDist > centerDist )
					{
						reachedCenter = true;

						if( type == Type_Chicken && blk.hasSeed )
						{
							rv.gotSeed = true;
							blk.takeSeed();
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
						rv.leftBlock = true;
						rv.oldBlockId = blockId;
						moveToBlock( blocks, blk.getNbor(exitSide) );
					}
					else
					{
						// game over! do nothing for now
						pos = getSpritePos(
								getSidePos(oppositeSide(exitSide)),
								getCurrentImage(), true );
						state = Entity_Dead;
					}
				}
			}

			return rv;
		}

		bool isDead() { return state == Entity_Dead; }
};

class State
{
public:

	AudioChannel musicChan, effectsChan;

	enum { State_Playing, State_GameOver } state;
	Entity chicken;
	Entity chicks[6];
	int nextUnusedChick;
	Block blocks[gNumBlocksBuffered];
	int lastChickenBlock;
	int numSeeds;

	float gameOverTime = 0.0;

	State() :
		musicChan(0), effectsChan(1),
		state(State_Playing) ,
		nextUnusedChick(0),
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
		for( int i = 0; i < arraysize(chicks); i++ )
		{
			chicks[i].reset(blocks);
		}
		nextUnusedChick = 0;

		chicken.type = Entity::Type_Chicken;
		chicken.speed = 50.0;
		chicken.moveToBlock( blocks, lastChickenBlock );

		for( int i = 0; i < arraysize(chicks); i++ )
		{
			chicks[i].speed = 50.0;
		}

		numSeeds = 0;
		state = State_Playing;
		musicChan.play(Music, AudioChannel::REPEAT);
	}

	void triggerGameOver()
	{
		state = State_GameOver;
		gameOverTime = 0.0;
		for( int i = 0; i < arraysize(blocks); i++ )
		{
			blocks[i].onGameOver();
		}
	}

	void randomizeBlockIfUnused(int blockId)
	{
	LOG("maybe randomize block %d\n", blockId);
		if( chicken.blockId == blockId )
			return;
	LOG("not used by chicken %d\n", blockId);

		for( int i = 0; i < arraysize(chicks); i++ )
		{
			if( chicks[i].state != Entity::Entity_Unused && chicks[i].blockId == blockId )
				return;
		}

		//LOG("randomizing block %d\n", blockId);
		blocks[blockId].randomize(gRandom);
	}

	struct UpdateResult
	{
		bool chickDied, chickenDied;
		UpdateResult() : chickDied(false), chickenDied(false)
		{
		}
	};

	UpdateResult update(float dt)
	{
		if( state == State_Playing )
		{
			return updatePlaying(dt);
		}
		else if( state == State_GameOver )
		{
			return updateGameOver(dt);
		}
		return UpdateResult();
	}

	UpdateResult updateGameOver(float dt)
	{
		UpdateResult rv;
		gameOverTime += dt;
		if( gameOverTime > 1.0 )
		{
			reset();
		}
		return rv;
	}

	UpdateResult updatePlaying(float dt)
	{
		UpdateResult rv;

		Entity::UpdateResult crv = chicken.update(blocks, dt);
		lastChickenBlock = chicken.blockId;
		if( crv.gotSeed )
		{
			effectsChan.play(EatSeedSnd);
			numSeeds++;
			//LOG("got seed, now %d\n", numSeeds);

			if( numSeeds >= gSeedsPerHatch && nextUnusedChick < arraysize(chicks) )
			{
				// HATCH
				Entity& parent = ( nextUnusedChick == 0 ? chicken : chicks[nextUnusedChick-1] );
				Entity& chick = chicks[nextUnusedChick++];

				chick.centerPos = parent.getCenter();
				chick.moveToBlock( blocks, parent.blockId, true );
				chick.reachedCenter = parent.reachedCenter;
				chick.dir = parent.dir;
				chick.enterSide = parent.enterSide;
				//LOG("hatching\n");
			}
		}

		if( crv.leftBlock )
			randomizeBlockIfUnused(crv.oldBlockId);

		for( int i = 0; i < arraysize(chicks); i++ )
		{
			Entity::UpdateResult crv2 = chicks[i].update(blocks, dt);

			if( crv2.leftBlock )
				randomizeBlockIfUnused(crv2.oldBlockId);

			if( crv2.didHatch )
				effectsChan.play( HatchSnd );
		}

		// check death
		bool anyDead = chicken.isDead();

		for( int i = 0; i < arraysize(chicks); i++ )
		{
			anyDead = anyDead || chicks[i].isDead();
		}

		if( anyDead )
		{
			triggerGameOver();

			if( chicken.isDead() )
			{
				rv.chickenDied = true;
				musicChan.stop();
				effectsChan.play(ChickenDiedSnd);
			}
			else
			{
				rv.chickDied = true;
				musicChan.stop();
				effectsChan.play(ChickDiedSnd);
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
					triggerGameOver();
					musicChan.stop();
					effectsChan.play(ChickDiedSnd);
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

		return rv;
	}

	bool isPlaying()
	{
		return state == State_Playing;
	}

private:
};

void main()
{

		State state;
		state.reset();
    
    TimeStep ts;
    while (1)
		{
			float dt = (float)ts.delta();
			State::UpdateResult rv = state.update( dt );

			System::paint();
			ts.next();
    }
}
