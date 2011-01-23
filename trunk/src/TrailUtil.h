#ifndef TRAIL_UTIL_H
#define TRAIL_UTIL_H

#include "GameConstantsAndTypes.h"
#include "Difficulty.h"
#include "RageUtil_CachedObject.h"

class Song;
class Trail;
class Course;
class XNode;

namespace TrailUtil
{
	int GetNumSongs(const Trail* pTrail);
	float GetTotalSeconds(const Trail* pTrail);
};

class TrailID
{
	StepsType st;
	CourseDifficulty cd;
	mutable CachedObjectPointer<Trail> m_Cache;

public:
	TrailID() {Unset();}
	void Unset() {FromTrail(NULL);}
	void FromTrail(const Trail* p);
	Trail* ToTrail(const Course* p, bool bAllowNull) const;
	bool operator < (const TrailID& rhs) const;
	bool MatchesStepsType(StepsType s) const {return st == s;}

	XNode* CreateNode() const;
	void LoadFromNode(const XNode* pNode);
	RString ToString() const;
	bool IsValid() const;
	static void Invalidate(Song* pStaleSong);
};

#endif
