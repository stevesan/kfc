
#ifndef _UTILS_HPP
#define _UTILS_HPP

static Side oppositeSide(Side s)
{
	switch(s)
	{
		case TOP: return BOTTOM;
		case RIGHT: return LEFT;
		case BOTTOM: return TOP;
		case LEFT: return RIGHT;
		default: return TOP;
	}
}

static Float2 sideDirection(Side s)
{
	Float2 d;
	switch(s)
	{
		case TOP: d.set(0, -1); break;
		case RIGHT: d.set(1, 0); break;
		case BOTTOM: d.set(0, 1); break;
		case LEFT: d.set(-1, 0); break;
		default: break;
	}
	return d;
}

static Float2 getSidePos(Side s)
{
	Float2 p;
	p.set(64, 64);
	p += sideDirection(s)*64;
	return p;
}

static Float2 centerPos( Float2 p, const AssetImage& img )
{
	p.x -= img.pixelWidth()/2.0;
	p.y -= img.pixelHeight()/2.0;

	return p;
}

#endif
