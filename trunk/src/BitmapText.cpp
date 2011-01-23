#include "global.h"
#include "BitmapText.h"
#include "XmlFile.h"
#include "FontManager.h"
#include "RageLog.h"
#include "RageTimer.h"
#include "RageDisplay.h"
#include "ThemeManager.h"
#include "Font.h"
#include "ActorUtil.h"
#include "LuaBinding.h"
#include "Foreach.h"

REGISTER_ACTOR_CLASS( BitmapText )

#define NUM_RAINBOW_COLORS	THEME->GetMetricI("BitmapText","NumRainbowColors")
#define RAINBOW_COLOR(n)	THEME->GetMetricC("BitmapText",ssprintf("RainbowColor%i", n+1))

static vector<RageColor> RAINBOW_COLORS;

BitmapText::BitmapText()
{
	// Loading these theme metrics is slow, so only do it ever 20th time.
	static int iReloadCounter = 0;
	if( iReloadCounter%20==0 )
	{
		RAINBOW_COLORS.resize( NUM_RAINBOW_COLORS );
		for( unsigned i = 0; i < RAINBOW_COLORS.size(); ++i )
			RAINBOW_COLORS[i] = RAINBOW_COLOR(i);
	}
	iReloadCounter++;

	m_pFont = NULL;
	m_bUppercase = false;

	m_bRainbowScroll = false;
	m_bJitter = false;

	m_iWrapWidthPixels = -1;
	m_fMaxWidth = 0;
	m_fMaxHeight = 0;
	m_iVertSpacing = 0;
	m_bHasGlowAttribute = false;

	m_StrokeColor = RageColor(1,1,1,1);

	SetShadowLength( 4 );
}

BitmapText::~BitmapText()
{
	if( m_pFont )
		FONT->UnloadFont( m_pFont );
}

BitmapText & BitmapText::operator=(const BitmapText &cpy)
{
	Actor::operator=(cpy);

#define CPY(a) a = cpy.a
	CPY( m_bUppercase );
	CPY( m_sText );
	CPY( m_wTextLines );
	CPY( m_iLineWidths );
	CPY( m_iWrapWidthPixels );
	CPY( m_fMaxWidth );
	CPY( m_fMaxHeight );
	CPY( m_bRainbowScroll );
	CPY( m_bJitter );
	CPY( m_iVertSpacing );
	CPY( m_aVertices );
	CPY( m_vpFontPageTextures );
	CPY( m_mAttributes );
	CPY( m_bHasGlowAttribute );
	CPY( m_StrokeColor );
#undef CPY

	if( m_pFont )
		FONT->UnloadFont( m_pFont );
	
	if( cpy.m_pFont != NULL )
		m_pFont = FONT->CopyFont( cpy.m_pFont );
	else
		m_pFont = NULL;

	return *this;
}

BitmapText::BitmapText( const BitmapText &cpy ):
	Actor( cpy )
{
	m_pFont = NULL;

	*this = cpy;
}

void BitmapText::LoadFromNode( const XNode* pNode )
{
	RString sText;
	pNode->GetAttrValue( "Text", sText );
	RString sAltText;
	pNode->GetAttrValue( "AltText", sAltText );

	ThemeManager::EvaluateString( sText );
	ThemeManager::EvaluateString( sAltText );

	RString sFont;
	if( !ActorUtil::GetAttrPath(pNode, "File", sFont) )
	{
		if( !pNode->GetAttrValue("Font", sFont) )
			RageException::Throw( "%s: BitmapText: missing the File attribute",
					      ActorUtil::GetWhere(pNode).c_str() );
		sFont = THEME->GetPathF( "", sFont );
	}

	LoadFromFont( sFont );

	SetText( sText, sAltText );
	Actor::LoadFromNode( pNode );
}

