/*
___________________________________________________________________

 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Jun 2008
___________________________________________________________________

-*/


#include "vistexturechannel2rgba.h"

#include "vistexturechannels.h"
#include "color.h"
#include "coltabseqmgr.h"
#include "visosg.h"

#include <osgGeo/LayeredTexture>
#include <osg/Image>

#define mNrColors	255

mCreateFactoryEntry( visBase::ColTabTextureChannel2RGBA );

namespace visBase
{

// TODO: Port to OSG (or remove?) MappedTextureDataSetImpl

class MappedTextureDataSetImpl : public MappedTextureDataSet
{
public:

static MappedTextureDataSetImpl* create()
mCreateDataObj( MappedTextureDataSetImpl );

int nrChannels() const
{ return 0; } //{ return tc_->channels.getNum(); }


bool addChannel()
{ return true; }


bool enableNotify( bool yn )
{ return true; } //{ return tc_->channels.enableNotify( yn ); }


void setNrChannels( int nr )
{} //{ tc_->channels.setNum( nr ); }


void touch()
{} //{ tc_->touch(); }


protected:

~MappedTextureDataSetImpl()
{} //{ tc_->unref(); }

//    SoTextureChannelSet*	tc_;
};


MappedTextureDataSetImpl::MappedTextureDataSetImpl()
//    : tc_( new SoTextureChannelSet )
{} //{ tc_->ref(); }


mCreateFactoryEntry( MappedTextureDataSetImpl );


//============================================================================


TextureChannel2RGBA::TextureChannel2RGBA()
    : channels_( 0 )
    , laytex_( 0 )
{}


MappedTextureDataSet* TextureChannel2RGBA::createMappedDataSet() const
{ return MappedTextureDataSetImpl::create(); }


void TextureChannel2RGBA::setChannels( TextureChannels* ch )
{
    if ( ch!=channels_ && laytex_ )
    {
	for ( int idx=laytex_->nrProcesses()-1; idx>=0; idx-- )
	    laytex_->removeProcess( laytex_->getProcess(idx) );

    }

    channels_ = ch;
    laytex_ = channels_ ? channels_->getOsgTexture() : 0;
}


void TextureChannel2RGBA::allowShading( bool yn )
{
    if ( laytex_ ) laytex_->allowShaders( yn );
}


bool TextureChannel2RGBA::canUseShading() const
{
    return laytex_ ? laytex_->areShadersAllowed() : false;
}


bool TextureChannel2RGBA::usesShading() const
{
    return laytex_ ? laytex_->areShadersUsedNow() : false;
}


void TextureChannel2RGBA::getChannelName( int idx, uiString& res ) const
{
    res = tr("Layer %1").arg( idx+1 );
}


const osg::Image* TextureChannel2RGBA::createRGBA() const
{
     return laytex_ ? laytex_->getCompositeTextureImage() : 0;
}


int TextureChannel2RGBA::getTexturePixelSizeInBits() const
{
    if ( !createRGBA() ) return 0;
    return createRGBA()->getPixelSizeInBits();
}


const unsigned char* TextureChannel2RGBA::getTextureData() const
{
     if ( !createRGBA() ) return 0;
     return createRGBA()->data();
}


int TextureChannel2RGBA::getTextureWidth() const
{
    if ( !createRGBA() ) return 0;
    return createRGBA()->s();
}


int TextureChannel2RGBA::getTextureHeight() const
{
    if ( !createRGBA() ) return 0;
    return createRGBA()->t();
}


//============================================================================


ColTabTextureChannel2RGBA::ColTabTextureChannel2RGBA()
{}


int ColTabTextureChannel2RGBA::maxNrChannels() const
{
    return 8; /* Any number will do, because of automatic fallback to
		   non-shader solution when exceeding nrTextureUnits(). */
}


ColTabTextureChannel2RGBA::~ColTabTextureChannel2RGBA()
{
    detachAllNotifiers();
    for ( int idx=0; idx<osgcolsequences_.size(); idx++ )
	osgcolsequences_[idx]->unref();
    osgcolsequences_.erase();
    deepErase( osgcolseqarrays_ );
}


void ColTabTextureChannel2RGBA::startMonitorSeq( int ch )
{
    mAttachCB( colseqs_[ch]->objectChanged(),
		ColTabTextureChannel2RGBA::colSeqModifCB );
}


void ColTabTextureChannel2RGBA::stopMonitorSeq( int ch )
{
    mDetachCB( colseqs_[ch]->objectChanged(),
		ColTabTextureChannel2RGBA::colSeqModifCB );
}


void ColTabTextureChannel2RGBA::setChannels( TextureChannels* tc )
{
    TextureChannel2RGBA::setChannels( tc );
    adjustNrChannels();
    update();
}


void ColTabTextureChannel2RGBA::update()
{
    if ( laytex_ )
    {
	TypeSet<int> layerids;
	for ( int procidx=laytex_->nrProcesses()-1; procidx>=0; procidx-- )
	{
	    mDynamicCastGet( osgGeo::ColTabLayerProcess*, proc,
			     laytex_->getProcess(procidx) );

	    const int layerid = proc->getDataLayerID();
	    if ( laytex_->getDataLayerIndex(layerid)<0 )
	    {
		const osgGeo::ColorSequence* colseq = proc->getColorSequence();
		const int colseqidx = osgcolsequences_.indexOf( colseq );
		osgcolsequences_.removeSingle(colseqidx)->unref();
		delete osgcolseqarrays_.removeSingle( colseqidx );
		laytex_->removeProcess( proc );
	    }
	    else
		layerids.insert( 0, layerid );
	}

	for ( int channel=0; channel<channels_->nrChannels(); channel++ )
	{
	    const int layerid = (*channels_->getOsgIDs(channel))[0];
	    int procidx = layerids.indexOf(layerid);

	    mDynamicCastGet( osgGeo::ColTabLayerProcess*, proc,
			     laytex_->getProcess(procidx) );

	    if ( procidx != channel )
	    {
		if ( !proc )
		{
		    procidx = laytex_->nrProcesses();
		    proc = new osgGeo::ColTabLayerProcess( *laytex_ );
		    laytex_->addProcess( proc );
		    for ( int idx=channels_->nrDataBands()-1; idx>=0; idx-- )
			proc->setDataLayerID( idx, layerid, idx );

		    TypeSet<unsigned char>* ts = new TypeSet<unsigned char>();
		    getColors( channel, *ts );
		    osgcolseqarrays_ += ts;
		    osgGeo::ColorSequence* colseq =
					new osgGeo::ColorSequence( ts->arr() );
		    colseq->ref();
		    osgcolsequences_ += colseq;

		    proc->setColorSequence( osgcolsequences_[procidx] );

		    const Color& udfcol = getSequence(channel).undefColor();
		    proc->setNewUndefColor( Conv::to<osg::Vec4f>(udfcol) );
		}
		else
		    layerids.removeSingle( procidx );

		for ( int idx=procidx; idx>channel; idx-- )
		{
		    laytex_->moveProcessEarlier( proc );
		    osgcolsequences_.swap( idx, idx-1 );
		    osgcolseqarrays_.swap( idx, idx-1 );
		}

		layerids.insert( channel, layerid );
	    }
	    else
	    {
		for ( int idx=channels_->nrDataBands()-1; idx>=0; idx-- )
		{
		    int newid = layerid;
		    if ( idx && channels_->isCurrentDataPremapped(channel) )
			newid = -1;

		    proc->setDataLayerID( idx, newid, idx );
		}
	    }
	}
    }
}


void ColTabTextureChannel2RGBA::adjustNrChannels()
{
    const int nr = channels_ ? channels_->nrChannels() : 0;

    while ( colseqs_.size() < nr )
    {
	colseqs_ += ColTab::SeqMGR().getDefault();
	enabled_ += true;
	opacity_ += 255;
	startMonitorSeq( colseqs_.size()-1 );
    }

    while ( colseqs_.size() > nr )
    {
	const int lastidx = colseqs_.size()-1;
	stopMonitorSeq( lastidx );
	colseqs_.removeSingle( lastidx );
	enabled_.removeSingle( lastidx );
	opacity_.removeSingle( lastidx );
    }
}


void ColTabTextureChannel2RGBA::swapChannels( int ch0, int ch1 )
{
    if ( !colseqs_.validIdx(ch0) || !colseqs_.validIdx(ch1) )
	return;

    colseqs_.swap( ch0, ch1 );
    enabled_.swap( ch0, ch1 );
    opacity_.swap( ch0, ch1 );

    update();
}


void ColTabTextureChannel2RGBA::notifyChannelInsert( int ch )
{
    if ( ch<0 || ch>colseqs_.size() )
	return;

    colseqs_.insert( ch, ColTab::SeqMGR().getDefault() );
    startMonitorSeq( ch );
    enabled_.insert( ch, true );
    opacity_.insert( ch, 255 );

    update();
}


void ColTabTextureChannel2RGBA::notifyChannelRemove( int ch )
{
    if ( !colseqs_.validIdx(ch) )
	return;

    stopMonitorSeq( ch );
    colseqs_.removeSingle( ch );
    enabled_.removeSingle( ch );
    opacity_.removeSingle( ch );

    update();
}


void ColTabTextureChannel2RGBA::setSequence( int ch, const Sequence& seq )
{
    if ( !colseqs_.validIdx(ch) )
	return;

    ConstRefMan<Sequence>& curref = colseqs_[ch];
    if ( replaceMonitoredRef(curref,seq,this) )
	updFromSeq( ch );
}


void ColTabTextureChannel2RGBA::colSeqModifCB( CallBacker* cb )
{
    mGetMonitoredChgDataWithCaller( cb, chgdata, seq );
    for ( int idx=0; idx<colseqs_.size(); idx++ )
    {
	if ( colseqs_[idx].ptr() == seq )
	    updFromSeq( idx );
    }
}


void ColTabTextureChannel2RGBA::updFromSeq( int ch )
{
    if ( laytex_ )
    {
	getColors( ch, *osgcolseqarrays_[ch] );
	osgcolsequences_[ch]->touch();

	if ( laytex_->getProcess(ch) )
	{
	    const Color& udfcol = getSequence(ch).undefColor();
	    laytex_->getProcess(ch)->setNewUndefColor(
					    Conv::to<osg::Vec4f>(udfcol) );
	}
    }
}


const ColTab::Sequence& ColTabTextureChannel2RGBA::getSequence( int ch ) const
{
    if ( ch < 0 )
	ch = 0;
    if ( ch > colseqs_.size()-1 )
	ch = colseqs_.size()-1;
    return *colseqs_[ch];
}


void ColTabTextureChannel2RGBA::setEnabled( int ch, bool yn )
{
    if ( !enabled_.validIdx(ch) || enabled_[ch]==yn )
	return;

    enabled_[ch] = yn;

    if ( laytex_ )
    {
	const float opac = yn ? float(opacity_[ch])/255.0f : 0.0f;
	laytex_->getProcess(ch)->setOpacity( opac );
    }
}


bool ColTabTextureChannel2RGBA::isEnabled( int ch ) const
{
    return enabled_.validIdx(ch) ? (bool) enabled_[ch] : false;
}


void ColTabTextureChannel2RGBA::setTransparency( int ch, unsigned char t )
{
    if ( !opacity_.validIdx(ch) || opacity_[ch]==255-t )
	return;

    opacity_[ch] = 255-t;

    if ( laytex_ )
    {
	const float opac = enabled_[ch] ? float(opacity_[ch])/255.0f : 0.0f;
	laytex_->getProcess(ch)->setOpacity( opac );
    }
}


unsigned char ColTabTextureChannel2RGBA::getTransparency( int ch ) const
{
    return opacity_.validIdx(ch) ? 255-opacity_[ch] : 255;
}


void ColTabTextureChannel2RGBA::getColors( int channelidx,
					   TypeSet<unsigned char>& cols ) const
{
    if ( !colseqs_.validIdx(channelidx) )
    {
	cols.setSize( 256*4, 255 );
	cols.setAll( 255 );
	return;
    }

    const Sequence& seq = *colseqs_[channelidx];
    if ( cols.size()!=((mNrColors+1)*4) )
	cols.setSize( ((mNrColors+1)*4), 0 );

    unsigned char* arr = cols.arr();
    for ( int idx=0; idx<mNrColors; idx++ )
    {
	const float val = ((float) idx)/(mNrColors-1);
	const Color col = seq.color( val );

	(*arr++) = col.r();
	(*arr++) = col.g();
	(*arr++) = col.b();
	(*arr++) = 255-col.t();
    }

    const Color col = seq.undefColor();
    (*arr++) = col.r();
    (*arr++) = col.g();
    (*arr++) = col.b();
    (*arr++) = 255-col.t();
}


}; // namespace visBase
