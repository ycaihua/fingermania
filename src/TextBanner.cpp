#include "global.h"
#include "TextBanner.h"
#include "Song.h"
#include "ActorUtil.h"
#include "ThemeManager.h"
#include "XmlFile.h"

REGISTER_ACTOR_CLASS( TextBanner )

void TextBanner::LoadFromNode( const XNode* pNode )
{
	m_bInitted = true;

	ActorFrame::LoadFromNode( pNode );
}

void TextBanner::Load( RString sMetricsGroup )
{
	m_bInitted = true;

	m_textTitle.SetName( "Title" );
	m_textTitle.LoadFromFont( THEME->GetPathF(sMetricsGroup,"text") );
	this->AddChild( &m_textTitle );

	m_textSubTitle.SetName( "Subtitle" );
	m_textSubTitle.LoadFromFont( THEME->GetPathF(sMetricsGroup,"text") );
	this->AddChild( &m_textSubTitle );

	m_textArtist.SetName( "Artist" );
	m_textArtist.LoadFromFont( THEME->GetPathF(sMetricsGroup,"text") );
	this->AddChild( &m_textArtist );

	AddCommand( "AfterSet", THEME->GetMetricA(sMetricsGroup,"AfterSetCommand") );
	m_sArtistPrependString = THEME->GetMetric(sMetricsGroup,"ArtistPrependString");

	ActorUtil::LoadAllCommandsAndOnCommand( m_textTitle, sMetricsGroup );
	ActorUtil::LoadAllCommandsAndOnCommand( m_textSubTitle, sMetricsGroup );
	ActorUtil::LoadAllCommandsAndOnCommand( m_textArtist, sMetricsGroup );
}

TextBanner::TextBanner()
{
	m_bInitted = false;
}

TextBanner::TextBanner( const TextBanner &cpy ):
	ActorFrame( cpy ),
	m_bInitted( cpy.m_bInitted ),
	m_textTitle( cpy.m_textTitle ),
	m_textSubTitle( cpy.m_textSubTitle ),
	m_textArtist( cpy.m_textArtist ),
	m_sArtistPrependString( cpy.m_sArtistPrependString )
{
	this->AddChild( &m_textTitle );
	this->AddChild( &m_textSubTitle );
	this->AddChild( &m_textArtist );
}

void TextBanner::SetFromString( 
	const RString &sDisplayTitle, const RString &sTranslitTitle, 
	const RString &sDisplaySubTitle, const RString &sTranslitSubTitle, 
	const RString &sDisplayArtist, const RString &sTranslitArtist )
{
	ASSERT( m_bInitted );

	m_textTitle.SetText( sDisplayTitle, sTranslitTitle );
	m_textSubTitle.SetText( sDisplaySubTitle, sTranslitSubTitle );
	m_textArtist.SetText( sDisplayArtist, sTranslitArtist );

	Message msg("AfterSet");
	this->PlayCommandNoRecurse( msg );
}

void TextBanner::SetFromSong( const Song *pSong )
{
	if( pSong == NULL )
	{
		SetFromString( "", "", "", "", "", "" );
		return;
	}
	SetFromString( pSong->GetDisplayMainTitle(),				pSong->GetTranslitMainTitle(),
			pSong->GetDisplaySubTitle(),				pSong->GetTranslitSubTitle(),
			m_sArtistPrependString + pSong->GetDisplayArtist(),	m_sArtistPrependString + pSong->GetTranslitArtist() );
}

#include "LuaBinding.h"

class LunaTextBanner: public Luna<TextBanner>
{
public:
	static int Load( T* p, lua_State *L ) { p->Load( SArg(1) ); return 0; }
	static int SetFromSong( T* p, lua_State *L )
	{
		Song *pSong = Luna<Song>::check(L,1);
  		p->SetFromSong( pSong );
		return 0;
	}
	static int SetFromString( T* p, lua_State *L )
	{
		RString sDisplayTitle = SArg(1);
		RString sTranslitTitle = SArg(2);
		RString sDisplaySubTitle = SArg(3);
		RString sTranslitSubTitle = SArg(4);
		RString sDisplayArtist = SArg(5);
		RString sTranslitArtist = SArg(6);
  		p->SetFromString( sDisplayTitle, sTranslitTitle, sDisplaySubTitle, sTranslitSubTitle, sDisplayArtist, sTranslitArtist );
		return 0;
	}

	LunaTextBanner()
	{
		ADD_METHOD( Load );
		ADD_METHOD( SetFromSong );
		ADD_METHOD( SetFromString );
	}
};

LUA_REGISTER_DERIVED_CLASS( TextBanner, ActorFrame )
