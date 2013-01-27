
#ifndef _ANIM_HPP
#define _ANIM_HPP

#include <sifteo.h>

static const int kMaxFrames = 8;

class Frame
{
public:
	const PinnedAssetImage* asset;
	int frameNum;

	Frame() : asset(NULL), frameNum(-1)
	{
	}

	Frame( const PinnedAssetImage* _asset, int _frameNum ) :
		asset(_asset),
		frameNum(_frameNum)
		{
		}
};

class Animation
{
public:

	Frame frames[kMaxFrames];
	float fps;

	Animation() :
		 fps(8.0),
		 nextFreeFrame(0)
	{
	}

	void addFrame( const PinnedAssetImage& img, int frameNum )
	{
		if( nextFreeFrame < arraysize(frames) )
			frames[nextFreeFrame++] = Frame(&img, frameNum);
	}

	void updateSprite( const SpriteRef& ref, float playTime ) const
	{
		const Frame& f = frames[getFrameNum(playTime)];
		ref.setImage( *f.asset, f.frameNum );
	}

	int getNumFrames() const { return nextFreeFrame; }
	float getTotalTime() const { return getNumFrames()/fps; }

	int getFrameNum( float playTime ) const
	{
		unsigned num = (int)(playTime * fps) % nextFreeFrame;
		return num;
	}

	const Frame& getFrame( float playTime ) const
	{
		//LOG("frame num = %d nframes = %d\n", getFrameNum(playTime), nextFreeFrame);
		return frames[ getFrameNum(playTime) ];
	}

private:

	int nextFreeFrame;


};

class AnimPlayer
{
public:

	const Animation* anim;
	float playTime;
	bool loop;

	AnimPlayer() : anim(NULL),
	playTime(0.0),
	loop(true)
	{
	}

	bool getIsDone()
	{
		if( loop )
			return false;
		else
		{
			return playTime >= anim->getTotalTime();
		}
	}

	void restart() { playTime = 0.0; }

	void update(float dt)
	{
		playTime += dt;

		if( !loop && playTime > anim->getTotalTime() )
		{
			playTime == anim->getTotalTime();
		}
	}

	void setAnim( const Animation* _anim, bool _loop )
	{
		playTime = 0.0;
		anim = _anim;
		loop = _loop;
	}

	void updateSprite( const SpriteRef& ref ) const
	{
		anim->updateSprite( ref, playTime );
	}

	const Frame& getCurrFrame() const
	{
		return anim->getFrame(playTime);
	}

	const PinnedAssetImage* getCurrAsset() const
	{
		return anim->frames[ anim->getFrameNum(playTime) ].asset;
	}


};

#endif
