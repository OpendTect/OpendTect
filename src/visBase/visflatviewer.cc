/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Yuancheng Liu
 Date:		5-11-2007
________________________________________________________________________

-*/
static const char* rcsID = "$Id: visflatviewer.cc,v 1.29 2009-08-18 18:12:46 cvsyuancheng Exp $";

#include "visflatviewer.h"

#include "arraynd.h"
#include "coltabmapper.h"
#include "coltabsequence.h"
#include "dataclipper.h"
#include "flatposdata.h"
#include "linear.h"
#include "survinfo.h"
#include "viscoord.h"
#include "vismaterial.h"
#include "vispolyline.h"
#include "vistexturechannels.h"
#include "vissplittexture2rectangle.h"
#include "vistexturechannel2rgba.h"

mCreateFactoryEntry( visBase::FlatViewer );

namespace visBase
{
   
FlatViewer::FlatViewer()
    : VisualObjectImpl( false )
    , dataChange( this )
    , channels_( TextureChannels::create() )
    , channel2rgba_( ColTabTextureChannel2RGBA::create() )
    , rectangle_( SplitTexture2Rectangle::create() )
    , x1gridlines_( visBase::IndexedPolyLine::create() )
    , x2gridlines_( visBase::IndexedPolyLine::create() )
{
    channel2rgba_->ref();
    channel2rgba_->allowShading( true );

    channels_->ref();
    addChild( channels_->getInventorNode() );
    channels_->setChannels2RGBA( channel2rgba_ );

    if ( channels_->nrChannels()<1 )
    {
    	channels_->addChannel();
    	channel2rgba_->setEnabled( 0, true );	
    }

    rectangle_->ref();
    rectangle_->setMaterial( 0 );
    rectangle_->removeSwitch();
    addChild( rectangle_->getInventorNode() );

    x1gridlines_->ref();
    x1gridlines_->setMaterial( visBase::Material::create() );
    x1gridlines_->getMaterial()->setColor( Color(0,0,0,0) );
    addChild( x1gridlines_->getInventorNode() );

    x2gridlines_->ref();
    x2gridlines_->setMaterial( x1gridlines_->getMaterial() );
    addChild( x2gridlines_->getInventorNode() );
}


FlatViewer::~FlatViewer()
{
    channels_->unRef();
    channel2rgba_->unRef();
    rectangle_->unRef();
    x2gridlines_->unRef();
    x1gridlines_->unRef();
}


void FlatViewer::handleChange( FlatView::Viewer::DataChangeType dt, bool dofill)
{
    switch ( dt )
    {
	case None:	
		break;
	case WVAData:	
	case WVAPars:	
		pErrMsg( "Not supported" );
		break;
	case All:	
	case Annot:
	    {	
		updateGridLines( true );
		updateGridLines( false );

		if ( dt!=All )
		    break;
	    }
	case VDData:
	    {
		const FlatDataPack* dp = pack( false );
		if ( !dp )
		    channels_->turnOn( false );
		else
		{
		    const Array2D<float>& dparr = dp->data();
		    channels_->setSize( 1, dparr.info().getSize(0),
					   dparr.info().getSize(1) );
		    if ( !dparr.getData() )
		    {
			const int bufsz = dparr.info().getTotalSz();
			mDeclareAndTryAlloc(float*,ptr, float[bufsz]);
			if ( !ptr )
			    channels_->turnOn( false );
			else
			{
			    dparr.getAll( ptr );
			    channels_->setUnMappedData( 0, 0, ptr,
							OD::TakeOverPtr, 0 );
			}
		    }
		    else 
		    {
			channels_->setUnMappedData( 0, 0, dparr.getData(),
						    OD::UsePtr, 0 );
		    }

		    appearance().ddpars_.vd_.ctab_ =
			channel2rgba_->getSequence(0)->name();
		    rectangle_->setOriginalTextureSize( 
				dparr.info().getSize(0),
				dparr.info().getSize(1) );
		    channels_->turnOn( appearance().ddpars_.vd_.show_ );

		    dataChange.trigger();
		    if ( dt!=All )
			break;
		}
	    }
	case VDPars : 	
	    {
	    	const FlatView::DataDispPars::VD& vd = appearance().ddpars_.vd_;
	    	ColTab::MapperSetup mappersetup;
		vd.fill( mappersetup );
		if ( channels_->getColTabMapperSetup( 0,0 )!=mappersetup )
		{
		    channels_->setColTabMapperSetup( 0, mappersetup );
		    channels_->reMapData( 0, 0 );
		}

		ColTab::Sequence sequence = *channel2rgba_->getSequence( 0 );
		if ( vd.ctab_!=sequence.name() )
		{
		    if ( ColTab::SM().get( vd.ctab_, sequence ) )
		    {
			channel2rgba_->setSequence( 0, sequence );
		    }
		}
	    }
    }			
}


void FlatViewer::setPosition( const Coord3& c00, const Coord3& c01, 
			      const Coord3& c10, const Coord3& c11 )
{
    rectangle_->setPosition( c00, c01, c10, c11 );

    c00_ = c00;
    c01_ = c01;
    c10_ = c10;
    c11_ = c11;
    
    updateGridLines( true );
    updateGridLines( false );
}    


const SamplingData<float> FlatViewer::getDefaultGridSampling( bool x1 ) const
{
    const FlatPosData* posdata = pack(false) ? &pack(false)->posData() : 0;
    if ( !posdata )
	return SamplingData<float>( 0, 1 );

    Interval<float> range; range.setFrom( posdata->range( x1 ) );
    AxisLayout layout( range );
    return layout.sd;
}


void FlatViewer::turnOnGridLines( bool offsetlines, bool zlines )
{
    x1gridlines_->turnOn( offsetlines );
    x2gridlines_->turnOn( zlines );
}


void FlatViewer::updateGridLines( bool x1 )
{
    if ( channel2rgba_->getSequence(0) )
    {
    	const Color markcolor = channel2rgba_->getSequence(0)->markColor();
    	x1gridlines_->getMaterial()->setColor( markcolor );
    	x2gridlines_->getMaterial()->setColor( markcolor );
    }

    const FlatPosData* posdata = pack(false) ? &pack(false)->posData() : 0;
    visBase::IndexedPolyLine* gridlines = x1 ? x1gridlines_ : x2gridlines_;

    if ( !posdata || (x1 && !appearance().annot_.x1_.showgridlines_ ) ||
	 (!x1 && !appearance().annot_.x2_.showgridlines_ ) )
    {
	gridlines->turnOn( false );
	return;
    }

    Interval<float> range; range.setFrom( posdata->range( x1 ) );
    SamplingData<float> sd = x1 ? appearance().annot_.x1_.sampling_
				: appearance().annot_.x2_.sampling_;
    if ( mIsUdf(sd.start) || mIsUdf(sd.step) )
	sd = getDefaultGridSampling( x1 );

    int coordidx = 0, ciidx = 0;
    visBase::Coordinates* coords = gridlines->getCoordinates();
    float pos = sd.start;
    while ( range.includes( pos ) )
    {
	const float relpos = (pos-range.start)/range.width();

	const Coord3 startpos = x1 
	    ? c00_*(1-relpos)+c10_*relpos
	    : c00_*(1-relpos)+c01_*relpos;
	const Coord3 stoppos = x1
	    ? c01_*(1-relpos)+c11_*relpos
	    : c10_*(1-relpos)+c11_*relpos;
	
	coords->setPos( coordidx, startpos );
	gridlines->setCoordIndex( ciidx++, coordidx++ );

	coords->setPos( coordidx, stoppos );
	gridlines->setCoordIndex( ciidx++, coordidx++ );
	gridlines->setCoordIndex( ciidx++, -1 );

	pos += sd.step;
    }

    gridlines->removeCoordIndexAfter( ciidx-1 );
    gridlines->turnOn( true );
}


void FlatViewer::allowShading( bool yn )
{
    channel2rgba_->allowShading( yn );
}


void FlatViewer::replaceChannels( TextureChannels* nt )
{
    if ( !nt )
	return;

    if ( channels_ )
    {
	removeChild( channels_->getInventorNode() );
	channels_->unRef();
    }

    channels_ = nt;
    channels_->ref();
}


Interval<float> FlatViewer::getDataRange( bool wva ) const
{
    const FlatView::DataDispPars::VD& vd = appearance().ddpars_.vd_;
    if ( !vd.rg_.isUdf() )
	return vd.rg_;
 
    const float clipstop = mIsUdf(vd.clipperc_.stop) ? mUdf(float) 
						     : vd.clipperc_.stop*0.01;
    Interval<float> cliprate( vd.clipperc_.start*0.01, clipstop );
   
    DataClipper clipper;
    if ( (wva && wvapack_) || (!wva && vdpack_) )
    	clipper.putData( wva ? wvapack_->data() : vdpack_->data() );
    clipper.fullSort();

    Interval<float> res;
    if ( mIsUdf(vd.symmidvalue_) )
    {
	if ( mIsUdf(cliprate.stop) )
	    clipper.getRange( cliprate.start, res );
	else
	    clipper.getRange( cliprate.start, cliprate.stop, res );
    }
    else
	clipper.getSymmetricRange( cliprate.start, vd.symmidvalue_, res );

    return res;
}


}; // Namespace
