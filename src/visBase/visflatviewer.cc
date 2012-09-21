/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Yuancheng Liu
 Date:		5-11-2007
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id$";

#include "visflatviewer.h"

#include "array2dresample.h"
#include "arraynd.h"
#include "coltabmapper.h"
#include "coltabsequence.h"
#include "dataclipper.h"
#include "flatposdata.h"
#include "axislayout.h"
#include "simpnumer.h"
#include "survinfo.h"
#include "viscoord.h"
#include "vismaterial.h"
#include "vispolyline.h"
#include "vistexturechannels.h"
#include "vissplittexture2rectangle.h"
#include "vistexturechannel2rgba.h"
#include "envvars.h"
#include "settings.h"

#define mNrResolutions 3

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
    , resolution_( 0 )							
{
    int resolutionfromsettings;

    // try getting default resolution from settings
    bool success = Settings::common().get(
	    "dTect.Default texture resolution factor", resolutionfromsettings );

    if ( success )
    {
	if ( resolutionfromsettings >= 0 && resolutionfromsettings <= 2 )
	    resolution_ = resolutionfromsettings;
	else if ( resolutionfromsettings == -1 )
	    success = false;
    }

    if ( !success )
    {
	// get default resolution from environment variable
	const char* envvar = GetEnvVar(
		"OD_DEFAULT_TEXTURE_RESOLUTION_FACTOR" );
	if ( envvar && isdigit(*envvar) )
	    resolution_ = toInt( envvar );
    }

    if ( resolution_ >= nrResolutions() )
	resolution_ = nrResolutions()-1;

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
		const FlatDataPack* dp = pack( false );
		if ( !dp )
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
			rowsz *= (resolution_+1);
			colsz *= (resolution_+1);
			
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
			    Array2DReSampler<float,float> resampler( dparr,
				    tmparr, rowsz, colsz, true );
			    resampler.setInterpolate( true );
			    resampler.execute();
			}
			
			arr = tmparr;
			cp = OD::TakeOverPtr;
		    }

		    channels_->setSize( 1, rowsz, colsz );
		    channels_->setUnMappedData( 0, 0, arr, cp, 0 );

		    appearance().ddpars_.vd_.ctab_ =
			channel2rgba_->getSequence(0)->name();
		    rectangle_->setOriginalTextureSize( rowsz, colsz );
		    channels_->turnOn( appearance().ddpars_.vd_.show_ );

		    dataChange.trigger();
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
    range.sort();
    const float rgwidth = !range.width() ? 1 : range.width();
    SamplingData<float> sd = x1 ? appearance().annot_.x1_.sampling_
				: appearance().annot_.x2_.sampling_;
    if ( mIsUdf(sd.start) || mIsUdf(sd.step) )
	sd = getDefaultGridSampling( x1 );

    int coordidx = 0, ciidx = 0;
    visBase::Coordinates* coords = gridlines->getCoordinates();
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
    const ColTab::MapperSetup mapper = vd.mappersetup_;
    Interval<float> range = mapper.range_;
    if ( !range.isUdf() )
	return range;
 
    DataClipper clipper;
    if ( (wva && wvapack_) || (!wva && vdpack_) )
    	clipper.putData( wva ? wvapack_->data() : vdpack_->data() );
    clipper.fullSort();

    Interval<float> res;
    if ( mIsUdf(mapper.symmidval_) )
	clipper.getRange( mapper.cliprate_.start, mapper.cliprate_.stop, res );
    else
	clipper.getSymmetricRange( mapper.cliprate_.start, mapper.symmidval_,
				   res );

    return res;
}


int FlatViewer::nrResolutions() const
{
    return mNrResolutions; 
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



}; // Namespace
