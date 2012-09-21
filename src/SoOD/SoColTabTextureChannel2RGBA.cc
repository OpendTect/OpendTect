/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          September 2008
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id$";


#include "SoColTabTextureChannel2RGBA.h"

#include "Inventor/actions/SoGLRenderAction.h"
#include "Inventor/fields/SoSFImage.h"
#include "Inventor/sensors/SoFieldSensor.h"
#include "SoTextureChannelSetElement.h"
#include "SoTextureComposerElement.h"

SO_NODE_SOURCE( SoColTabTextureChannel2RGBA );

void SoColTabTextureChannel2RGBA::initClass()
{
    SO_NODE_INIT_CLASS(SoColTabTextureChannel2RGBA, SoNode, "Node");

    SO_ENABLE(SoGLRenderAction, SoTextureChannelSetElement );
    SO_ENABLE(SoGLRenderAction, SoTextureComposerElement );
}


SoColTabTextureChannel2RGBA::SoColTabTextureChannel2RGBA()
    : needsregeneration_( true )
    , matchinfo_( 0 )
{
    SO_NODE_CONSTRUCTOR( SoColTabTextureChannel2RGBA );
    SO_NODE_ADD_FIELD( colorsequences, (SbImagei32(0, SbVec2i32(0,0), 0)) );
    SO_NODE_ADD_FIELD( opacity, (255) );
    SO_NODE_ADD_FIELD( enabled, (true) );

    sequencesensor_ = new SoFieldSensor( fieldChangeCB, this );
    sequencesensor_->attach( &colorsequences );

    opacitysensor_ = new SoFieldSensor( fieldChangeCB, this );
    opacitysensor_->attach( &opacity );

    enabledsensor_ = new SoFieldSensor( fieldChangeCB, this );
    enabledsensor_->attach( &enabled );
}


SoColTabTextureChannel2RGBA::~SoColTabTextureChannel2RGBA()
{
    delete sequencesensor_;
    delete opacitysensor_;
    delete enabledsensor_;

    delete matchinfo_;
}


void SoColTabTextureChannel2RGBA::fieldChangeCB( void* data, SoSensor* )
{
    SoColTabTextureChannel2RGBA* ptr = (SoColTabTextureChannel2RGBA*) data;
    ptr->needsregeneration_ = true;
}


void SoColTabTextureChannel2RGBA::GLRender( SoGLRenderAction* action )
{
    SoState* state = action->getState();
    const SoTextureChannelSetElement* elem = (const SoTextureChannelSetElement*)
       state->getConstElement(SoTextureChannelSetElement::getClassStackIndex());

    if ( !needsregeneration_ && matchinfo_ && elem &&
	 !elem->matches( matchinfo_ ) )
    {
	needsregeneration_ = true;
    }

    SbList<uint32_t> dep;
    dep.append( elem->getNodeId() );
    if ( !needsregeneration_ )
    {
	sendRGBA( state, dep );
	return;
    }
    else
    {
	delete matchinfo_; matchinfo_ = elem->copyMatchInfo();
    }

    int nrchannels = SoTextureChannelSetElement::getNrChannels( state );
    if ( nrchannels>colorsequences.getNum() )
	nrchannels = colorsequences.getNum();

    if ( nrchannels>enabled.getNum() )
	nrchannels = enabled.getNum();

    if ( !nrchannels )
    {
	for ( int idx=0; idx<4; idx++ )
	    rgba_[idx].setValue( SbVec3i32(1,1,1), 1, 0 );
	sendRGBA( state, dep );
	return;
    }

    const SbImagei32* channels = SoTextureChannelSetElement::getChannels( 
	    state );

    processChannels( channels, nrchannels );
    needsregeneration_ = false;


    sendRGBA( state, dep );

}


const SbImagei32& SoColTabTextureChannel2RGBA::getRGBA(int rgba) const
{
    return rgba_[rgba];
}


void SoColTabTextureChannel2RGBA::processChannels( const SbImagei32* channels,
	int nrchannels )
{
    if ( !nrchannels ) return;

    //All channels should have the same size or be empty.
    SbVec3i32 size3;
    int size = 0;
    for ( int idx=0; idx<nrchannels; idx++ )
    {
	const SbVec3i32 chsize3 = channels[idx].getSize();
	const int chsize = chsize3[0]*chsize3[1]*chsize3[2];
	if ( !size )
	{
	    size3 = chsize3;
	    size = chsize;
	}
	else if ( chsize && size3!=chsize3 )
	    return;
    }

    int lastchannel = -1;
    int firstchannel = -1;

    char fullyopaque = 0;
    //-1 = false, -2 = all non-opaque samples are fully transparent 1 = true;

    char fullytransparent = 0;
    //-1 = false, 1 = true

    for ( int channel=nrchannels-1; channel>=0; channel-- )
    {
	fullyopaque = 0; fullytransparent = 0;
	if ( !enabled[channel] )
	    continue;

	if ( opacity[channel]<255 )
	{
	    fullyopaque = -1;
	    if ( opacity[channel]<=0 )
	    {
		fullytransparent = 1;
		fullyopaque = -2;
	    }
	}

	if ( (fullyopaque!=-2 && fullyopaque!=1) || !fullytransparent )
	{
	    getTransparencyStatus( channels, size, channel, fullyopaque,
				   fullytransparent );
	}

	if ( lastchannel==-1 && fullytransparent==-1 )
	    lastchannel = channel;

	if ( fullytransparent==-1 )
	    firstchannel = channel;

	if ( fullyopaque==1 )
	    break;
    }

    if ( lastchannel==-1 || firstchannel==-1 )
    {
	for ( int idx=0; idx<4; idx++ )
	    rgba_[idx].setValue( SbVec3i32(1,1,1), 1, 0 );
	return;
    }

    if ( fullyopaque==1 )
	ti_ = SoTextureComposerInfo::cHasNoTransparency();
    else if ( fullyopaque==-1 )
	ti_ = SoTextureComposerInfo::cHasTransparency();
    else
	ti_ = SoTextureComposerInfo::cHasNoIntermediateTransparency();

    //Set size of outputs
    for ( int idx=0; idx<4; idx++ )
	rgba_[idx].setValue( size3, 1, 0 );

    computeRGBA( channels, 0, size-1, firstchannel, lastchannel );
}


