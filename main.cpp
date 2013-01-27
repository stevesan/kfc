/*
 * Sifteo SDK Example.
 */

#include <sifteo.h>
using namespace Sifteo;

#include "assets.gen.h"

#include "Utils.hpp"
#include "Block.hpp"
#include "Anim.hpp"

//----------------------------------------
//  
//----------------------------------------
static const float gHatchTime = 0.8;
static const int gSeedsPerHatch = 1;
static const unsigned gNumCubes = 6;
static const unsigned gMaxNumChicks = 24;
Block gBlocks[ gNumCubes ];

Random gRandom;

enum { Anim_ChickenWalk,
Anim_ChickWalk,
Anim_ChickWalkRight,
Anim_ChickWalkLeft,
Anim_ChickWalkUp,
Anim_Hatch,
Anim_ChickenDeath,
Anim_ChickDeath,
NumAnims };

Animation gAnims[NumAnims];

void gInitAnims()
{
	//gAnims[Anim_ChickenWalk].addFrame( AnimChicken, 0 );
	//gAnims[Anim_ChickenWalk].addFrame( AnimChicken, 1 );
	//gAnims[Anim_ChickenWalk].addFrame( AnimChicken, 2 );

	gAnims[Anim_ChickWalk].addFrame( AnimChick, 0 );
	gAnims[Anim_ChickWalk].addFrame( AnimChick, 1 );
	gAnims[Anim_ChickWalk].addFrame( AnimChick, 2 );
	gAnims[Anim_ChickWalk].fps = 8.0;

	gAnims[Anim_ChickWalkRight].addFrame( AnimChick, 3 );
	gAnims[Anim_ChickWalkRight].addFrame( AnimChick, 4 );
	gAnims[Anim_ChickWalkRight].fps = 8.0;
	gAnims[Anim_ChickWalkLeft].addFrame( AnimChick, 5 );
	gAnims[Anim_ChickWalkLeft].addFrame( AnimChick, 6 );
	gAnims[Anim_ChickWalkLeft].fps = 8.0;
	gAnims[Anim_ChickWalkUp].addFrame( AnimChick, 7 );
	gAnims[Anim_ChickWalkUp].addFrame( AnimChick, 8 );
	gAnims[Anim_ChickWalkUp].addFrame( AnimChick, 9 );
	gAnims[Anim_ChickWalkUp].fps = 8.0;

	gAnims[Anim_Hatch].addFrame( AnimChick, 10 );
	gAnims[Anim_Hatch].addFrame( AnimChick, 11 );
	gAnims[Anim_Hatch].addFrame( AnimChick, 12 );
	gAnims[Anim_Hatch].addFrame( AnimChick, 13 );
	gAnims[Anim_Hatch].fps = 8.0;

	gAnims[Anim_ChickenDeath].addFrame( AnimChickenDeath, 0 );
	gAnims[Anim_ChickenDeath].addFrame( AnimChickenDeath, 1 );
	gAnims[Anim_ChickenDeath].addFrame( AnimChickenDeath, 2 );

	gAnims[Anim_ChickDeath].addFrame( AnimChickDeath, 0 );
	gAnims[Anim_ChickDeath].addFrame( AnimChickDeath, 1 );
	gAnims[Anim_ChickDeath].addFrame( AnimChickDeath, 2 );
}

//----------------------------------------
//  
//----------------------------------------
static AssetSlot MainSlot = AssetSlot::allocate()
    .bootstrap(GameAssets);

