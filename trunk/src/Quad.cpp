#include "global.h"
#include "Quad.h"
#include "ActorUtil.h"
#include "RageTextureManager.h"

REGISTER_ACTOR_CLASS(Quad)

Quad::Quad()
{
	Load(TEXTUREMAN->GetDefaultTextureID());
}

void Quad::LoadFromNode(const XNode* pNode)
{
	Actor::LoadFromNode(pNode);
}