bool BitmapText::LoadFromFont( const RString& sFontFilePath )
{
	CHECKPOINT_M( ssprintf("BitmapText::LoadFromFont(%s)", sFontFilePath.c_str()) );

	if( m_pFont )
	{
		FONT->UnloadFont( m_pFont );
		m_pFont = NULL;
	}

	m_pFont = FONT->LoadFont( sFontFilePath );

	this->SetStrokeColor( m_pFont->GetDefaultStrokeColor() );

	BuildChars();

	return true;
}


bool BitmapText::LoadFromTextureAndChars( const RString& sTexturePath, const RString& sChars )
{
	CHECKPOINT_M( ssprintf("BitmapText::LoadFromTextureAndChars(\"%s\",\"%s\")", sTexturePath.c_str(), sChars.c_str()) );

	if( m_pFont )
	{
		FONT->UnloadFont( m_pFont );
		m_pFont = NULL;
	}

	m_pFont = FONT->LoadFont( sTexturePath, sChars );

	BuildChars();

	return true;
}

void BitmapText::BuildChars()
{
	if( m_pFont == NULL )
		return;

	m_size.x = 0;

	m_iLineWidths.clear();
	for( unsigned l=0; l<m_wTextLines.size(); l++ )	// for each line
	{
		m_iLineWidths.push_back(m_pFont->GetLineWidthInSourcePixels( m_wTextLines[l] ));
		m_size.x = max( m_size.x, m_iLineWidths.back() );
	}

	m_size.x = QuantizeUp( m_size.x, 2.0f );

	m_aVertices.clear();
	m_vpFontPageTextures.clear();
	
	if( m_wTextLines.empty() )
		return;

	m_size.y = float(m_pFont->GetHeight() * m_wTextLines.size());

	int iPadding = m_pFont->GetLineSpacing() - m_pFont->GetHeight();
	iPadding += m_iVertSpacing;

	m_size.y += iPadding * int(m_wTextLines.size()-1);

	int iY = lrintf(-m_size.y/2.0f);

	for( unsigned i=0; i<m_wTextLines.size(); i++ )
	{
		iY += m_pFont->GetHeight();

		wstring sLine = m_wTextLines[i];
		if( m_pFont->IsRightToLeft() )
			reverse( sLine.begin(), sLine.end() );
		const int iLineWidth = m_iLineWidths[i];

		float fX = SCALE( m_fHorizAlign, 0.0f, 1.0f, -m_size.x/2.0f, +m_size.x/2.0f - iLineWidth );
		int iX = lrintf( fX );

		for (unsigned j = 0; j < sLine.size(); ++j)
		{
			RageSpriteVertex v[4];
			const glyph& g = m_pFont->GetGlyph(sLine[j]);

			if (m_pFont->IsRightToLeft())
				iX -= g.m_iHadvance;

			v[0].p = RageVector3(iX + g.m_fHshift, iY + g.m_pPage->m_fVshift, 0);
			v[1].p = RageVector3(iX + g.m_fHshift, iY + g.m_pPage->m_fVshift + g.m_fHeight, 0);
			v[2].p = RageVector3(iX + g.m_fHshift + g.m_fWidth, iY + g.m_pPage->m_fVshift + g.m_fHeight, 0);
			v[3].p = RageVector3(iX + g.m_fHshift + g.m_fWidth, iY + g.m_pPage->m_fVshift, 0);

			iX += g.m_iHadvance;

			v[0].t = RageVector2(g.m_TexRect.left, g.m_TexRect.top);
			v[1].t = RageVector2(g.m_TexRect.left, g.m_TexRect.bottom);
			v[2].t = RageVector2(g.m_TexRect.right, g.m_TexRect.bottom);
			v[3].t = RageVector2(g.m_TexRect.right, g.m_TexRect.top);

			m_aVertices.insert(m_aVertices.end(), &v[0], &v[4]);
			m_vpFontPageTextures.push_back(g.GetFontPageTextures());
		}

		iY += iPadding;
	}
}

