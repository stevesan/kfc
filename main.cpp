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

		Entity() :
			spriteId(-1), img(NULL), blockId(-1)
			{
			}

		void moveToBlock( Block* blocks, int newBid )
		{
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

		int spriteId;	// within the block's sprites
		Float2 pos;
		Float2 dir;
		float speed;
		const PinnedAssetImage* img;

		int blockId;
		Side enterSide;
		bool reachedCenter;

		void update( Block* blocks, float dt )
		{
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
				}
			}
		}
};

void main()
{
		AudioChannel chan(0);
		chan.play(Music, AudioChannel::REPEAT);

    static Block blocks[gNumBlocksBuffered];

    for (unsigned i = 0; i < arraysize(blocks); i++)
		{
        blocks[i].init(i);
				blocks[i].randomize(gRandom);
		}

		Entity chicken;
		chicken.speed = 50.0;
		chicken.img = &Chicken;
		chicken.moveToBlock( blocks, 0 );

		Entity chicks[3];
		for( int i = 0; i < arraysize(chicks); i++ )
		{
			chicks[i].speed = 50.0;
			chicks[i].img = &Chick;
			chicks[i].moveToBlock( blocks, 0);
			chicks[i].pos.y += (i+1)*32;
		}
    
    TimeStep ts;
    while (1)
		{
			float dt = (float)ts.delta();

			// move chicken
			chicken.update(&blocks[0], dt);

			for( int i = 0; i < arraysize(chicks); i++ )
			{
				chicks[i].update( &blocks[0], dt );
			}

			// update is-connected-to-chicken state
			for( int i = 0; i < arraysize(blocks); i++ )
			{
				blocks[i].isActive = false;
			}

			blocks[chicken.blockId].propagateActive(blocks);

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

				block.update(ts.delta());
			}

			System::paint();
			ts.next();
    }
}
