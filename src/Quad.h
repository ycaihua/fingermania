#ifndef QUAD_H
#define QUAD_H

#include "Sprite.h"


class Quad : public Sprite
{
public:
	Quad();
	void LoadFromNode( const XNode* pNode );
	virtual Quad *Copy() const;
};

#endif
