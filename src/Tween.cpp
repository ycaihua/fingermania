#include "global.h"
#include "Tween.h"
#include "RageUtil.h"
#include "RageMath.h"
#include "LuaManager.h"
#include "EnumHelper.h"

static const char *TweenTypeNames[] = {
	"Linear",
	"Accelerate",
	"Decelerate",
	"Spring",
	"Bezier"
};
XToString( TweenType );
LuaXType( TweenType );


struct TweenLinear: public ITween
{
	float Tween( float f ) const { return f; }
	ITween *Copy() const { return new TweenLinear(*this); }
};
struct TweenAccelerate: public ITween
{
	float Tween( float f ) const { return f*f; }
	ITween *Copy() const { return new TweenAccelerate(*this); }
};
struct TweenDecelerate: public ITween
{
	float Tween( float f ) const { return 1 - (1-f) * (1-f); }
	ITween *Copy() const { return new TweenDecelerate(*this); }
};
struct TweenSpring: public ITween
{
	float Tween( float f ) const { return 1 - RageFastCos( f*PI*2.5f )/(1+f*3); }
	ITween *Copy() const { return new TweenSpring(*this); }
};

struct InterpolateBezier1D: public ITween
{
	float Tween( float f ) const;
	ITween *Copy() const { return new InterpolateBezier1D(*this); }

	RageQuadratic m_Bezier;
};

float InterpolateBezier1D::Tween( float f ) const
{
	return m_Bezier.Evaluate( f );
}

struct InterpolateBezier2D: public ITween
{
	float Tween(float f) const;
	ITween* Copy() const
	{
		return new InterpolateBezier2D(*this);
	}

	RageBezier2D m_Bezier;
};

float InterpolateBezier2D:Tween(float f) const
{
	return m_Bezier.EvaluateYFromX(f);
}

ITween* ITween::CreateFromType(TweenType tt)
{
	switch(tt)
	{
	case TWEEN_LINEAR:
		return new TweenLinear;
	case TWEEN_ACCELERATE:
		return new TweenAccelerate;
	case TWEEN_DECELERATE:
		return new TweenDecelerate;
	case TWEEN_SPRING:
		return new TweenSpring;
	default:
		ASSERT(0);
	}
}

ITween* ITween::CreateFromStack(Lua* L, int iStackPos)
{
	TweenType iType = Enum::Check<TweenType>(L, iStackPos);
	if (iType == TWEEN_BEZIER)
	{
		luaL_checkType(L, iStackPos + 1, LUA_TTABLE);
		int iArgs = lua_objlen(L, iStackPos + 1);
		if (iArgs != 4 && iArgs != 8)
			RageException::Throw("CreateFromStack: table argument must have 4 or 8 entries");

		float fC[8];
		for (int i = 0; i < iArgs; ++i)
		{
			lua_rawgeti(L, iStackPos + 1, i + 1);
			fC[i] = (float)lua_tonumber(L, -1);
		}

		lua_pop(L, iArgs);
		if (iArgs == 4)
		{
			InterpolateBezier1D* pBezier = new InterpolateBezier1D;
			pBezier->m_Bezier.SetFromBezier(fC[0], fC[1], fC[2], fC[3]);
			return pBezier;
		}
		else if (iArgs == 8)
		{
			InterpolateBezier2D* pBezier = new InterpolateBezier2D;
			pBezier->m_pBezier.SetFromBezier(fC[0], fC[1], fC[2], fC[3], fC[4], fC[5], fC[6], fC[7]);
			return pBezier;
		}
	}

	return CreateFromType(iType);
}
