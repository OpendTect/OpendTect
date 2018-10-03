/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Yuancheng Liu
 Date:		5-11-2007
________________________________________________________________________

-*/

#include "visflatviewer.h"

#include "array2dresample.h"
#include "arraynd.h"
#include "coltabmapper.h"
#include "coltabseqmgr.h"
#include "dataclipper.h"
#include "flatposdata.h"
#include "axislayout.h"
#include "simpnumer.h"
#include "survinfo.h"
#include "viscoord.h"
#include "vismaterial.h"
#include "vispolyline.h"
#include "vistexturechannels.h"
#include "vistexturerect.h"
#include "vistexturechannel2rgba.h"
#include "envvars.h"
#include "settingsaccess.h"
#include "uistrings.h"


mCreateFactoryEntry( visBase::FlatViewer );

namespace visBase
{

FlatViewer::FlatViewer()
    : VisualObjectImpl(false)
    , dataChanged(this)
    , dispParsChanged(this)
    , channels_( TextureChannels::create() )
    , channel2rgba_( ColTabTextureChannel2RGBA::create() )
    , rectangle_( TextureRectangle::create() )
    , x1gridlines_( PolyLine::create() )
    , x2gridlines_( PolyLine::create() )
    , gridlinematerial_( new Material )
{
    resolution_ = SettingsAccess().getDefaultTexResFactor( nrResolutions() );

    channel2rgba_->ref();
    channel2rgba_->allowShading( true );

    channels_->ref();
    channels_->setChannels2RGBA( channel2rgba_ );

    if ( channels_->nrChannels()<1 )
    {
	channels_->addChannel();
	channel2rgba_->setEnabled( 0, true );
    }

    rectangle_->setMaterial( 0 );
    rectangle_->setTextureChannels( channels_ );
    addChild( rectangle_->osgNode() );

    gridlinematerial_->setColor( Color( 0, 0, 0 ) );

    x1gridlines_->ref();
    x1gridlines_->setMaterial( gridlinematerial_ );
    addChild( x1gridlines_->osgNode() );

    x2gridlines_->ref();
    x2gridlines_->setMaterial( gridlinematerial_ );
    addChild( x2gridlines_->osgNode() );
}


FlatViewer::~FlatViewer()
{
    channels_->unRef();
    channel2rgba_->unRef();
    x2gridlines_->unRef();
    x1gridlines_->unRef();
}


void FlatViewer::doHandleChange( unsigned int dt)
{
    switch ( dt )
    {
	case All:
	case Auxdata:
	case Annot:
	    {
		updateGridLines( true );
		updateGridLines( false );

		if ( dt!=All )
		    break;
	    }
	case BitmapData:
	    {
		ConstRefMan<FlatDataPack> dp = getPack( false );
		if ( !dp )
		    channels_->turnOn( false );
		else
		{
		    const Array2D<float>& dparr = dp->data();
		    const float* arr = dparr.getData();
		    OD::PtrPolicy cp = OD::UsePtr;

		    int rowsz = dparr.getSize(0);
		    int colsz = dparr.getSize(1);

		    if ( !arr || resolution_!=0 )
		    {
			rowsz = 1 + (rowsz-1) * (resolution_+1);
			colsz = 1 + (colsz-1) * (resolution_+1);

			const od_int64 totalsz = rowsz*colsz;
			mDeclareAndTryAlloc( float*, tmparr, float[totalsz] );

			if ( !tmparr )
			{
			    channels_->turnOn( false );
			    return;
			}
			if ( resolution_==0 )
			    dparr.getAll( tmparr );
			else
			{
			    Array2DReSampler<float,float>
				resampler( dparr, tmparr, rowsz, colsz, true );
			    resampler.setInterpolate( true );
			    resampler.execute();
			}


			arr = tmparr;
			cp = OD::TakeOverPtr;
		    }

		    channels_->setSize( 0, 1, rowsz, colsz );
		    channels_->setUnMappedData( 0, 0, arr, cp,
					        SilentTaskRunnerProvider() );

		    appearance().ddpars_.vd_.colseqname_ =
			channel2rgba_->getSequence(0).name();
		    channels_->turnOn( appearance().ddpars_.vd_.show_ );

		    dataChanged.trigger();
		    if ( dt!=All )
			break;
		}
	    }
	case DisplayPars:
	    {
		const FlatView::DataDispPars::VD& vd = appearance().ddpars_.vd_;
		if ( &channels_->getColTabMapper(0) != vd.mapper_.ptr() )
		{
		    channels_->setColTabMapper( 0, *vd.mapper_ );
		    channels_->reMapData( 0, SilentTaskRunnerProvider() );
		}

		const ColTab::Sequence& sequence
			= channel2rgba_->getSequence( 0 );
		if ( vd.colseqname_ != sequence.name() )
		    channel2rgba_->setSequence( 0,
				    *ColTab::SeqMGR().getAny(vd.colseqname_) );
		dispParsChanged.trigger();
	    }
    }
}


void FlatViewer::setPosition( const Coord3& c00, const Coord3& c01,
			      const Coord3& c10, const Coord3& c11 )
{
    const Coord3 center = 0.5 * (c01+c10);
    if ( (c00+c11-2*center).abs<float>() > 1e-4*(c11-c00).abs<float>() )
    {
	pErrMsg( "Non-rectangular flatviewing not yet implemented" );
    }

    rectangle_->setCenter( center );
    rectangle_->setRotationAndWidth( c10-c00, c01-c00 );
    rectangle_->swapTextureAxes();

    c00_ = c00;
    c01_ = c01;
    c10_ = c10;
    c11_ = c11;

    updateGridLines( true );
    updateGridLines( false );
}


const SamplingData<float> FlatViewer::getDefaultGridSampling( bool x1 ) const
{
    ConstRefMan<FlatDataPack> dp = getPack( false );
    if ( !dp )
	return SamplingData<float>( 0, 1 );

    Interval<float> range;
    range.setFrom( dp->posData().range( x1 ) );
    AxisLayout<float> layout( range );
    return layout.sd_;
}


void FlatViewer::turnOnGridLines( bool offsetlines, bool zlines )
{
    x1gridlines_->turnOn( offsetlines );
    x2gridlines_->turnOn( zlines );
}


void FlatViewer::updateGridLines( bool x1 )
{
    const Color markcolor = channel2rgba_->getSequence(0).markColor();
    x1gridlines_->getMaterial()->setColor( markcolor );
    x2gridlines_->getMaterial()->setColor( markcolor );

    ConstRefMan<FlatDataPack> dp = getPack( false );
    PolyLine* gridlines = x1 ? x1gridlines_ : x2gridlines_;

    if ( !dp || (x1 && !appearance().annot_.x1_.showgridlines_ ) ||
	 (!x1 && !appearance().annot_.x2_.showgridlines_ ) )
    {
	gridlines->turnOn( false );
	return;
    }

    gridlines->removeAllPrimitiveSets();
    gridlines->getCoordinates()->setEmpty();

    Interval<float> range; range.setFrom( dp->posData().range( x1 ) );
    range.sort();
    const float rgwidth = !range.width() ? 1 : range.width();
    SamplingData<float> sd = x1 ? appearance().annot_.x1_.sampling_
				: appearance().annot_.x2_.sampling_;
    if ( mIsUdf(sd.start) || mIsUdf(sd.step) )
	sd = getDefaultGridSampling( x1 );

    float pos = sd.start;
    while ( range.includes( pos, false ) )
    {
	const float relpos = (pos-range.start)/rgwidth;

	const Coord3 startpos = x1
	    ? c00_*(1-relpos)+c10_*relpos
	    : c00_*(1-relpos)+c01_*relpos;
	const Coord3 stoppos = x1
	    ? c01_*(1-relpos)+c11_*relpos
	    : c10_*(1-relpos)+c11_*relpos;

	gridlines->addPoint( startpos );
	gridlines->addPoint( stoppos );
	const int lastidx = gridlines->size();
	Geometry::RangePrimitiveSet* ps =
	    Geometry::RangePrimitiveSet::create();
	Interval<int> psrange( lastidx-2, lastidx -1);
	ps->setRange( psrange );
	ps->ref();
	gridlines->addPrimitiveSet( ps );
	pos += sd.step;
    }

    gridlines->dirtyCoordinates();
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
	removeChild( channels_->osgNode() );
	channels_->unRef();
    }

