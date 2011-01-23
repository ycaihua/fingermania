#include "global.h"
#include "TimingData.h"
#include "PrefsManager.h"
#include "RageUtil.h"
#include "RageLog.h"
#include "NoteTypes.h"
#include "Foreach.h"
#include <float.h>

TimingData::TimingData()
{
	m_fBeat0OffsetInSeconds = 0;
}

void TimingData::GetActualBPM( float &fMinBPMOut, float &fMaxBPMOut ) const
{
	fMinBPMOut = FLT_MAX;
	fMaxBPMOut = 0;
	FOREACH_CONST( BPMSegment, m_BPMSegments, seg )
	{
		const float fBPM = seg->GetBPM();
		fMaxBPMOut = max( fBPM, fMaxBPMOut );
		fMinBPMOut = min( fBPM, fMinBPMOut );
	}
}


void TimingData::AddBPMSegment( const BPMSegment &seg )
{
	m_BPMSegments.insert( upper_bound(m_BPMSegments.begin(), m_BPMSegments.end(), seg), seg );
}

void TimingData::AddStopSegment( const StopSegment &seg )
{
	m_StopSegments.insert( upper_bound(m_StopSegments.begin(), m_StopSegments.end(), seg), seg );
}

void TimingData::AddTimeSignatureSegment( const TimeSignatureSegment &seg )
{
	m_vTimeSignatureSegments.insert( upper_bound(m_vTimeSignatureSegments.begin(), m_vTimeSignatureSegments.end(), seg), seg );
}

void TimingData::SetBPMAtRow( int iNoteRow, float fBPM )
{
	float fBPS = fBPM / 60.0f;
	unsigned i;
	for( i=0; i<m_BPMSegments.size(); i++ )
		if( m_BPMSegments[i].m_iStartRow >= iNoteRow )
			break;

	if( i == m_BPMSegments.size() || m_BPMSegments[i].m_iStartRow != iNoteRow )
	{
		if( i == 0 || fabsf(m_BPMSegments[i-1].m_fBPS - fBPS) > 1e-5f )
			AddBPMSegment( BPMSegment(iNoteRow, fBPM) );
	}
	else
	{
		if( i > 0  &&  fabsf(m_BPMSegments[i-1].m_fBPS - fBPS) < 1e-5f )
			m_BPMSegments.erase( m_BPMSegments.begin()+i, m_BPMSegments.begin()+i+1 );
		else
			m_BPMSegments[i].m_fBPS = fBPS;
	}
}

void TimingData::SetStopAtRow( int iRow, float fSeconds )
{
	unsigned i;
	for( i=0; i<m_StopSegments.size(); i++ )
		if( m_StopSegments[i].m_iStartRow == iRow )
			break;

	if( i == m_StopSegments.size() )	
	{
		if( fSeconds > 0 )
			AddStopSegment( StopSegment(iRow, fSeconds) );
	}
	else	
	{
		if( fSeconds > 0 )
			m_StopSegments[i].m_fStopSeconds = fSeconds;
		else
			m_StopSegments.erase( m_StopSegments.begin()+i, m_StopSegments.begin()+i+1 );
	}
}

float TimingData::GetStopAtRow( int iNoteRow ) const
{
	for( unsigned i=0; i<m_StopSegments.size(); i++ )
	{
		if( m_StopSegments[i].m_iStartRow == iNoteRow )
			return m_StopSegments[i].m_fStopSeconds;
	}

	return 0;
}

void TimingData::MultiplyBPMInBeatRange( int iStartIndex, int iEndIndex, float fFactor )
{
	for( unsigned i=0; i<m_BPMSegments.size(); i++ )
	{
		const int iStartIndexThisSegment = m_BPMSegments[i].m_iStartRow;
		const bool bIsLastBPMSegment = i==m_BPMSegments.size()-1;
		const int iStartIndexNextSegment = bIsLastBPMSegment ? INT_MAX : m_BPMSegments[i+1].m_iStartRow;

		if( iStartIndexThisSegment <= iStartIndex && iStartIndexNextSegment <= iStartIndex )
			continue;

		if( iStartIndexThisSegment < iStartIndex && iStartIndexNextSegment > iStartIndex )
		{
			BPMSegment b = m_BPMSegments[i];
			b.m_iStartRow = iStartIndexNextSegment;
			m_BPMSegments.insert( m_BPMSegments.begin()+i+1, b );

			continue;
		}

		if( iStartIndexThisSegment < iEndIndex && iStartIndexNextSegment > iEndIndex )
		{
			BPMSegment b = m_BPMSegments[i];
			b.m_iStartRow = iEndIndex;
			m_BPMSegments.insert( m_BPMSegments.begin()+i+1, b );
		}
		else if( iStartIndexNextSegment > iEndIndex )
			continue;

		m_BPMSegments[i].m_fBPS = m_BPMSegments[i].m_fBPS * fFactor;
	}
}