void BitmapText::DrawChars(bool bUseStrokeTexture)
{
	if (m_pTempState->crop.left + m_pTempState->crop.right >= 1 ||
		m_pTempState->crop.top + m_pTempState->crop.bottom >= 1)
		return;

	const int iNumGlyphs = m_vpFontPageTextures.size();
	int iStartGlyph = lrintf(SCALE(m_pTempState->crop.left, 0.f, 1.f, 0, (float)iNumGlyphs));
	int iEndGlyph = lrintf(SCALE(m_pTempState->crop.right, 0.f, 1.f, (float)iNumGlyphs, 0));
	iStartGlyph = clamp(iStartGlyph, 0, iNumGlyphs);
	iEndGlyph = clamp(iEndGlyph, 0, iNumGlyphs);

	if (m_pTempState->fade.top > 0 ||
		m_pTempState->fade.bottom > 0 ||
		m_pTempState->fade.left > 0 ||
		m_pTempState->fade.right > 0)
	{
		const RectF& FadeDist = m_pTempState->fade;
		RectF FadeSize = FadeDist;

		const float fHorizRemaining = 1.0f - (m_pTempState->crop.left + m_pTempState->crop.right);
		if (FadeDist.left + FadeDist.right > 0 &&
			fHorizRemaining < FadeDist.left + FadeDist.right)
		{
			const float LeftPercent = FadeDist.left / (FadeDist.left + FadeDist.right);
			FadeSize.left = LeftPercent * fHorizRemaining;
			FadeSize.right = (1.0f - LeftPercent) * fHorizRemaining;
		}

		const float fRightAlpha = SCALE(FadeSize.right, FadeDist.right, 0, 1, 0);
		const float fLeftAlpha = SCALE(FadeSize.left, FadeDist.left, 0, 1, 0);

		const float fStartFadeLeftPercent = m_pTempState->crop.left;
		const float fStopFadeLeftPercent = m_pTempState->crop.left + FadeSize.left;
		const float fLeftFadeStartGlyph = SCALE(fStartFadeLeftPercent, 0.f, 1.f, 0, (float)iNumGlyphs);
		const float fRightFadeStopGlyph = SCALE( fStopFadeRightPercent, 0.f, 1.f, 0, (float) iNumGlyphs );

		for (int start = iStartGlyph; start < iEndGlyph; ++start)
		{
			int i = start * 4;

			float fAlpha = 1.0f;
			if (FadeSize.left > 0.001f)
			{
				float fPercent = SCALE( start+0.5f, fLeftFadeStartGlyph, fLeftFadeStopGlyph, 0.0f, 1.0f );
				fPercent = clamp(fPercent, 0.0f, 1.0f);
				fAlpha *= fPercent * fLeftAlpha;
			}

			if (FadeSize.right > 0.001f)
			{
				float fPercent = SCALE(start + 0.5f, fRightFadeStartGlyph, fRightFadeStopGlyph, 1.0f, 0.0f);
				fPercent = clamp(fPercent, 0.0f, 1.0f);
				fAlpha = fPercent * fRightAlpha;
			}

			for (int j = 0; j < 4; ++j)
				m_aVertices[i + j].c.a = (unsigned char)(m_aVertices[i + j].c.a * fAlpha);
		}
	}

	for (int start = iStartGlyph; start < iEndGlyph; )
	{
		int end = start;
		while (end < iEndGlyph && m_vpFontPageTextures[end] == m_vpFontPageTextures[start])
			end++;

		bool bHaveATexture = !UseStrokeTexture || (bUseStrokeTexture && m_vpFontPageTextures[start]->m_pTextureStroke);
		if (bHaveATexture)
		{
			DISPLAY->ClearAllTextures();
			if (bUseStrokeTexture)
				DISPLAY->SetTexture(TextureUnit_1, m_vpFontPageTextures[start]->m_pTextureStroke->GetTexHandle());
			else
				DISPLAY->SetTexture(TextureUnit_1, m_vpFontPageTextures[start]->m_pTextureMain->GetTexHandle());

			Actor::SetTextureRenderStates();

			RageSpriteVertex& start_vertex = m_aVertices[start * 4];
			int iNumVertsToDraw = (end - start) * 4;
			DISPLAY->DrawQuads(&start_vertex, iNumVertsToDraw);
		}

		start = end;
	}
}

