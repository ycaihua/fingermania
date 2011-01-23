#include "global.h"
#include <cassert>

#include "Sprite.h"
#include "RageTextureManager.h"
#include "XmlFile.h"
#include "RageLog.h"
#include "RageDisplay.h"
#include "RageTexture.h"
#include "RageUtil.h"
#include "ActorUtil.h"
#include "arch/Dialog/Dialog.h"
#include "Foreach.h"
#include "LuaBinding.h"
#include "LuaManager.h"

REGISTER_ACTOR_CLASS( Sprite )


Sprite::Sprite()
{
	m_pTexture = NULL;
	m_iCurState = 0;
	m_fSecsIntoState = 0.0f;
	m_bUsingCustomTexCoords = false;
	m_bSkipNextUpdate = true;
	m_EffectMode = EffectMode_Normal;
	
	m_fRememberedClipWidth = -1;
	m_fRememberedClipHeight = -1;

	m_fTexCoordVelocityX = 0;
	m_fTexCoordVelocityY = 0;
}

Sprite::~Sprite()
{
	UnloadTexture();
}

Sprite::Sprite( const Sprite &cpy ):
	Actor( cpy )
{
#define CPY(a) a = cpy.a
	CPY( m_States );
	CPY( m_iCurState );
	CPY( m_fSecsIntoState );
	CPY( m_bUsingCustomTexCoords );
	CPY( m_bSkipNextUpdate );
	CPY( m_EffectMode );
	memcpy( m_CustomTexCoords, cpy.m_CustomTexCoords, sizeof(m_CustomTexCoords) );
	CPY( m_fRememberedClipWidth );
	CPY( m_fRememberedClipHeight );
	CPY( m_fTexCoordVelocityX );
	CPY( m_fTexCoordVelocityY );
#undef CPY

	if( cpy.m_pTexture != NULL )
		m_pTexture = TEXTUREMAN->CopyTexture( cpy.m_pTexture );
	else
		m_pTexture = NULL;
}

void Sprite::InitState()
{
	Actor::InitState();
	m_iCurState = 0;
	m_fSecsIntoState = 0.0f;
	m_bSkipNextUpdate = true;
}


RageTextureID Sprite::SongBGTexture( RageTextureID ID )
{
	ID.bMipMaps = true;

	ID.iAlphaBits = 0;

	ID.Policy = RageTextureID::TEX_VOLATILE;

	ID.bDither = true;

	return ID;
}

RageTextureID Sprite::SongBannerTexture( RageTextureID ID )
{
	ID.bHotPinkColorKey = true;
	ID.iColorDepth = 32;
	ID.Policy = RageTextureID::TEX_VOLATILE;

	return ID;
}

void Sprite::Load( RageTextureID ID )
{
	if( !ID.filename.empty() ) 
		LoadFromTexture( ID );

	LoadStatesFromTexture();
};

void Sprite::LoadFromNode( const XNode* pNode )
{
	RString sPath;
	pNode->GetAttrValue( "Texture", sPath );
	if( !sPath.empty() && !TEXTUREMAN->IsTextureRegistered( RageTextureID(sPath) ) )
		ActorUtil::GetAttrPath( pNode, "Texture", sPath );

	if( !sPath.empty() )
	{
		LoadFromTexture( sPath );
		
		LoadStatesFromTexture();

		vector<State> aStates;

		const XNode *pFrames = pNode->GetChild( "Frames" );
		if( pFrames != NULL )
		{
			int iFrameIndex = 0;
			for( int i=0; true; i++ )
			{
				const XNode *pFrame = pFrames->GetChild( ssprintf("%i", i+1) ); // +1 for Lua's arrays
				if( pFrame == NULL )
					break;

				State newState;
				if( !pFrame->GetAttrValue("Delay", newState.fDelay) )
					newState.fDelay = 0.1f;

				pFrame->GetAttrValue( "Frame", iFrameIndex );
				if( iFrameIndex >= m_pTexture->GetNumFrames() )
					RageException::Throw( "%s: State #%i is frame %d, but the texture \"%s\" only has %d frames",
						ActorUtil::GetWhere(pNode).c_str(), i, iFrameIndex, sPath.c_str(), m_pTexture->GetNumFrames() );
				newState.rect = *m_pTexture->GetTextureCoordRect( iFrameIndex );

				const XNode *pPoints[2] = { pFrame->GetChild( "1" ), pFrame->GetChild( "2" ) };
				if( pPoints[0] != NULL && pPoints[1] != NULL )
				{
					RectF r = newState.rect;

					float fX = 1.0f, fY = 1.0f;
					pPoints[0]->GetAttrValue( "x", fX );
					pPoints[0]->GetAttrValue( "y", fY );
					newState.rect.left = SCALE( fX, 0.0f, 1.0f, r.left, r.right );
					newState.rect.top = SCALE( fY, 0.0f, 1.0f, r.top, r.bottom );

					pPoints[1]->GetAttrValue( "x", fX );
					pPoints[1]->GetAttrValue( "y", fY );
					newState.rect.right = SCALE( fX, 0.0f, 1.0f, r.left, r.right );
					newState.rect.bottom = SCALE( fY, 0.0f, 1.0f, r.top, r.bottom );
				}

				aStates.push_back( newState );
			}
		}
		else for( int i=0; true; i++ )
		{
			RString sFrameKey = ssprintf( "Frame%04d", i );
			RString sDelayKey = ssprintf( "Delay%04d", i );
			State newState;

			int iFrameIndex;
			if( !pNode->GetAttrValue(sFrameKey, iFrameIndex) )
				break;
			if( iFrameIndex >= m_pTexture->GetNumFrames() )
				RageException::Throw( "%s: %s is %d, but the texture \"%s\" only has %d frames",
					ActorUtil::GetWhere(pNode).c_str(), sFrameKey.c_str(), iFrameIndex, sPath.c_str(), m_pTexture->GetNumFrames() );

			newState.rect = *m_pTexture->GetTextureCoordRect(iFrameIndex);
			if (!pNode->GetAttrValue(sDelayKey, newState.fDelay))
				break;
			aStates.push_back(newState);
		}

		if (!aStates.empty())
		{
			m_States = aStates;
			Sprite::m_size.x = aStates[0].rect.GetWidth() / m_pTexture->GetSourceToTexCoordsRatioX();
			Sprite::m_size.y = aStates[0].rect.GetHeight() / m_pTexture->GetSourceToTexCoordsRatioY();
		}
	}

	Actor::LoadFromNode(pNode);
}