float TimingData::GetBPMAtBeat( float fBeat ) const
{
	int iIndex = BeatToNoteRow( fBeat );
	unsigned i;
	for( i=0; i<m_BPMSegments.size()-1; i++ )
		if( m_BPMSegments[i+1].m_iStartRow > iIndex )
			break;
	return m_BPMSegments[i].GetBPM();
}

int TimingData::GetBPMSegmentIndexAtBeat( float fBeat )
{
	int iIndex = BeatToNoteRow( fBeat );
	int i;
	for( i=0; i<(int)(m_BPMSegments.size())-1; i++ )
		if( m_BPMSegments[i+1].m_iStartRow > iIndex )
			break;
	return i;
}

const TimeSignatureSegment& TimingData::GetTimeSignatureSegmentAtBeat( float fBeat ) const
{
	int iIndex = BeatToNoteRow( fBeat );
	unsigned i;
	for( i=0; i<m_vTimeSignatureSegments.size()-1; i++ )
		if( m_vTimeSignatureSegments[i+1].m_iStartRow > iIndex )
			break;
	return m_vTimeSignatureSegments[i];
}

BPMSegment& TimingData::GetBPMSegmentAtBeat( float fBeat )
{
	static BPMSegment empty;
	if( m_BPMSegments.empty() )
		return empty;
	
	int i = GetBPMSegmentIndexAtBeat( fBeat );
	return m_BPMSegments[i];
}

void TimingData::GetBeatAndBPSFromElapsedTime( float fElapsedTime, float &fBeatOut, float &fBPSOut, bool &bFreezeOut ) const
{
	fElapsedTime += PREFSMAN->m_fGlobalOffsetSeconds;

	GetBeatAndBPSFromElapsedTimeNoOffset( fElapsedTime, fBeatOut, fBPSOut, bFreezeOut );
}

void TimingData::GetBeatAndBPSFromElapsedTimeNoOffset( float fElapsedTime, float &fBeatOut, float &fBPSOut, bool &bFreezeOut ) const
{
	const float fTime = fElapsedTime;
	fElapsedTime += m_fBeat0OffsetInSeconds;


	for( unsigned i=0; i<m_BPMSegments.size(); i++ ) 
	{
		const int iStartRowThisSegment = m_BPMSegments[i].m_iStartRow;
		const float fStartBeatThisSegment = NoteRowToBeat( iStartRowThisSegment );
		const bool bIsFirstBPMSegment = i==0;
		const bool bIsLastBPMSegment = i==m_BPMSegments.size()-1;
		const int iStartRowNextSegment = bIsLastBPMSegment ? MAX_NOTE_ROW : m_BPMSegments[i+1].m_iStartRow; 
		const float fStartBeatNextSegment = NoteRowToBeat( iStartRowNextSegment );
		const float fBPS = m_BPMSegments[i].m_fBPS;

		for( unsigned j=0; j<m_StopSegments.size(); j++ )	
		{
			if( !bIsFirstBPMSegment && iStartRowThisSegment >= m_StopSegments[j].m_iStartRow )
				continue;
			if( !bIsLastBPMSegment && m_StopSegments[j].m_iStartRow > iStartRowNextSegment )
				continue;

			const int iRowsBeatsSinceStartOfSegment = m_StopSegments[j].m_iStartRow - iStartRowThisSegment;
			const float fBeatsSinceStartOfSegment = NoteRowToBeat(iRowsBeatsSinceStartOfSegment);
			const float fFreezeStartSecond = fBeatsSinceStartOfSegment / fBPS;
			
			if( fFreezeStartSecond >= fElapsedTime )
				break;

			fElapsedTime -= m_StopSegments[j].m_fStopSeconds;

			if( fFreezeStartSecond >= fElapsedTime )
			{
				fBeatOut = NoteRowToBeat(m_StopSegments[j].m_iStartRow);
				fBPSOut = fBPS;
				bFreezeOut = true;
				return;
			}
		}

		const float fBeatsInThisSegment = fStartBeatNextSegment - fStartBeatThisSegment;
		const float fSecondsInThisSegment =  fBeatsInThisSegment / fBPS;
		if( bIsLastBPMSegment || fElapsedTime <= fSecondsInThisSegment )
		{
			fBeatOut = fStartBeatThisSegment + fElapsedTime*fBPS;
			fBPSOut = fBPS;
			bFreezeOut = false;
			return;
		}

		fElapsedTime -= fSecondsInThisSegment;
	}

	vector<BPMSegment> vBPMS = m_BPMSegments;
	vector<StopSegment> vSS = m_StopSegments;
	sort( vBPMS.begin(), vBPMS.end() );
	sort( vSS.begin(), vSS.end() );
	ASSERT_M( vBPMS == m_BPMSegments, "The BPM segments were not sorted!" );
	ASSERT_M( vSS == m_StopSegments, "The Stop segments were not sorted!" );
	FAIL_M( ssprintf("Failed to find the appropriate segment for elapsed time %f.", fTime) );
}

