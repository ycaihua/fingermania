#ifndef MUSIC_LIST_H
#define MUSIC_LIST_H

#include "ActorFrame.h"
#include "BitmapText.h"

const int MAX_MLIST_COLUMNS = 5;

class Song;

class MusicList : public ActorFrame
{
	BitmapText m_textTitles[MAX_MLIST_COLUMNS];

	struct group
	{
		RString ContentsText[MAX_MLIST_COLUMNS];
		int m_iNumSongsInGroup;
	};

	vector<group> m_ContentsText;

	int NumGroups, CurGroup;

public:
	MusicList();
	void Load();

	void AddGroup();

	void AddSongsToGroup(const vector<Song*>& songs);

	void SetGroupNo(int group);

	void TweenOnScreen();
	void TweenOffScreen();
	int GetNumSongs() const
	{
		return m_ContentsText[CurGroup].m_iNumSongsInGroup;
	}
};

#endif
