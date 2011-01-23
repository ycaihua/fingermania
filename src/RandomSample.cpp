#include "global.h"
#include "RandomSample.h"
#include "RageSound.h"
#include "RageUtil.h"
#include "RageLog.h"


RandomSample::RandomSample()
{
	m_iIndexLastPlayed = -1;
}


RandomSample::~RandomSample()
{
	UnloadAll();
}

bool RandomSample::Load( RString sFilePath, int iMaxToLoad )
{
	if( GetExtension(sFilePath) == "" )
		return LoadSoundDir( sFilePath, iMaxToLoad );
	else
		return LoadSound( sFilePath );
}

void RandomSample::UnloadAll()
{
	for( unsigned i=0; i<m_pSamples.size(); i++ )
		delete m_pSamples[i];
	m_pSamples.clear();
}

bool RandomSample::LoadSoundDir( RString sDir, int iMaxToLoad )
{
	if( sDir == "" )
		return true;

#if 0

	if(IsADirectory(sDir) && sDir[sDir.size()-1] != "/" )
		sDir += "/";
#else
	if( sDir.Right(1) != "/" )
		sDir += "/";
#endif

	vector<RString> arraySoundFiles;
	GetDirListing( sDir + "*.mp3", arraySoundFiles );
	GetDirListing( sDir + "*.ogg", arraySoundFiles );
	GetDirListing( sDir + "*.wav", arraySoundFiles );

	random_shuffle( arraySoundFiles.begin(), arraySoundFiles.end() );
	arraySoundFiles.resize( min( arraySoundFiles.size(), (unsigned)iMaxToLoad ) );

	for( unsigned i=0; i<arraySoundFiles.size(); i++ )
		LoadSound( sDir + arraySoundFiles[i] );

	return true;
}
	
bool RandomSample::LoadSound( RString sSoundFilePath )
{
	LOG->Trace( "RandomSample::LoadSound( %s )", sSoundFilePath.c_str() );

	RageSound *pSS = new RageSound;
	if( !pSS->Load(sSoundFilePath) )
	{
		LOG->Trace( "Error loading \"%s\": %s", sSoundFilePath.c_str(), pSS->GetError().c_str() );
		delete pSS;
		return false;
	}


	m_pSamples.push_back( pSS );
	
	return true;
}

int RandomSample::GetNextToPlay()
{
	if( m_pSamples.empty() )
		return -1;

	int iIndexToPlay = 0;
	for( int i=0; i<5; i++ )
	{
		iIndexToPlay = RandomInt( m_pSamples.size() );
		if( iIndexToPlay != m_iIndexLastPlayed )
			break;
	}

	m_iIndexLastPlayed = iIndexToPlay;
	return iIndexToPlay;
}

void RandomSample::PlayRandom()
{
	int iIndexToPlay = GetNextToPlay();
	if( iIndexToPlay == -1 )
		return;
	m_pSamples[iIndexToPlay]->Play();
}

void RandomSample::PlayCopyOfRandom()
{
	int iIndexToPlay = GetNextToPlay();
	if( iIndexToPlay == -1 )
		return;
	m_pSamples[iIndexToPlay]->PlayCopy();
}

void RandomSample::Stop()
{
	if( m_iIndexLastPlayed == -1 )
		return;

	m_pSamples[m_iIndexLastPlayed]->Stop();
}