float TimingData::GetElapsedTimeFromBeat( float fBeat ) const
{
	return TimingData::GetElapsedTimeFromBeatNoOffset( fBeat ) - PREFSMAN->m_fGlobalOffsetSeconds;
}

float TimingData::GetElapsedTimeFromBeatNoOffset(float fBeat) const
{
	float fElapsedTime = 0;
	fElapsedTime -= m_fBeatOffsetInSeconds;

	int iRow = BeatToNoteRow(fBeat);
	for (unsigned j = 0; j < m_StopSegments.size(); j++)
	{
		if (m_StopSegments[j].m_iStartRow >= iRow)
			break;
		fElapsedTime += m_StopSegments[j].m_fStopSeconds;
	}

	for (unsigned i = 0; i < m_BPMSegments.size(); i++)
	{
		const bool bIsLastBPMSegment = i == m_BPMSegments.size() - 1;
		const float fBPS = m_BPMSegments[i].m_fBPS;

		if (bIsLastBPMSegment)
		{
			fElapsedTime += NoteRowToBeat(iRow) / fBPS;
		}
		else
		{
			const int iStartIndexThisSegment = m_BPMSegments[i].m_iStartRow;
			const int iStartIndexNextSegment = m_BPMSegments[i + 1].m_iStartRow;
			const int iRowsInThisSegment = min(iStartIndexNextSegment - iStartIndexThisSegment, iRow);
			fElapsedTime += NoteRowToBeat(iRowsInThisSegment) / fBPS;
			iRow -= iRowsInThisSegment;
		}

		if (iRow <= 0)
			return fElapsedTime;
	}

	return fElapsedTime;
}

void TimingData::ScaleRegion(float fScale, int iStartIndex, int iEndIndex)
{
	ASSERT(fScale > 0);
	ASSERT(iStartIndex > 0);
	ASSERT(iStartIndex < iEndIndex);

	for (unsigned i = 0; i < m_BPMSegments.size(); i++)
	{
		const int iSegStart = m_BPMSegments[i].m_iStartRow;
		if (iSegStart < iStartIndex)
			continue;
		else if (iSegStart > iEndIndex)
			m_BPMSegments[i].m_iStartRow += lrintf((iEndIndex - iStartIndex) * (fScale - 1));
		else
			m_BPMSegments[i].m_iStartRow = lrintf((iSegStart - iStartIndex) * fScale) + iStartIndex;
	}

	for (unsigned i = 0; i < m_StopSegments.size(); i++)
	{
		const int iSegStartRow = m_StopSegments[i].m_iStartRow;
		if (iSegStartRow < iStartIndex)
			continue;
		else if (iSegStartRow > iEndIndex)
			m_StopSegments[i].m_iStartRow += lrintf((iEndIndex - iStartIndex) * (fScale - 1));
		else
			m_StopSegments[i].m_iStartRow = lrintf((iSegStartRow - iStartIndex) * fScale) + iStartIndex;
	}
}