void BitmapText::SetText(const RString& _sText, const RString& _sAlternateText, int iWrapWidthPixels)
{
	ASSERT(m_pFont);

	RString sNewText = StringWillUseAlternate(_sText, _sAlternateText) ? _sAlternateText : _sText;

	if (m_bUppercase)
		sNewText.MakeUpper();

	if (iWrapWidthPixels == -1)
		iWrapWidthPixels = m_iWrapWidthPixels;

	if (m_sText == sNewText && iWrapWidthPixels == m_iWrapWidthPixels)
		return;

	m_sText = sNewText;
	m_iWrapWidthPixels = iWrapWidthPixels;
	ClearAttributes();
	SetTextInternal();
}

void BitmapText::SetTextInternal()
{
	m_wTextLines.clear();

	if (m_iWrapWidthPixels == -1)
	{
		split(RStringToWstring(m_sText), L"\n", m_wTextLines, false);
	}
	else
	{
		vector<RString> asLines;
		split(m_sText, "\n", asLines, false);

		for (unsigned line = 0; line < asLines.size(); ++line)
		{
			vector<RString> asWords;
			split(asLines[line], " ", asWords);

			RString sCurLine;
			int iCurLineWidth = 0;

			for (unsigned i = 0; i < asWords.size(); i++)
			{
				const RString &sWord = asWords[i];
				int iWidthWord = m_pFont->GetLineWidthInSourcePixels( RStringToWstring(sWord) );

				if( sCurLine.empty() )
				{
					sCurLine = sWord;
					iCurLineWidth = iWidthWord;
					continue;
				}

				RString sToAdd = " " + sWord;
				int iWidthToAdd = m_pFont->GetLineWidthInSourcePixels(L" ") + iWidthWord;
				if( iCurLineWidth + iWidthToAdd <= m_iWrapWidthPixels )	// will fit on current line
				{
					sCurLine += sToAdd;
					iCurLineWidth += iWidthToAdd;
				}
				else
				{
					m_wTextLines.push_back( RStringToWstring(sCurLine) );
					sCurLine = sWord;
					iCurLineWidth = iWidthWord;
				}
			}
			m_wTextLines.push_back( RStringToWstring(sCurLine) );
		}
	}

	BuildChars();
	UpdateBaseZoom();
}

void BitmapText::SetVertSpacing( int iSpacing )
{
	m_iVertSpacing = iSpacing;
	BuildChars();
}

void BitmapText::SetMaxWidth( float fMaxWidth )
{
	m_fMaxWidth = fMaxWidth;
	UpdateBaseZoom();
}

void BitmapText::SetMaxHeight( float fMaxHeight )
{
	m_fMaxHeight = fMaxHeight;
	UpdateBaseZoom();
}

void BitmapText::SetUppercase( bool b )
{
	m_bUppercase = b;
	BuildChars();
}

void BitmapText::UpdateBaseZoom()
{
	if( m_fMaxWidth == 0 )
	{
		this->SetBaseZoomX( 1 );
	}
	else
	{
		const float fWidth = GetUnzoomedWidth();
		if( fWidth != 0 )     
		{
			const float fZoom = min( 1, m_fMaxWidth/fWidth );
			this->SetBaseZoomX( fZoom );
		}
	}

	if( m_fMaxHeight == 0 )
	{
		this->SetBaseZoomY( 1 );
	}
	else
	{
		const float fHeight = GetUnzoomedHeight();
		if( fHeight != 0 )    
		{
			const float fZoom = min( 1, m_fMaxHeight/fHeight );
			this->SetBaseZoomY( fZoom );
		}
	}
}