    channels_ = nt;
    channels_->ref();
}


Interval<float> FlatViewer::getDataRange( bool wva ) const
{
    return mapper(wva)->getRange();
}


void FlatViewer::setResolution( int res )
{
    if ( res==resolution_ )
	return;

    resolution_ = res;
    handleChange( Viewer::BitmapData );
}


uiWord FlatViewer::getResolutionName( int res ) const
{
    if ( res == 1 )
	return uiStrings::sHigher();
    if ( res == 2 )
	return uiStrings::sHighest();

    if ( res != 0 )
	{ pErrMsg("Resolution out of range" ); }

    return uiStrings::sStandard();
}


void FlatViewer::setDisplayTransformation( const mVisTrans* trans )
{
   if ( rectangle_ )
    rectangle_->setDisplayTransformation( trans );

   if ( x1gridlines_ )
       x1gridlines_->setDisplayTransformation( trans );

   if ( x2gridlines_ )
       x2gridlines_->setDisplayTransformation( trans );

}


void FlatViewer::setPixelDensity( float dpi )
{
    VisualObjectImpl::setPixelDensity( dpi );

    if ( x1gridlines_ )
	x1gridlines_->setPixelDensity( dpi );
    if ( x2gridlines_ )
	x2gridlines_->setPixelDensity( dpi );

}

}; // Namespace