void TimingData::InsertRows(int iStartRow, int iRowsToAdd)
{
	for (unsigned i = 0; i < m_BPMSegments.size(); i++)
	{
		BPMSegment& bpm = m_BPMSegments[i];
		if (bpm.m_iStartRow < iStartRow)
			continue;
		bpm.m_iStartRow += iRowsToAdd;
	}

	for( unsigned i = 0; i < m_StopSegments.size(); i++ )
	{
		StopSegment &stop = m_StopSegments[i];
		if( stop.m_iStartRow < iStartRow )
			continue;
		stop.m_iStartRow += iRowsToAdd;
	}

	if( iStartRow == 0 )
	{
		ASSERT( m_BPMSegments.size() > 0 );
		m_BPMSegments[0].m_iStartRow = 0;
	}
}

void TimingData::DeleteRows( int iStartRow, int iRowsToDelete )
{
	float fNewBPM = this->GetBPMAtBeat( NoteRowToBeat(iStartRow+iRowsToDelete) );

	for( unsigned i = 0; i < m_BPMSegments.size(); i++ )
	{
		BPMSegment &bpm = m_BPMSegments[i];

		if( bpm.m_iStartRow < iStartRow )
			continue;

		if( bpm.m_iStartRow < iStartRow+iRowsToDelete )
		{
			m_BPMSegments.erase( m_BPMSegments.begin()+i, m_BPMSegments.begin()+i+1 );
			--i;
			continue;
		}

		bpm.m_iStartRow -= iRowsToDelete;
	}

	for( unsigned i = 0; i < m_StopSegments.size(); i++ )
	{
		StopSegment &stop = m_StopSegments[i];

		if( stop.m_iStartRow < iStartRow )
			continue;

		if( stop.m_iStartRow < iStartRow+iRowsToDelete )
		{
			m_StopSegments.erase( m_StopSegments.begin()+i, m_StopSegments.begin()+i+1 );
			--i;
			continue;
		}

		stop.m_iStartRow -= iRowsToDelete;
	}

	this->SetBPMAtRow( iStartRow, fNewBPM );
}

bool TimingData::HasBpmChanges() const
{
	return m_BPMSegments.size()>1;
}

bool TimingData::HasStops() const
{
	return m_StopSegments.size()>0;
}

bool TimingData::HasTimeSignatures() const
{
	return m_vTimeSignatureSegments.size()>0;
}

void TimingData::NoteRowToMeasureAndBeat(int iNoteRow, int& iMeasureIndexOut, int& iBeatIndexOut, int& iRowsRemainder) const
{
	iMeasureIndexOut = 0;

	FOREACH_CONST(TimeSignatureSegment, m_vTimeSignatureSegments, iter)
	{
		vector<TimeSignatureSegment>::const_iterator next = iter;
		next++;
		int iSegmentEndRow = (next == m_vTimeSignatureSegments.end()) ? INT_MAX : next->m_iStartRow;

		int iRowsPerMeasureThisSegment = iter->GetNoteRowsPerMeasure();

		if (iNoteRow >= iter->m_iStartRow)
		{
			int iNumRowsThisSegment = iNoteRow - iter->m_iStartRow;
			int iNumMeasuresThisSegment = (iNumRowsThisSegment) / iRowsPerMeasureThisSegment;
			iMeasureIndexOut += iNumMeasuresThisSegment;
			iBeatIndexOut = iNumRowsThisSegment / iRowsPerMeasureThisSegment;
			iRowsRemainder = iNumRowsThisSegment % iRowsPerMeasureThisSegment;
			return;
		}
		else
		{
			int iNumRowsThisSegment = iSegmentEndRow - iter->m_iStartRow;
			int iNumMeasuresThisSegment = (iNumRowsThisSegment + iRowsPerMeasureThisSegment - 1) / iRowsPerMeasureThisSegment;
			iMeasureIndexOut += iNumMeasuresThisSegment;
		}
	}

	ASSERT(0);
	return;
}

#include "LuaBinding.h"

class LunaTimingData : public Luna<TimingData>
{
public:
	static int HasStops(T* p, lua_State* L)
	{
		lua_pushboolean(L, p->HasStops());
		return;
	}

	static int GetElapsedTimeFromBeat(T* p, lua_State* L)
	{
		lua_pushnumber(L, p->GetElapsedTimeFromBeat(FArg(1)));
		return 1;
	}

	LunaTimingData()
	{
		ADD_METHOD(HasStops);
		ADD_METHOD(GetElapsedTimeFromBeat);
	}
};

LUA_REGISTER_CLASS(TimingData)