bool BitmapText::StringWillUseAlternate( const RString& sText, const RString& sAlternateText ) const
{
	ASSERT( m_pFont );

	if( !sAlternateText.size() )
		return false;

	if( m_pFont->FontCompleteForString(RStringToWstring(sText)) )
		return false;

	if( !m_pFont->FontCompleteForString(RStringToWstring(sAlternateText)) )
		return false;

	return true;
}

void BitmapText::CropToWidth( int iMaxWidthInSourcePixels )
{
	iMaxWidthInSourcePixels = max( 0, iMaxWidthInSourcePixels );

	for( unsigned l=0; l<m_wTextLines.size(); l++ )	// for each line
	{
		while( m_iLineWidths[l] > iMaxWidthInSourcePixels )
		{
			m_wTextLines[l].erase( m_wTextLines[l].end()-1, m_wTextLines[l].end() );
			m_iLineWidths[l] = m_pFont->GetLineWidthInSourcePixels( m_wTextLines[l] );
		}
	}

	BuildChars();
}

bool BitmapText::EarlyAbortDraw() const
{
	return m_wTextLines.empty();
}

void BitmapText::DrawPrimitives()
{
	
	Actor::SetGlobalRenderStates();
	DISPLAY->SetTextureMode( TextureUnit_1, TextureMode_Modulate );

	if( m_pTempState->diffuse[0].a != 0 )
	{
		if( m_fShadowLengthX != 0  ||  m_fShadowLengthY != 0 )
		{
			DISPLAY->PushMatrix();
			DISPLAY->TranslateWorld( m_fShadowLengthX, m_fShadowLengthY, 0 );

			RageColor c = m_ShadowColor;
			c.a *= m_pTempState->diffuse[0].a;
			for( unsigned i=0; i<m_aVertices.size(); i++ )
				m_aVertices[i].c = c;
			DrawChars( false );

			DISPLAY->PopMatrix();
		}

		if( m_StrokeColor.a > 0 )
		{
			RageColor c = m_StrokeColor;
			c.a *= m_pTempState->diffuse[0].a;
			for( unsigned i=0; i<m_aVertices.size(); i++ )
				m_aVertices[i].c = c;
			DrawChars( true );
		}

		if( m_bRainbowScroll )
		{
			int color_index = int(RageTimer::GetTimeSinceStartFast() / 0.200) % RAINBOW_COLORS.size();
			for( unsigned i=0; i<m_aVertices.size(); i+=4 )
			{
				const RageColor color = RAINBOW_COLORS[color_index];
				for( unsigned j=i; j<i+4; j++ )
					m_aVertices[j].c = color;

				color_index = (color_index+1) % RAINBOW_COLORS.size();
			}
		}
		else
		{
			size_t i = 0;
			map<size_t,Attribute>::const_iterator iter = m_mAttributes.begin();
			while( i < m_aVertices.size() )
			{
				size_t iEnd = iter == m_mAttributes.end()? m_aVertices.size():iter->first*4;
				iEnd = min( iEnd, m_aVertices.size() );
				for( ; i < iEnd; i += 4 )
				{
					m_aVertices[i+0].c = m_pTempState->diffuse[0];
					m_aVertices[i+1].c = m_pTempState->diffuse[2];	
					m_aVertices[i+2].c = m_pTempState->diffuse[3];	
					m_aVertices[i+3].c = m_pTempState->diffuse[1];
				}
				if( iter == m_mAttributes.end() )
					break;
		
				const Attribute &attr = iter->second;
				++iter;
				if( attr.length < 0 )
					iEnd = iter == m_mAttributes.end()? m_aVertices.size():iter->first*4;
				else
					iEnd = i + attr.length*4;
				iEnd = min( iEnd, m_aVertices.size() );
				for( ; i < iEnd; i += 4 )
				{
					m_aVertices[i+0].c = attr.diffuse[0];
					m_aVertices[i+1].c = attr.diffuse[2];	
					m_aVertices[i+2].c = attr.diffuse[3];	
					m_aVertices[i+3].c = attr.diffuse[1];
				}
			}
		}

		vector<RageVector3> vGlyphJitter;
		if( m_bJitter )
		{
			int iSeed = lrintf( RageTimer::GetTimeSinceStartFast()*8 );
			RandomGen rnd( iSeed );

			for( unsigned i=0; i<m_aVertices.size(); i+=4 )
			{
				RageVector3 jitter( rnd()%2, rnd()%3, 0 );
				vGlyphJitter.push_back( jitter );

				m_aVertices[i+0].p += jitter;	// top left
				m_aVertices[i+1].p += jitter;	// bottom left
				m_aVertices[i+2].p += jitter;	// bottom right
				m_aVertices[i+3].p += jitter;	// top right
			}
		}

		DrawChars( false );

		if( m_bJitter )
		{
			ASSERT( vGlyphJitter.size() == m_aVertices.size()/4 );
			for( unsigned i=0; i<m_aVertices.size(); i+=4 )
			{
				const RageVector3 &jitter = vGlyphJitter[i/4];;

				m_aVertices[i+0].p -= jitter;	
				m_aVertices[i+1].p -= jitter;	
				m_aVertices[i+2].p -= jitter;
				m_aVertices[i+3].p -= jitter;	
			}
		}
	}

	if (m_pTemplate->glow.a > 0.0001f || m_bHasGlowAttribute)
	{
		DISPLAY->SetTextureMode(TextureUnit_1, TextureMode_Glow);

		size_t i = 0;
		map<size_t, Attribute>::const_iterator iter = m_mAttributes.begin();
		while (i < m_aVertices.size())
		{
			size_t iEnd = iter == m_mAttributes.end() ? m_aVertices.size() : iter->first * 4;
			iEnd = min(iEnd, m_aVertices.size());
			for (; i < iEnd; ++i)
				m_aVertices[i].c = m_pTemplate->glow;
			if (iter == m_mAttributes.end())
				break;
			const Attribute& attr = iter->second;
			++iter;
			if (arr.length < 0)
				iEnd = iter == m_mAttributes.end() ? m_aVertices.size() : iter->first * 4;
			else
				iEnd = i + arr.length * 4;
			iEnd = min(iEnd, m_aVertices.size());
			for (; i < iEnd; ++i)
				m_aVertices[i].c = attr.glow;
		}

		DrawChars(false);
		DrawChars(true);
	}
}