void SoColTabTextureChannel2RGBA::computeRGBA( const SbImagei32* channels,
	int start, int stop, int firstchannel, int lastchannel )
{
    SbVec3i32 size; int bytesperpixel;

    unsigned char* red = rgba_[0].getValue( size, bytesperpixel ) + start;
    unsigned char* green = rgba_[1].getValue( size, bytesperpixel )+start;
    unsigned char* blue = rgba_[2].getValue( size, bytesperpixel )+ start;
    unsigned char* alpha = rgba_[3].getValue( size, bytesperpixel )+start;

    for ( int idx=start; idx<=stop; idx++ )
    {
	bool inited = false;
	for ( int channelidx=firstchannel;channelidx<=lastchannel;channelidx++ )
	{
	    if ( !enabled[channelidx] )
		continue;

	    int layeropacity = opacity[channelidx];
	    if ( layeropacity<=0 )
		continue;

	    if ( layeropacity>255 )
		layeropacity=255;

	    const unsigned char* channel =
		channels[channelidx].getValue( size, bytesperpixel );
	    if ( !channel || bytesperpixel!=1 )
		continue;

	    const unsigned int coltabindex = channel[idx];
	    const unsigned char* color =
		colorsequences[channelidx].getValue( size, bytesperpixel ) +
		coltabindex * bytesperpixel;

	    const unsigned char curopacity = (int) (color[3]*layeropacity)/255;
	    const unsigned char trans = 255-curopacity;
	    if ( inited && !curopacity )
		continue;

	    if ( !inited )
	    {
		*red = color[0];
		*green = color[1];
		*blue = color[2];
		*alpha = curopacity;
		inited = true;
	    }
	    else
	    {
		*red = (int)((int) *red * trans + (int)color[0]*curopacity)/255;
		*green = (int)((int) *green*trans + (int)color[1]*curopacity)/255;
		*blue = (int)((int) *blue * trans + (int)color[2]*curopacity)/255;
		if ( color[3]>*alpha )
		    *alpha = color[3];
	    }
	}

	red++;
	green++;
	blue++;
	alpha++;
    }
}


void SoColTabTextureChannel2RGBA::sendRGBA( SoState* state,
					    const SbList<uint32_t>& dep )
{
    SoTextureChannelSetElement::set( state, this, rgba_, 4, &dep );
    SbList<int> units;
    units.append( 0 );
    SoTextureComposerElement::set( state, this, units, ti_ );
}


void SoColTabTextureChannel2RGBA::getTransparencyStatus(
	const SbImagei32* channels, int size, int channelidx,
	char& fullopacity, char& fulltransparency ) const
{
    SbVec3i32 seqsize; int seqbytesperpixel;
    unsigned const char* channel =
	channels[channelidx].getValue( seqsize, seqbytesperpixel );

    if ( !channel )
    {
	if ( !fullopacity )
	    fullopacity = -1;

	if ( !fulltransparency )
	    fulltransparency = 1;

	return;
    }

    const unsigned char* colseq =
	colorsequences[channelidx].getValue( seqsize, seqbytesperpixel );
    if ( seqbytesperpixel<4 )
    {
	if ( !fullopacity )
	    fullopacity = 1;
	if ( !fulltransparency )
	    fulltransparency = -1;
	return;
    }

    for ( int idx=0; idx<size; idx++ )
    {
	const unsigned int coltabindex = channel[idx];  
	const unsigned opac = colseq[coltabindex*seqbytesperpixel+3];

	if ( opac!=255 )
	{
	    if ( !opac )
	    {
		if ( !fullopacity  )
		    fullopacity = -2;
	    }
	    else 
		fullopacity = -1;
	}
	    
	if ( !fulltransparency && opac )
	    fulltransparency = -1;

	if ( (fullopacity==1 || fullopacity==-1) && fulltransparency )
	    return;
    }

    if ( !fulltransparency )
	fulltransparency = 1;

    if ( !fullopacity )
	fullopacity = 1;
}