void Sprite::UnloadTexture()
{
	if (m_pTexture != NULL)
	{
		TEXTUREMAN->UnloadTexture(m_pTexture);
		m_pTexture = NULL;

		SetState(0);
	}
}

void Sprite::EnableAnimation(bool bEnable)
{
	bool bWasEnabled = m_bIsAnimating;
	Actor::EnableAnimation(bEnable);

	if (bEnable && !bWasEnabled)
	{
		m_bSkipNextUpdate = true;
	}
}

void Sprite::SetTexture(RageTexture* pTexture)
{
	ASSERT(pTexture != NULL);

	if (m_pTexture != pTexture)
	{
		UnloadTexture();
		m_pTexture = pTexture;
	}

	ASSERT(m_pTexture->GetTextureWidth() >= 0);
	ASSERT(m_pTexture->GetTextureHeight() >= 0);

	if( m_fRememberedClipWidth != -1 && m_fRememberedClipHeight != -1 )
		ScaleToClipped( m_fRememberedClipWidth, m_fRememberedClipHeight );

	if( m_States.empty() )
		LoadStatesFromTexture();
}

void Sprite::LoadFromTexture( RageTextureID ID )
{
	RageTexture *pTexture = NULL;
	if( m_pTexture && m_pTexture->GetID() == ID )
		pTexture = m_pTexture;
	else
		pTexture = TEXTUREMAN->LoadTexture( ID );

	SetTexture( pTexture );
}

void Sprite::LoadStatesFromTexture()
{
	m_States.clear();

	if( m_pTexture == NULL )
	{
		State newState;
		newState.fDelay = 0.1f;
		newState.rect = RectF( 0, 0, 1, 1 );
		m_States.push_back( newState );
		return;		
	}
	
	for( int i=0; i<m_pTexture->GetNumFrames(); ++i )
	{
		State newState;
		newState.fDelay = 0.1f;
		newState.rect = *m_pTexture->GetTextureCoordRect( i );
		m_States.push_back( newState );
	}
}

void Sprite::UpdateAnimationState()
{

	if( m_States.size() > 1 )
	{
		while( m_fSecsIntoState+0.0001f > m_States[m_iCurState].fDelay )	
		{
			m_fSecsIntoState -= m_States[m_iCurState].fDelay;
			m_iCurState = (m_iCurState+1) % m_States.size();
		}
	}
}

void Sprite::Update( float fDelta )
{
	Actor::Update( fDelta );

	const bool bSkipThisMovieUpdate = m_bSkipNextUpdate;
	m_bSkipNextUpdate = false;

	if( !m_bIsAnimating )
		return;

	if( !m_pTexture )	
	    return;

	float fTimePassed = GetEffectDeltaTime();
	m_fSecsIntoState += fTimePassed;

	if( m_fSecsIntoState < 0 )
		wrap( m_fSecsIntoState, GetAnimationLengthSeconds() );

	UpdateAnimationState();

	if( !bSkipThisMovieUpdate )
		m_pTexture->DecodeSeconds( max(0, fTimePassed) );

	if( m_fTexCoordVelocityX != 0 || m_fTexCoordVelocityY != 0 )
	{
		float fTexCoords[8];
		Sprite::GetActiveTextureCoords( fTexCoords );
 
		fTexCoords[0] += fDelta*m_fTexCoordVelocityX;
		fTexCoords[1] += fDelta*m_fTexCoordVelocityY; 
		fTexCoords[2] += fDelta*m_fTexCoordVelocityX;
		fTexCoords[3] += fDelta*m_fTexCoordVelocityY;
		fTexCoords[4] += fDelta*m_fTexCoordVelocityX;
		fTexCoords[5] += fDelta*m_fTexCoordVelocityY;
		fTexCoords[6] += fDelta*m_fTexCoordVelocityX;
		fTexCoords[7] += fDelta*m_fTexCoordVelocityY;

		if( m_bTextureWrapping )
		{
			const float fXAdjust = floorf( fTexCoords[0] );
			const float fYAdjust = floorf( fTexCoords[1] );
			fTexCoords[0] -= fXAdjust;
			fTexCoords[2] -= fXAdjust;
			fTexCoords[4] -= fXAdjust;
			fTexCoords[6] -= fXAdjust;
			fTexCoords[1] -= fYAdjust;
			fTexCoords[3] -= fYAdjust;
			fTexCoords[5] -= fYAdjust;
			fTexCoords[7] -= fYAdjust;
		}

		Sprite::SetCustomTextureCoords( fTexCoords );
	}
}

void TexCoordArrayFromRect( float fImageCoords[8], const RectF &rect )
{
	fImageCoords[0] = rect.left;	fImageCoords[1] = rect.top;	
	fImageCoords[2] = rect.left;	fImageCoords[3] = rect.bottom;
	fImageCoords[4] = rect.right;	fImageCoords[5] = rect.bottom;
	fImageCoords[6] = rect.right;	fImageCoords[7] = rect.top;	
}