static Metadata M = Metadata()
    .title("Chicken Run, GGJ2013")
    .package("com.sifteo.sdk.stars", "1.0")
    .icon(Icon)
    .cubeRange(gNumCubes);

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

		AnimPlayer animer;

		Entity() :
			type(Type_Chicken),
			spriteId(-1), blockId(-1), state(Entity_Unused)
			{
			}

		const PinnedAssetImage& getCurrentImage()
		{
			if( animer.anim == NULL )
				return AnimChick;
			else
				return *animer.getCurrAsset();
		}

		void reset(Block* blocks)
		{
			if( blockId != -1 && spriteId != -1)
			{
				blocks[blockId].deactivateSprite(spriteId);
				spriteId = -1;
			}

			blockId = -1;
			state = Entity_Unused;

			animer.anim = &gAnims[ Anim_ChickWalk ];
		}

		Float2 getCenter()
		{
			return centerPos;
		}

		Float2 getSpritePos()
		{
			return toSpritePos( centerPos, getCurrentImage() );
		}

		void moveToBlock( Block* blocks, int newBid, bool isHatch = false )
		{
			if( isHatch )
			{
				animer.setAnim( &gAnims[Anim_Hatch], false );
				state = Entity_Hatching;
				type = Type_Chick;
			}
			else
			{
				/*TODO: change to chicken*/
				animer.setAnim( &gAnims[Anim_ChickWalk], true );
				state = Entity_Alive;
			}

			if( blockId != -1 && spriteId != -1 )
			{
				blocks[blockId].deactivateSprite(spriteId);
				spriteId = -1;
				LOG("deact'd block %d sprite %d\n", blockId, spriteId );
			}

			int oldBid = blockId;
			blockId = newBid;
			Block& newBlock = blocks[blockId];

			spriteId = newBlock.activateSprite(getCurrentImage());
			//LOG("ent sprite = %d\n", spriteId);
			newBlock.updateSprite(spriteId, getSpritePos());

			if( oldBid != -1 )
				enterSide = newBlock.getSideOf(oldBid);
			else
				enterSide = TOP;

			dir = -1 * sideDirection(enterSide);

			if( !isHatch )
			{
				centerPos = getSidePos(enterSide);
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

			animer.update(dt);

			if( state != Entity_Unused )
			{
				Block& blk = blocks[blockId];
				const Frame& f = animer.getCurrFrame();
				if( f.asset != NULL )
				{
					blk.updateSprite( spriteId, *f.asset, f.frameNum );
				}
			}

			if( state == Entity_Hatching )
			{
				if( animer.getIsDone() )
				{
					// hatched!
					animer.setAnim( &gAnims[Anim_ChickWalk], true );
					state = Entity_Alive;
					rv.didHatch = true;
				}
			}
			else if( state == Entity_Alive )
			{
				Block& blk = blocks[blockId];
				centerPos += dt * speed * dir;
				blk.updateSprite( spriteId, getSpritePos() );

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
						Side exitSide = blk.getExitSide(enterSide);
						dir = sideDirection( exitSide );
						LOG("new dir = %f %f\n", dir.x, dir.y);

						if( exitSide == LEFT )
							animer.setAnim( &gAnims[Anim_ChickWalkLeft], true );
						else 
							animer.setAnim( &gAnims[Anim_ChickWalk], true );
					}
				}

				// Did we exit?
				Side exitSide = NO_SIDE;
				if( centerPos.x < 0 )
					exitSide = LEFT;
				else if( centerPos.x > 128.0 )
					exitSide = RIGHT;
				else if( centerPos.y < 0 )
					exitSide = TOP;
				else if( centerPos.y > 128.0 )
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
						onDie();
					}
				}
			}

			return rv;
		}

		bool isDead() { return state == Entity_Dead; }

		void onDie()
		{
			if( type == Type_Chicken )
				animer.setAnim( &gAnims[Anim_ChickenDeath], false );
			else
				animer.setAnim( &gAnims[Anim_ChickDeath], false );

			state = Entity_Dead;
		}
};

class State
{
public:

	AudioChannel musicChan, effectsChan, seedChan;

	enum { State_Playing, State_GameOver } state;
	Entity chicken;
	Entity chicks[gNumCubes];
	int nextUnusedChick;
	Block blocks[gNumCubes];
	int lastChickenBlock;
	int numSeeds;

	float gameOverTime = 0.0;

	State() :
		musicChan(0), effectsChan(1), seedChan(2),
		state(State_Playing) ,
		nextUnusedChick(0),
		lastChickenBlock(0)
	{
		for (unsigned i = 0; i < arraysize(blocks); i++)
		{
			blocks[i].init(i);
		}
	}

	void reset()
	{

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

		for (unsigned i = 0; i < arraysize(blocks); i++)
		{
			blocks[i].randomize(gRandom);
		}
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
		chicken.update(blocks, dt);
		for( int i = 0; i < arraysize(chicks); i++ )
		{
			Entity::UpdateResult crv2 = chicks[i].update(blocks, dt);
		}

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
			seedChan.stop();
			seedChan.play(EatSeedSnd);
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

				effectsChan.stop();
				effectsChan.play( HatchSnd );
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
			{
				//effectsChan.stop();
				//effectsChan.play( HatchSnd );
			}
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
				effectsChan.stop();
				effectsChan.play(ChickenDiedSnd);
			}
			else
			{
				rv.chickDied = true;
				musicChan.stop();
				effectsChan.stop();
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
					effectsChan.stop();
					effectsChan.play(ChickDiedSnd);
					chicks[i].onDie();
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
	gInitAnims();

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