void BitmapText::SetHorizAlign(float f)
{
	float fHorizAlign = m_fHorizAlign;
	Actor::SetHorizAlign(f);
	if (fHorizAlign == m_fHorizAlign)
		return;
	BuildChars();
}

void BitmapText::SetWrapWidthPixels(int iWrapWidthPixels)
{
	ASSERT(m_pFont);
	if (m_iWrapWidthPixels == iWrapWidthPixels)
		return;
	m_iWrapWidthPixels = iWrapWidthPixels;
	SetTextInternal();
}

BitmapText::Attribute BitmapText::GetDefaultAttribute() const
{
	Attribute attr;
	for( int i = 0; i < 4; ++i )
		attr.diffuse[i] = GetDiffuses( i );
	attr.glow = GetGlow();
	return attr;
}

void BitmapText::AddAttribute( size_t iPos, const Attribute &attr )
{
	int iLines = 0;
	size_t iAdjustedPos = iPos;
	
	FOREACH_CONST( wstring, m_wTextLines, line )
	{
		size_t length = line->length();
		if( length >= iAdjustedPos )
			break;
		iAdjustedPos -= length;
		++iLines;
	}
	m_mAttributes[iPos-iLines] = attr;
	m_bHasGlowAttribute = m_bHasGlowAttribute || attr.glow.a > 0.0001f;
}

