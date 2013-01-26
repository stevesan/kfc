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

//----------------------------------------
//  
//----------------------------------------
class KFCGame {
public:

    static const unsigned numChickens = 4;
    
    static const float textSpeed = 0.2f;
    static const float bgScrollSpeed = 10.0f;
    static const float bgTiltSpeed = 1.0f;
    static const float starEmitSpeed = 60.0f;
    static const float starTiltSpeed = 3.0f;

    void init(CubeID cube)
    {
        frame = 0;
        bg.x = 0;
        bg.y = 0;

        vid.initMode(BG0_SPR_BG1);
        vid.attach(cube);
        
        for (unsigned i = 0; i < numChickens; i++)
            initStar(i);

        // Our background is 18x18 to match BG0, and it seamlessly tiles
        vid.bg0.image(vec(0,0), TestBg, cube % TestBg.numFrames());

        // Allocate 16x2 tiles on BG1 for text at the bottom of the screen
        vid.bg1.setMask(BG1Mask::filled(vec(0,14), vec(16,2)));
    }
    
    void update(TimeDelta timeStep)
    {
        /*
         * Action triggers
         */
        
        switch (frame) {
        
        case 0:
            text.set(0, -20);
            textTarget = text;
            break;
        
        case 100:
            writeText(" Hello traveler ");
            textTarget.y = 8;
            break;

        case 200:
            textTarget.y = -20;
            break;
            
        case 250:
            writeText(" Tilt me to fly ");
            textTarget.y = 8;
            break;
            
        case 350:
            textTarget.y = -20;
            break;
        
        case 400:
            writeText(" Enjoy subspace ");
            textTarget.y = 64-8;
            break;
        
        case 500:
            textTarget.x = 128;
            break;
        
        case 550:
            text.x = 128;
            textTarget.x = 0;
            writeText(" and be careful ");
            break;

        case 650:
            textTarget.y = -20;
            break;
        
        case 800: {
            text.set(-4, 128);
            textTarget.set(-4, 128-16);
            
            /*
             * This is the *average* FPS so far, as measured by the game.
             * If the cubes aren't all running at the same frame rate, this
             * will be roughly the rate of the slowest cube, due to how the
             * flow control in System::paint() works.
             */

            String<17> buf;
            float fps = frame / fpsTimespan;
            buf << (int)fps << "." << Fixed((int)(fps * 100) % 100, 2, true)
                << " FPS avg        ";
            writeText(buf);
            break;
        }
        
        case 900:
            textTarget.y = 128;
            break;
            
        case 2048:
            frame = 0;
            fpsTimespan = 0;
            break;
        }
       
        /*
         * We respond to the accelerometer
         */

        Int2 accel = vid.physicalAccel().xy();
        Float2 tilt = accel * starTiltSpeed;
        
        /*
         * Update starfield animation
         */
        
        for (unsigned i = 0; i < numChickens; i++) {
            const Float2 center = { 64 - 3.5f, 64 - 3.5f };
            vid.sprites[i].setImage(ChickenSprites, frame % ChickenSprites.numFrames());
            vid.sprites[i].move(stars[i].pos + center);
            
            stars[i].pos += float(timeStep) * (stars[i].velocity + tilt);
            
            if (stars[i].pos.x > 80 || stars[i].pos.x < -80 ||
                stars[i].pos.y > 80 || stars[i].pos.y < -80)
                initStar(i);
        }
        
        /*
         * Update global animation
         */

        frame++;
        fpsTimespan += timeStep;

        //Float2 bgVelocity = accel * bgTiltSpeed + vec(0.0f, -1.0f) * bgScrollSpeed;
        //bg += float(timeStep) * bgVelocity;
        //vid.bg0.setPanning(bg.round());

        text += (textTarget - text) * textSpeed;
        vid.bg1.setPanning(text.round());
    }

private:   
    struct
		{
        Float2 pos, velocity;
    }
		stars[numChickens];
    
    VideoBuffer vid;
    unsigned frame;
    Float2 bg, text, textTarget;
    float fpsTimespan;

    void writeText(const char *str)
    {
        // Text on BG1, in the 16x2 area we allocated
        vid.bg1.text(vec(0,14), Font, str);
    }

    void initStar(int id)
    {
        float angle = gRandom.uniform(0, 2 * M_PI);
        float speed = gRandom.uniform(starEmitSpeed * 0.5f, starEmitSpeed);    
        stars[id].pos.set(0, 0);
        stars[id].velocity.setPolar(angle, speed);
    }
};

void main()
{
    static Block blocks[gNumBlocksBuffered];

    for (unsigned i = 0; i < arraysize(blocks); i++)
		{
        blocks[i].init(i);
				blocks[i].randomize(gRandom);
		}

		int chickenBlockId = 0;
		int chickBlocks[ gMaxNumChicks ];
		Float2 chickenPos = {64.0, 0.0};
		float chickenSpeed = 50.0;
		Float2 chickPoss[ gMaxNumChicks ];
		Side exitSide = BOTTOM;

		for( unsigned i = 0; i < arraysize(chickBlocks); i++ )
		{
			chickBlocks[i] = -1;
			chickPoss[i].x = 0.0;
			chickPoss[i].y = 0.0;
		}
    
    TimeStep ts;
    while (1)
		{
			float dt = (float)ts.delta();

			// move chicken
			chickenPos += dt * chickenSpeed * sideDirection(exitSide);

			float chickenDist = dot( sideDirection(exitSide), chickenPos );
			float sideDist = dot( sideDirection(exitSide), getSidePos(exitSide) );

			// Chicken walking to next block?
			if( chickenDist > sideDist )
			{
				Block& block = blocks[chickenBlockId];

				if( block.isSideConnected(exitSide) )
				{
					// reset old one
					block.randomize(gRandom);

					// move to bottom block
					int oldChickenBlockId = chickenBlockId;
					chickenBlockId = block.getNbor(exitSide);

					// update exit side
					Block& newBlock = blocks[chickenBlockId];
					Side enterSide = newBlock.getSideOf(oldChickenBlockId);
					exitSide = oppositeSide(enterSide);
				}
				else
				{
					// game over! do nothing for now
				}

				chickenPos = centerPos(
					getSidePos(oppositeSide(exitSide)),
					ChickenSprites );
			}

			for (unsigned i = 0; i < arraysize(blocks); i++)
			{
				Block& block = blocks[i];
				// update connected states
				block.preUpdate();

				if( i == chickenBlockId )
				{
					block.showChicken( chickenPos );
				}

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
