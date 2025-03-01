/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "visflatviewer.h"

#include "array2dresample.h"
#include "arraynd.h"
#include "axislayout.h"
#include "coltabmapper.h"
#include "coltabsequence.h"
#include "dataclipper.h"
#include "envvars.h"
#include "flatposdata.h"
#include "settingsaccess.h"
#include "simpnumer.h"
#include "survinfo.h"
#include "viscoord.h"
#include "vismaterial.h"
#include "vispolyline.h"
#include "vistexturechannels.h"
#include "vistexturerect.h"
#include "vistexturechannel2rgba.h"


mCreateFactoryEntry( visBase::FlatViewer );

namespace visBase
{

FlatViewer::FlatViewer()
    : VisualObjectImpl(false)
    , dataChanged(this)
    , dispParsChanged(this)
{
    ref();
    channels_ = TextureChannels::create();
    channel2rgba_ = ColTabTextureChannel2RGBA::create();
    rectangle_ = TextureRectangle::create();
    x1gridlines_ = PolyLine::create();
    x2gridlines_ = PolyLine::create();
    resolution_ = SettingsAccess().getDefaultTexResFactor( nrResolutions() );

    channel2rgba_->allowShading( true );
    channels_->setChannels2RGBA( channel2rgba_.ptr() );
    if ( channels_->nrChannels()<1 )
    {
	channels_->addChannel();
	channel2rgba_->setEnabled( 0, true );
    }

    rectangle_->setMaterial( nullptr );
    rectangle_->setTextureChannels( channels_.ptr() );
    addChild( rectangle_->osgNode() );

    RefMan<Material> gridlinematerial = Material::create();
    gridlinematerial->setColor( OD::Color(0,0,0) );

    x1gridlines_->setMaterial( gridlinematerial.ptr() );
    addChild( x1gridlines_->osgNode() );

    x2gridlines_->setMaterial( gridlinematerial.ptr() );
    addChild( x2gridlines_->osgNode() );
    unRefNoDelete();
}


FlatViewer::~FlatViewer()
{
}


void FlatViewer::handleChange( unsigned int dt )
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
		ConstRefMan<FlatDataPack> dp = getPack( false ).get();
		if ( !dp || !dp->isOK() )
		    channels_->turnOn( false );
		else
		{
		    const Array2D<float>& dparr = dp->data();
		    const float* arr = dparr.getData();
		    OD::PtrPolicy cp = OD::UsePtr;

		    int rowsz = dparr.info().getSize(0);
		    int colsz = dparr.info().getSize(1);

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
		    channels_->setUnMappedData( 0, 0, arr, cp, 0 );

		    appearance().ddpars_.vd_.ctab_ =
			channel2rgba_->getSequence(0)->name();
		    channels_->turnOn( appearance().ddpars_.vd_.show_ );

		    dataChanged.trigger();
		    if ( dt!=All )
			break;
		}
	    }
	case DisplayPars:
	    {
		const FlatView::DataDispPars::VD& vd = appearance().ddpars_.vd_;
		ColTab::MapperSetup mappersetup;
		mappersetup = vd.mappersetup_;
		if ( channels_->getColTabMapperSetup( 0,0 )!=mappersetup )
		{
		    channels_->setColTabMapperSetup( 0, mappersetup );
		    channels_->reMapData( 0, false, 0 );
		}

		ColTab::Sequence sequence = *channel2rgba_->getSequence( 0 );
		if ( vd.ctab_!=sequence.name() )
		{
		    if ( ColTab::SM().get( vd.ctab_, sequence ) )
		    {
			channel2rgba_->setSequence( 0, sequence );
		    }
		}
		dispParsChanged.trigger();
	    }
    }
}


void FlatViewer::setPosition( const Coord3& c00, const Coord3& c01,
			      const Coord3& c10, const Coord3& c11 )
{
    const Coord3 center = 0.5 * (c01+c10);
    if ( (c00+c11-2*center).abs() > 1e-4*(c11-c00).abs() )
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
    ConstRefMan<FlatDataPack> dp = getPack( false ).get();
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
    if ( channel2rgba_->getSequence(0) )
    {
	const OD::Color markcolor = channel2rgba_->getSequence(0)->markColor();
	x1gridlines_->getMaterial()->setColor( markcolor );
	x2gridlines_->getMaterial()->setColor( markcolor );
    }

    ConstRefMan<FlatDataPack> dp = getPack( false ).get();
    PolyLine* gridlines = x1 ? x1gridlines_.ptr() : x2gridlines_.ptr();

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
    if ( mIsUdf(sd.start_) || mIsUdf(sd.step_) )
	sd = getDefaultGridSampling( x1 );

    if ( sd.start_ < range.start_ )
        sd.start_ = sd.atIndex( sd.indexOnOrAfter(range.start_) );

    float pos = sd.start_;
    while ( range.includes( pos, false ) )
    {
        const float relpos = (pos-range.start_)/rgwidth;

	const Coord3 startpos = x1
	    ? c00_*(1-relpos)+c10_*relpos
	    : c00_*(1-relpos)+c01_*relpos;
	const Coord3 stoppos = x1
	    ? c01_*(1-relpos)+c11_*relpos
	    : c10_*(1-relpos)+c11_*relpos;

	gridlines->addPoint( startpos );
	gridlines->addPoint( stoppos );
	const int lastidx = gridlines->size();
	RefMan<Geometry::RangePrimitiveSet> ps =
				Geometry::RangePrimitiveSet::create();
	Interval<int> psrange( lastidx-2, lastidx -1);
	ps->setRange( psrange );
	gridlines->addPrimitiveSet( ps.ptr() );
        pos += sd.step_;
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
	removeChild( channels_->osgNode() );

    channels_ = nt;
}


Interval<float> FlatViewer::getDataRange( bool wva ) const
{
    const FlatView::DataDispPars::VD& vd = appearance().ddpars_.vd_;
    const ColTab::MapperSetup mapper = vd.mappersetup_;
    Interval<float> range = mapper.range_;
    if ( !range.isUdf() )
	return range;

    DataClipper clipper;
    ConstRefMan<FlatDataPack> dp = getPack( wva ).get();
    if ( dp && dp->isOK() )
	clipper.putData( dp->data() );
    clipper.fullSort();

    Interval<float> res;
    if ( mIsUdf(mapper.symmidval_) )
        clipper.getRange( mapper.cliprate_.start_, mapper.cliprate_.stop_, res );
    else
        clipper.getSymmetricRange( mapper.cliprate_.start_, mapper.symmidval_,
				   res );

    return res;
}


void FlatViewer::setResolution( int res )
{
    if ( res==resolution_ )
	return;

    resolution_ = res;
    handleChange( Viewer::BitmapData );
}


BufferString FlatViewer::getResolutionName( int res ) const
{
    if ( res == 0 ) return "Standard";
    else if ( res == 1 ) return "Higher";
    else if ( res == 2 ) return "Highest";
    else return "?";
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

} // namespace visBase