void BitmapText::ClearAttributes()
{
	m_mAttributes.clear();
	m_bHasGlowAttribute = false;
}

void BitmapText::Attribute::FromStack( lua_State *L, int iPos )
{
	if( lua_type(L, iPos) != LUA_TTABLE )
		return;
	
	lua_pushvalue( L, iPos );
	const int iTab = lua_gettop( L );

	lua_getfield( L, iTab, "Length" );
	length = lua_tointeger( L, -1 );
	lua_settop( L, iTab );
	
	lua_getfield( L, iTab, "Diffuses" );
	if( !lua_isnil(L, -1) )
	{
		for( int i = 1; i <= 4; ++i )
		{
			lua_rawgeti( L, -i, i );
			diffuse[i-1].FromStack( L, -1 );
		}
	}
	lua_settop( L, iTab );
	
	lua_getfield( L, iTab, "Diffuse" );
	if( !lua_isnil(L, -1) )
	{
		diffuse[0].FromStack( L, -1 );
		diffuse[1] = diffuse[2] = diffuse[3] = diffuse[0];
	}
	lua_settop( L, iTab );
	
	lua_getfield( L, iTab, "Glow" );
	glow.FromStack( L, -1 );
	
	lua_settop( L, iTab - 1 );
}

#include "FontCharAliases.h"
class LunaBitmapText: public Luna<BitmapText>
{
	public:
	static int wrapwidthpixels( T* p, lua_State *L )	{ p->SetWrapWidthPixels( IArg(1) ); return 0; }
	static int maxwidth( T* p, lua_State *L )		{ p->SetMaxWidth( FArg(1) ); return 0; }
	static int maxheight( T* p, lua_State *L )		{ p->SetMaxHeight( FArg(1) ); return 0; }
	static int vertspacing( T* p, lua_State *L )		{ p->SetVertSpacing( IArg(1) ); return 0; }
	static int settext( T* p, lua_State *L )
	{
		RString s = SArg(1);
		RString sAlt;

		s.Replace("::","\n");
		FontCharAliases::ReplaceMarkers( s );

		if( lua_gettop(L) > 1 )
		{
			sAlt = SArg(2);
			sAlt.Replace("::","\n");
			FontCharAliases::ReplaceMarkers( sAlt );
		}

		p->SetText( s, sAlt );
		return 0;
	}

	static int jitter( T* p, lua_State *L )			{ p->SetJitter( BArg(1) ); return 0; }
	static int GetText( T* p, lua_State *L )		{ lua_pushstring( L, p->GetText() ); return 1; }
	static int AddAttribute( T* p, lua_State *L )
	{
		size_t iPos = IArg(1);
		BitmapText::Attribute attr = p->GetDefaultAttribute();
		
		attr.FromStack( L, 2 );
		p->AddAttribute( iPos, attr );
		return 0;
	}

	static int ClearAttributes( T* p, lua_State *L )	{ p->ClearAttributes(); return 0; }
	static int strokecolor( T* p, lua_State *L )		{ RageColor c; c.FromStackCompat( L, 1 ); p->SetStrokeColor( c ); return 0; }
	static int uppercase( T* p, lua_State *L )		{ p->SetUppercase( BArg(1) ); return 0; }

	LunaBitmapText()
	{
		ADD_METHOD( wrapwidthpixels );
		ADD_METHOD( maxwidth );
		ADD_METHOD( maxheight );
		ADD_METHOD( vertspacing );
		ADD_METHOD( settext );
		ADD_METHOD( rainbowscroll );
		ADD_METHOD( jitter );
		ADD_METHOD( GetText );
		ADD_METHOD( AddAttribute );
		ADD_METHOD( ClearAttributes );
		ADD_METHOD( strokecolor );
		ADD_METHOD( uppercase );
	}
};

LUA_REGISTER_DERIVED_CLASS( BitmapText, Actor )

