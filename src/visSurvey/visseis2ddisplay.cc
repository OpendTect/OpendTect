/*+
 ________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          August 2004
 ________________________________________________________________________

-*/

static const char* rcsID = "$Id: visseis2ddisplay.cc,v 1.126 2011-03-15 21:12:19 cvsyuancheng Exp $";

#include "visseis2ddisplay.h"

#include "viscolortab.h"
#include "viscoord.h"
#include "visdataman.h"
#include "visevent.h"
#include "vismultitexture2.h"
#include "vismaterial.h"
#include "vistext.h"
#include "vistexturechannels.h"
#include "vistexturechannel2rgba.h"
#include "vistransform.h"
#include "vissplittextureseis2d.h"

#include "array2dresample.h"
#include "arrayndimpl.h"
#include "arrayndslice.h"
#include "attribdataholder.h"
#include "attribdatapack.h"
#include "coltabmapper.h"
#include "genericnumer.h"
#include "ioman.h"
#include "ioobj.h"
#include "iopar.h"
#include "keystrs.h"
#include "mousecursor.h"
#include "ptrman.h"
#include "samplfunc.h"
#include "posinfo.h"
#include "seisinfo.h"
#include "survinfo.h"
#include "task.h"
#include "zaxistransform.h"

//For parsing old pars
#include "attribsel.h"
#include "vistexture2.h"

#define mMaxImageSize 300000000	// 32767

// Define mUseSeis2DArray to make use of Seis2DArray and consume less memory in 
// setData. Undefine it to makes use of Seis2DTextureDataArrayFiller alone.
#define mUseSeis2DArray

mCreateFactoryEntry( visSurvey::Seis2DDisplay );

using namespace Attrib;

namespace visSurvey
{

const char* Seis2DDisplay::sKeyLineSetID()	{ return "LineSet ID"; }
const char* Seis2DDisplay::sKeyTrcNrRange()	{ return "Trc Nr Range"; }
const char* Seis2DDisplay::sKeyZRange()		{ return "Z Range"; }
const char* Seis2DDisplay::sKeyShowLineName()	{ return "Show linename"; }
const char* Seis2DDisplay::sKeyTextureID()	{ return "Texture ID"; }

class Seis2DTextureDataArrayFiller: public ParallelTask
{
public:

Seis2DTextureDataArrayFiller( const Seis2DDisplay& s2d, 
			      const Attrib::Data2DHolder& dh,  int seriesid,
			      Array2DImpl<float>* array,
			      const SamplingData<float>& outputzsd, int attrib )
    : data2dh_( dh )
    , arr_( array )	
    , valseridx_( dh.dataset_[0]->validSeriesIdx()[seriesid] )
    , s2d_( s2d )
    , attrib_( attrib )
    , outputptr_( 0 )
    , outputzsd_( outputzsd )	       
{
    sx_ = arr_->info().getSize(0);
    sy_ = arr_->info().getSize(1);    
}

      	
Seis2DTextureDataArrayFiller( const Seis2DDisplay& s2d, 
			      const Attrib::Data2DHolder& dh, int seriesid,
			      float* outptr, int sx, int sy,
			      const SamplingData<float>& outputzsd, int attrib )
    : data2dh_( dh )
    , valseridx_( dh.dataset_[0]->validSeriesIdx()[seriesid] )
    , s2d_( s2d )
    , attrib_( attrib )
    , outputptr_( outptr )
    , sx_( sx )
    , sy_ (sy )
    , outputzsd_( outputzsd )	       
{}


od_int64 nrIterations() const { return data2dh_.size(); }


private:

bool doWork( od_int64 start, od_int64 stop, int threadid )
{
    const int nrsamp = data2dh_.dataset_[0]->nrsamples_;
    const SamplingData<float>& sd = data2dh_.trcinfoset_[0]->sampling; 
    StepInterval<float> zrg = s2d_.getZRange(!s2d_.datatransform_,attrib_);
    StepInterval<int> arraysrg( mNINT(zrg.start/sd.step),
				mNINT(zrg.stop/sd.step),1 );
    const float firstz = data2dh_.dataset_[0]->z0_*sd.step;
    const int firstdhsample = sd.nearestIndex( firstz );  

    const bool samplebased = mIsEqual( sd.step, outputzsd_.step, 1e-3 ) &&
	mIsEqual( sd.getIndex(firstz), firstdhsample, 1e-3 );

    const TypeSet<int>& allpos = s2d_.trcdisplayinfo_.alltrcnrs;
    const int starttrcidx = allpos.indexOf( s2d_.trcdisplayinfo_.rg.start );
    const bool usez0 = s2d_.datatransform_ || zrg.start <= sd.start;

    for ( int idx=start; idx<=stop && shouldContinue(); idx++ )
    {
	const int trcnr = data2dh_.trcinfoset_[idx]->nr;
	if ( !s2d_.trcdisplayinfo_.rg.includes(trcnr) )
	{
	    addToNrDone( 1 );
	    continue;
	}

	const DataHolder* dh = data2dh_.dataset_[idx];
	const int trcidx = allpos.indexOf( trcnr ) - starttrcidx;
	if ( !dh ||  trcidx < 0 || trcidx >= sx_ )
	{
	    addToNrDone( 1 );
	    continue;
	}

	const ValueSeries<float>* dataseries = dh->series( valseridx_ );
	const int shift = usez0 ? dh->z0_ : mNINT(sd.start/sd.step);

	for ( int idy=0; idy<sy_; idy++ )
	{
	    float val;
	    int arrzidx;

	    if ( samplebased )
	    {
		const int smp = firstdhsample+idy;    
		const int validx = usez0 ? smp : smp+shift-dh->z0_;
		
		arrzidx = arraysrg.getIndex( smp+shift );
		if ( arrzidx<0 || arrzidx>=sy_ )
		{
		    addToNrDone( 1 );
		    continue;
                }

		val = ( dh->dataPresent(smp+shift) ) ?
			dataseries->value( validx ) : mUdf(float);
	    }
	    else
	    {
		arrzidx = idy;
		val = mUdf(float);
		const float offset = idy * outputzsd_.step / sd.step;
		IdxAble::interpolateRegWithUdf( *dataseries, nrsamp,
						offset+firstz, val );
	    }
	    
	    if ( outputptr_ )
		*( outputptr_ + (trcidx * sy_) + arrzidx ) = val ;
	    else
		arr_->set( trcidx, arrzidx, val );
	}

	addToNrDone( 1 );
    }

    return true;
}

    Array2DImpl<float>*			arr_;
    float*				outputptr_;
    int					sx_, sy_;	// size of output array
    const Attrib::Data2DHolder&		data2dh_;
    const Seis2DDisplay&		s2d_;
    const int				valseridx_;
    int					attrib_;
    const SamplingData<float>&		outputzsd_;
};


// Class to provide access to the data in the 2D data holder in array-format.
class Seis2DArray : public Array2D<float>
{
public:

Seis2DArray( const Seis2DDisplay& s2d, const Attrib::Data2DHolder& d2dh,
	     const StepInterval<float>& arrayzrg, int zsz, int attrib )
    : s2d_( s2d )
    , data2dh_( d2dh )
    , arrayzrg_( arrayzrg )
    , attrib_( attrib )
    , info_( *new Array2DInfoImpl( s2d.trcdisplayinfo_.size, zsz ) )
{}

~Seis2DArray() 					{ delete &info_; }
bool isOK()					{ return true; }
const Array2DInfo& info() const			{ return info_; }
void set( int idx0, int idx1, float val ) 	{}

// Fills ptr with values from array. ptr is assumed to be allocated with
// info().getTotalSz() number of values.
void getAll(float* ptr) const
{
    MemSetter<float> memsetter;
    memsetter.setSize( info().getSize(0) * info().getSize(1) );
    memsetter.setValue( mUdf(float) );
    memsetter.setTarget( ptr );
    memsetter.execute();

    Seis2DTextureDataArrayFiller arrayfiller( s2d_, data2dh_, s2d_.seriesidx_, 
	    ptr, info().getSize(0), info().getSize(1), arrayzrg_, attrib_ );
    arrayfiller.execute();
}


float get( int idx0, int idx1 ) const
{
    if ( s2d_.seriesidx_ < 0 || idx0 < 0 || idx0 >= info().getSize(0) || 
	    			idx1 < 0 || idx1 >= info().getSize(1) )
	return mUdf(float);

    const SamplingData<float>& sd = data2dh_.trcinfoset_[0]->sampling;
    const float firstz = data2dh_.dataset_[0]->z0_ * sd.step;
    const float firstdhsamplef = sd.getIndex( firstz );
    const int firstdhsample = sd.nearestIndex( firstz );
    const bool samestep = s2d_.datatransform_ ||
	mIsEqual(sd.step,s2d_.getScene()->getCubeSampling().zrg.step,1e-3);
    const bool samplebased = mIsEqual(firstdhsamplef,firstdhsample,1e-3)
	&& samestep;
    if ( !samplebased )
	return mUdf(float);

    const TypeSet<int>& allpos = s2d_.trcdisplayinfo_.alltrcnrs;
    const int starttrcidx = allpos.indexOf( s2d_.trcdisplayinfo_.rg.start );
    const int trcnr = allpos[idx0 + starttrcidx];
    if ( !s2d_.trcdisplayinfo_.rg.includes( trcnr ) )
	return mUdf(float);
    
    const int dataidx = data2dh_.getDataHolderIndex( trcnr );
    if ( dataidx == -1 || !data2dh_.dataset_[dataidx] )	    
	return mUdf(float);

    const DataHolder* dh = data2dh_.dataset_[dataidx];
    TypeSet<int> valididxs = data2dh_.dataset_[0]->validSeriesIdx();
    const ValueSeries<float>* dataseries = 
	    dh->series( valididxs[s2d_.seriesidx_] );

    StepInterval<int> arraysrg( mNINT(arrayzrg_.start/sd.step),
				mNINT(arrayzrg_.stop/sd.step),1 );
    
    const bool usez0 = s2d_.datatransform_ || arrayzrg_.start <= sd.start;
    const int shift = usez0 ? dh->z0_ : mNINT(sd.start/sd.step);
    const int samp = arraysrg.atIndex( idx1 );
    const int validx = usez0 ? samp - shift : samp - dh->z0_;
    const float val = dh->dataPresent( samp ) ? dataseries->value( validx )
	    			   		: mUdf(float);
    return val;
}

protected:

    const Seis2DDisplay&		s2d_;
    const Attrib::Data2DHolder&		data2dh_;
    const StepInterval<float>&		arrayzrg_;
    Array2DInfoImpl&			info_;
    int					attrib_;
};


Seis2DDisplay::Seis2DDisplay()
    : MultiTextureSurveyObject( true )
    , transformation_(0)
    , geometry_(*new PosInfo::Line2DData)
    , triangles_( visBase::SplitTextureSeis2D::create() )	 
    , geomchanged_(this)
    , maxtrcnrrg_(INT_MAX,INT_MIN,1)
    , datatransform_(0)
    , voiidx_(-1)
    , seriesidx_(-1)
    , prevtrcidx_(0)
{
    geometry_.setZRange( StepInterval<float>(mUdf(float),mUdf(float),1) );
    cache_.allowNull();

    triangles_->ref();
    triangles_->removeSwitch();
    addChild( triangles_->getInventorNode() );

    linename_ = visBase::Text2::create();
    linename_->ref();
    addChild( linename_->getInventorNode() );

    getMaterial()->setColor( Color::White() );
    getMaterial()->setAmbience( 0.8 );
    getMaterial()->setDiffIntensity( 0.2 );
}


Seis2DDisplay::~Seis2DDisplay()
{
    if ( linename_ )
    {
	removeChild( linename_->getInventorNode() );
	linename_->unRef();
    }

    delete &geometry_;

    if ( transformation_ ) transformation_->unRef();
    deepUnRef( cache_ );

    DataPackMgr& dpman = DPM( DataPackMgr::FlatID() );
    for ( int idx=0; idx<datapackids_.size(); idx++ )
	dpman.release( datapackids_[idx] );
    
    triangles_->unRef();
    setZAxisTransform( 0,0 );
}


void Seis2DDisplay::setLineInfo( const MultiID& lid, const char* lnm )
{
    linesetid_ = lid;
    PtrMan<IOObj> seis2dobj = IOM().get( lid );
    if ( !seis2dobj )
	return;

    geomid_ = S2DPOS().getGeomID( seis2dobj->name(), lnm );
    setName( lnm );
    if ( linename_ )
    {
	if ( scene_ )
	    setAnnotColor( scene_->getAnnotColor() );
	linename_->setText( lnm );
    }
}


const char* Seis2DDisplay::getLineName() const
{ return geomid_.isOK() ? S2DPOS().getLineName( geomid_.lineid_ ) : name(); }


PosInfo::GeomID Seis2DDisplay::getGeomID() const
{ return geomid_; }


void Seis2DDisplay::setGeometry( const PosInfo::Line2DData& geometry )
{
    geometry_ = geometry;
    const TypeSet<PosInfo::Line2DPos>& linepositions = geometry.positions();
    const int tracestep = geometry.trcNrRange().step;
    maxtrcnrrg_.set( INT_MAX, INT_MIN, tracestep );
    triangles_->setPath( linepositions );

    const int possz = linepositions.size();
    trcdisplayinfo_.alltrcnrs.erase();	
    for ( int idx=0; idx<possz; idx++ )
    {
	maxtrcnrrg_.include( linepositions[idx].nr_, false );
	trcdisplayinfo_.alltrcnrs += linepositions[idx].nr_;
    }

    trcdisplayinfo_.zrg.step = datatransform_ ? datatransform_->getGoodZStep()
					      : geometry_.zRange().step;
    setTraceNrRange( maxtrcnrrg_ );
    setZRange( geometry_.zRange() );
	    
    updateRanges( false, true );
    geomchanged_.trigger();
}


StepInterval<float> Seis2DDisplay::getMaxZRange( bool displayspace ) const
{
    if ( !datatransform_ || !displayspace )
	return geometry_.zRange();

    StepInterval<float> zrg;
    zrg.setFrom( datatransform_->getZInterval(false) );
    zrg.step = geometry_.zRange().step;
    return zrg;
}


void Seis2DDisplay::setZRange( const StepInterval<float>& nzrg )
{
    if ( mIsUdf(geometry_.zRange().start) )
	return;

    const StepInterval<float> maxzrg = getMaxZRange( true );
    const Interval<float> zrg( mMAX(maxzrg.start,nzrg.start),
			       mMIN(maxzrg.stop,nzrg.stop) );
    const bool hasdata = !cache_.isEmpty() && cache_[0];
    if ( hasdata && trcdisplayinfo_.zrg.isEqual(zrg,mDefEps) )
	return;

    trcdisplayinfo_.zrg.setFrom( zrg );
    if ( trcdisplayinfo_.zrg.nrSteps()+1 > mMaxImageSize )
	trcdisplayinfo_.zrg.stop = trcdisplayinfo_.zrg.start +
	    (mMaxImageSize-1) * trcdisplayinfo_.zrg.step;
    
    updateVizPath();
    geomchanged_.trigger();
}


StepInterval<float>
	Seis2DDisplay::getZRange( bool displayspace, int attrib ) const
{
    const FixedString zdomainkey =
	(attrib>=0 && attrib<nrAttribs()) ? getSelSpec(attrib)->zDomainKey()
					  : 0;
    const bool alreadytransformed =
	scene_ && zdomainkey == scene_->zDomainKey();
    if ( alreadytransformed )
	return trcdisplayinfo_.zrg;

    if ( datatransform_ && !displayspace )
    {
	StepInterval<float> zrg = datatransform_->getZInterval( true );
	zrg.step = SI().zStep();
	return zrg;
    }

    return trcdisplayinfo_.zrg;
}


const Interval<int> Seis2DDisplay::getSampleRange() const
{
    StepInterval<float> maxzrg = getMaxZRange( true );
    Interval<int> samplerg( maxzrg.nearestIndex(trcdisplayinfo_.zrg.start),
	    		    maxzrg.nearestIndex(trcdisplayinfo_.zrg.stop) );
    return samplerg;
}


#define mRetErrGeo \
{ pErrMsg("Geometry not set"); return; }

void Seis2DDisplay::setTraceNrRange( const Interval<int>& trcrg )
{
    if ( maxtrcnrrg_.isRev() )
    {
	pErrMsg("Geometry not set");
	return;
    }

    const Interval<int> rg( maxtrcnrrg_.limitValue(trcrg.start),
			    maxtrcnrrg_.limitValue(trcrg.stop) );

    if ( !rg.width() )
	return;

    trcdisplayinfo_.rg = rg;
    int startidx = trcdisplayinfo_.alltrcnrs.indexOf( rg.start );
    if ( startidx<0 )
    {
	for ( int idx=0; idx<trcdisplayinfo_.alltrcnrs.size(); idx++ )
	{
	    if ( trcdisplayinfo_.alltrcnrs[idx]>= rg.start )
	    {
		startidx = idx;
		trcdisplayinfo_.rg.start = trcdisplayinfo_.alltrcnrs[idx];
		break;
	    }
	}
    }
    if ( startidx<0 )
	mRetErrGeo;
    
    int stopidx = trcdisplayinfo_.alltrcnrs.indexOf( rg.stop );
    if ( stopidx<0 )
    {
	for ( int idx=trcdisplayinfo_.alltrcnrs.size()-1; idx>=0; idx-- )
	{
	    if ( trcdisplayinfo_.alltrcnrs[idx] <= rg.stop )
	    {
		stopidx = idx;
		trcdisplayinfo_.rg.stop = trcdisplayinfo_.alltrcnrs[idx];
		break;
	    }
	}
    }
    if ( stopidx<0 )
	mRetErrGeo;

    trcdisplayinfo_.size = stopidx - startidx + 1;
    if ( trcdisplayinfo_.size > mMaxImageSize )
    {
	trcdisplayinfo_.rg.stop =
	    trcdisplayinfo_.alltrcnrs[startidx + mMaxImageSize-1];
	trcdisplayinfo_.size = mMaxImageSize;
    }
        
    updateVizPath();
}


const Interval<int>& Seis2DDisplay::getTraceNrRange() const
{ return trcdisplayinfo_.rg; }


const StepInterval<int>& Seis2DDisplay::getMaxTraceNrRange() const
{ return maxtrcnrrg_; }


bool Seis2DDisplay::setDataPackID( int attrib, DataPack::ID dpid,
				   TaskRunner* tr )
{
    DataPackMgr& dpman = DPM( DataPackMgr::FlatID() );
    const DataPack* datapack = dpman.obtain( dpid );
    mDynamicCastGet(const Flat2DDHDataPack*,dp2d,datapack);
    if ( !dp2d )
    {
	dpman.release( dpid );
	return false;
    }

    setTraceData( attrib, dp2d->dataholder(), tr );

    DataPack::ID oldid = datapackids_[attrib];
    datapackids_[attrib] = dpid;
    dpman.release( oldid );
    return true;
}


DataPack::ID Seis2DDisplay::getDataPackID( int attrib ) const
{
    return datapackids_[attrib];
}


void Seis2DDisplay::setTraceData( int attrib,
				  const Attrib::Data2DHolder& dataset,
				  TaskRunner* tr )
{
    setData( attrib, dataset, tr );
    if ( cache_[attrib] ) cache_[attrib]->unRef();

    cache_.replace( attrib, &dataset );
    cache_[attrib]->ref();
}


const Attrib::Data2DHolder* Seis2DDisplay::getCache( int attrib ) const
{ return cache_[attrib]; }


void Seis2DDisplay::setData( int attrib,
			     const Attrib::Data2DHolder& data2dh,
       			     TaskRunner* tr )
{
    if ( data2dh.isEmpty() ) 
    {
	channels_->setUnMappedData( attrib, 0, 0, OD::UsePtr, tr );
	channels_->turnOn( false );
	return;
    }

    const SamplingData<float>& sd = data2dh.trcinfoset_[0]->sampling;

    StepInterval<float> arrayzrg;
    arrayzrg.setFrom( getZRange(!datatransform_,attrib) );
    arrayzrg.step = sd.step;
    const int arrzsz = mNINT( arrayzrg.nrfSteps() )+1;

#ifndef mUseSeis2DArray
    mDeclareAndTryAlloc( PtrMan<Array2DImpl<float> >, arr,
	    Array2DImpl<float>( trcdisplayinfo_.size, arrzsz ) );
    if ( !arr->isOK() )
	return;
#endif

    const DataHolder* firstdh = data2dh.dataset_[0];
    TypeSet<int> valididxs = firstdh->validSeriesIdx();
    const int nrseries = valididxs.size();
    if ( nrseries == 0 )
	return;

    if ( texture_ )
	texture_->setNrVersions( attrib, nrseries );
    else
	channels_->setNrVersions( attrib, nrseries );

#ifdef mUseSeis2DArray
    Seis2DArray arr( *this, data2dh, arrayzrg, arrzsz, attrib );
#endif

    MouseCursorChanger cursorlock( MouseCursor::Wait );

    for ( seriesidx_=0; seriesidx_<nrseries; seriesidx_++ )
    {
#ifndef mUseSeis2DArray
	arr->setAll( mUdf(float) );

	Seis2DTextureDataArrayFiller arrayfiller( *this, data2dh, seriesidx_, 
		arr, arrayzrg, attrib );
	if ( !arrayfiller.execute() )
	    continue;
#endif

	const FixedString zdomainkey = getSelSpec(attrib)->zDomainKey();
	const bool alreadytransformed =
	    scene_ && zdomainkey == scene_->zDomainKey();
	PtrMan<Array2D<float> > tmparr = 0;
	Array2D<float>* usedarr = 0;
	if ( alreadytransformed || !datatransform_ )
#ifdef mUseSeis2DArray
	    usedarr = &arr;
#else
            usedarr = arr;
#endif
	else
	{
	    CubeSampling cs;
	    cs.hrg.start.inl = cs.hrg.stop.inl = 0;
	    cs.hrg.start.crl = trcdisplayinfo_.rg.start;
	    cs.hrg.stop.crl = trcdisplayinfo_.rg.stop;
	    cs.hrg.step.crl = 1;
	    assign( cs.zrg, trcdisplayinfo_.zrg );
	    // use survey step here?
	    if ( voiidx_ < 0 )
		voiidx_ = datatransform_->addVolumeOfInterest(
						getLineName(), cs, true );
	    else
		datatransform_->setVolumeOfInterest( voiidx_, getLineName(),
						     cs, true );
	    datatransform_->loadDataIfMissing( voiidx_ );

	    ZAxisTransformSampler outpsampler( *datatransform_, true,
		SamplingData<double>(cs.zrg.start,cs.zrg.step), true );
	    outpsampler.setLineName( getLineName() );
	    mTryAlloc( tmparr, 
		    Array2DImpl<float>( trcdisplayinfo_.size, cs.nrZ() ) );
	    usedarr = tmparr;
	    const float firstz = data2dh.dataset_[0]->z0_ * sd.step;
	    const int z0idx = arrayzrg.nearestIndex( firstz );
	    const int startidx = trcdisplayinfo_.alltrcnrs.indexOf( 
		    trcdisplayinfo_.rg.start );

	    for ( int crlidx=0; crlidx<trcdisplayinfo_.size; crlidx++ )
	    {
#ifdef 	mUseSeis2DArray
	        Array1DSlice<float> slice( arr );    
#else
	        Array1DSlice<float> slice( *arr );		
#endif
		slice.setDimMap( 0, 1 );
		slice.setPos( 0, crlidx+startidx );
		if ( !slice.init() )
		{
		    pErrMsg( "Error reading array for Z-axis transformation." );
		    continue; 
		}

		outpsampler.setTrcNr( 
			trcdisplayinfo_.alltrcnrs[crlidx+startidx] );
		outpsampler.computeCache( Interval<int>(0,cs.nrZ()-1) );

		SampledFunctionImpl<float,ValueSeries<float> >
		    inputfunc( slice, arrzsz, firstz, sd.step );
		inputfunc.setHasUdfs( true );
		inputfunc.setInterpolate( textureInterpolationEnabled() );

		float* outputptr = tmparr->getData() +
				   tmparr->info().getOffset( crlidx, z0idx );	
		reSample( inputfunc, outpsampler, outputptr, cs.nrZ() );
	    }
	}

	int sz0 = usedarr->info().getSize(0) * (resolution_+1);
	int sz1 = usedarr->info().getSize(1) * (resolution_+1);
	if ( texture_ )
	{
	    Array2DSlice<float> slice( *usedarr );
	    slice.setDimMap( 0, 1 );
	    slice.setDimMap( 1, 0 );
	    if ( !slice.init() )
		continue;

	    if ( resolution_ )
		texture_->setDataOversample( attrib, seriesidx_, resolution_, 
			textureInterpolationEnabled(), &slice, true );
	    else
	    {
		texture_->splitTexture( true );
    		texture_->setData( attrib, seriesidx_, &slice, true );
	    }
	}
	else
	{
	    //If the size is too big to display, use low resolution only
	    if ( sz0 > mMaxImageSize && resolution_ > 0 )
		sz0 = usedarr->info().getSize(0);
	    
	    if ( sz1 > mMaxImageSize && resolution_ > 0 )
		sz1 = usedarr->info().getSize(1);
			
	    mDeclareAndTryAlloc( float*, tarr, float[sz0*sz1] );
	    if ( !tarr )
	    {
	        if ( texture_ )
		    texture_->turnOn( false );
		else
		    channels_->turnOn( false );
		pErrMsg(
			"Insufficient memory; cannot display the 2D seismics.");
		return;
	    }
	    
	    if ( resolution_==0 )
		usedarr->getAll( tarr );
	    else
	    {
		// Copy all the data from usedarr to an Array2DImpl and pass 
		// this object to Array2DReSampler. This copy is done because 
		// Array2DReSampler will access the input using the "get" 
		// method. The get method of Array2DImpl is much faster than 
		// that of Seis2DArray.
		Array2DImpl<float> sourcearr2d( usedarr->info() );
		if ( !sourcearr2d.isOK() )
		{
		    if ( texture_ )
			texture_->turnOn( false );
		    else
			channels_->turnOn( false );
		    pErrMsg(
			"Insufficient memory; cannot display the 2D seismics.");
	            return;
		}
	    
		sourcearr2d.copyFrom( *usedarr );

		Array2DReSampler<float,float> 
			resampler( sourcearr2d, tarr, sz0, sz1, true );
		resampler.setInterpolate( true );
		resampler.execute();
	    }

	    channels_->setSize( 1, sz0, sz1 );
	    channels_->setUnMappedData(attrib, seriesidx_, tarr, 
			    OD::TakeOverPtr, tr);
	}
	    
	triangles_->setTextureZPixelsAndPathScale( sz1, resolution_+1 );
    }

    if ( texture_ )
	texture_->turnOn( true );
    else
	channels_->turnOn( true );
}


void Seis2DDisplay::updateVizPath()
{
    triangles_->setDisplayedGeometry( trcdisplayinfo_.rg, trcdisplayinfo_.zrg );
    if ( trcdisplayinfo_.size )
    	updateLineNamePos();
}


void Seis2DDisplay::updateLineNamePos()
{
    if ( !linename_ ) return;

    const visBase::Coordinates* coords = triangles_->getCoordinates();
    if ( !coords ) return;

    Coord3 pos = coords->getPos( 0 );
    linename_->setPosition( pos );
}


SurveyObject* Seis2DDisplay::duplicate( TaskRunner* tr ) const
{
    Seis2DDisplay* s2dd = create();
    s2dd->setGeometry( geometry_ );
    s2dd->setZRange( trcdisplayinfo_.zrg );
    s2dd->setTraceNrRange( trcdisplayinfo_.rg );
    s2dd->setResolution( getResolution(), tr );

    s2dd->setLineInfo( linesetid_, getLineName() );

    for ( int idx=0; idx<nrAttribs(); idx++ )
    {
	if ( idx )
	    s2dd->addAttrib();

	s2dd->setSelSpec( idx, *getSelSpec(idx) );
	if ( texture_ && s2dd->texture_ )
    	    s2dd->texture_->copySettingsFrom( idx,
    		    (const visBase::MultiTexture&) *texture_, idx );

	if ( getCache( idx ) )
	    s2dd->setData( idx, *getCache( idx ), tr );
    }

    return s2dd;
}


float Seis2DDisplay::calcDist( const Coord3& pos ) const
{
    Coord3 xytpos = scene_ ?
	scene_->getUTM2DisplayTransform()->transformBack( pos ) : pos;
    
    int trcidx; float mindist;
    getNearestTrace( xytpos, trcidx, mindist );
    if ( mindist<0 || mIsUdf(mindist) )
	return mUdf(float);

    StepInterval<float> zrg = getZRange( true );
    float zdif = 0;
    if ( !zrg.includes(xytpos.z) )
    {
	zdif = mMIN( fabs(xytpos.z-zrg.start), fabs(xytpos.z-zrg.stop) );
	const float zscale = scene_
	    ? scene_->getZScale() * scene_->getZStretch()
	    : SI().zScale();
	zdif *= zscale;
    }

    return Math::Sqrt( mindist + zdif*zdif );
}


void Seis2DDisplay::setDisplayTransformation( visBase::Transformation* tf )
{
    if ( transformation_ ) 
	transformation_->unRef();

    transformation_ = tf;
    transformation_->ref();
    
    triangles_->setDisplayTransformation( transformation_ );
    if ( linename_ )
	linename_->setDisplayTransformation( transformation_ );
}


visBase::Transformation* Seis2DDisplay::getDisplayTransformation()
{
    return transformation_;
}


void Seis2DDisplay::showLineName( bool yn )
{
    if ( !linename_ )
	return;

    linename_->turnOn( yn );
    updateLineNamePos();
}


bool Seis2DDisplay::lineNameShown() const
{ return linename_ ? linename_->isOn() : false; }


int Seis2DDisplay::nrResolutions() const
{ return 3; }


void Seis2DDisplay::setResolution( int res, TaskRunner* tr )
{
    if ( res==resolution_ )
	return;

    resolution_ = res;
    updateDataFromCache( tr );
}


int Seis2DDisplay::getResolution() const
{ return resolution_; }


SurveyObject::AttribFormat Seis2DDisplay::getAttributeFormat( int ) const
{ return SurveyObject::OtherFormat; }


void Seis2DDisplay::addCache()
{
    cache_ += 0;
    datapackids_ += -1;
}


void Seis2DDisplay::removeCache( int attrib )
{
    if ( cache_[attrib] ) cache_[attrib]->unRef();
    cache_.remove( attrib );

    DPM( DataPackMgr::FlatID() ).release( datapackids_[attrib] );
    datapackids_.remove( attrib );
}


void Seis2DDisplay::swapCache( int a0, int a1 )
{
    cache_.swap( a0, a1 );
    datapackids_.swap( a0, a1 );
}


void Seis2DDisplay::emptyCache( int attrib )
{
    if ( cache_[attrib] )
	cache_[attrib]->unRef();

    cache_.replace( attrib, 0 );
    datapackids_[attrib] = -1;
}


bool Seis2DDisplay::hasCache( int attrib ) const
{ return cache_[attrib] && cache_[attrib]->size(); }


void Seis2DDisplay::updateDataFromCache( TaskRunner* tr )
{
    for ( int idx=nrAttribs()-1; idx>=0; idx-- )
	if ( cache_[idx] ) setData( idx, *cache_[idx], tr );
}


void Seis2DDisplay::getMousePosInfo( const visBase::EventInfo& evinfo,
				     IOPar& par ) const
{
    par.setEmpty();
    par.set( sKey::XCoord, evinfo.worldpickedpos.x );
    par.set( sKey::YCoord, evinfo.worldpickedpos.y );
    par.set( sKey::LineKey, name() );

    int dataidx = -1;
    float mindist;
    if ( getNearestTrace(evinfo.worldpickedpos,dataidx,mindist) )
	par.set( sKey::TraceNr, geometry_.positions()[dataidx].nr_ );

}


void Seis2DDisplay::getMousePosInfo( const visBase::EventInfo&,
				     Coord3& pos, BufferString& val,
				     BufferString& info ) const
{
    getObjectInfo( info );
    getValueString( pos, val );

    int dataidx = -1;
    float mindist;
    if ( getNearestTrace(pos,dataidx,mindist) )
    {
	info += "   Tracenr: ";
	info += geometry_.positions()[dataidx].nr_;
    }
}


void Seis2DDisplay::getObjectInfo( BufferString& info ) const
{
    info = "Line: "; info += getLineName();
}


bool Seis2DDisplay::getCacheValue( int attrib, int version,
				    const Coord3& pos, float& res ) const
{
    if ( attrib>=cache_.size() || !cache_[attrib] )
	return false;

    int trcidx = -1;
    float mindist;
    if ( !getNearestTrace(pos, trcidx, mindist) )
	return false;

    const int trcnr = geometry_.positions()[trcidx].nr_;
    for ( int idx=0; idx<cache_[attrib]->trcinfoset_.size(); idx++ )
    {
	if ( cache_[attrib]->trcinfoset_[idx]->nr != trcnr )
	    continue;
	
	const DataHolder* dh = cache_[attrib]->dataset_[idx];
	const int sampidx = -dh->z0_ +
		mNINT(pos.z/cache_[attrib]->trcinfoset_[idx]->sampling.step);
	
	if ( sampidx>=0 && sampidx<dh->nrsamples_ )
	{
	    res = dh->series(dh->validSeriesIdx()[version])->value(sampidx);
	    return true;
	}
    }
	
    return false;
}


int Seis2DDisplay::getNearestTraceNr( const Coord3& pos ) const
{
    int trcidx = -1;
    float mindist;
    getNearestTrace( pos, trcidx, mindist );

    return  geometry_.positions()[trcidx].nr_;
}

Coord3 Seis2DDisplay::getNearestSubPos( const Coord3& pos,
					bool usemaxrange ) const
{
    int trcnr1st, trcnr2nd;
    float frac;
    if ( getNearestSegment(pos, usemaxrange, trcnr1st, trcnr2nd, frac) < 0.0 )
	return Coord3::udf();

    const Coord subpos = getCoord(trcnr1st)*(1-frac) + getCoord(trcnr2nd)*frac;
    const Interval<float> zrg = usemaxrange ? getMaxZRange(false) :
					      getZRange(false);
    return Coord3( subpos, zrg.limitValue(pos.z) );
}


float Seis2DDisplay::getNearestSegment( const Coord3& pos, bool usemaxrange,
					int& trcnr1st, int& trcnr2nd,
					float& frac ) const
{
    float mindist2 = MAXFLOAT;
    const Interval<int>& trcrg = usemaxrange ? getMaxTraceNrRange() :
					       getTraceNrRange();

    const TypeSet<PosInfo::Line2DPos>& posns = geometry_.positions();
    for ( int aidx=0; aidx<posns.size()-1; aidx++ )
    {
	const Coord posa = posns[aidx].coord_;
	if ( !posa.isDefined() || !trcrg.includes(posns[aidx].nr_) )
	    continue;

	Coord posb = Coord::udf();
	int bidx = aidx;

	while ( !posb.isDefined() && bidx<posns.size()-1 &&
		trcrg.includes(posns[bidx+1].nr_) )
	{
	    bidx++;
	    posb = posns[bidx].coord_;
	}

	if ( !posb.isDefined() )
	{
	    bidx = aidx;
	    posb = posa;
	}

	const float dist2a = posa.sqDistTo( pos );
	const float dist2b = posb.sqDistTo( pos );
	const float dist2c = posa.sqDistTo( posb );

	if ( dist2b >= dist2a+dist2c )
	{
	    if ( mindist2 > dist2a )
	    {
		mindist2 = dist2a;
		trcnr1st = posns[aidx].nr_;
		trcnr2nd = posns[bidx].nr_;
		frac = 0.0;
	    }
	    continue;
	}

	if ( dist2a >= dist2b+dist2c )
	{
	    if ( mindist2 > dist2b )
	    {
		mindist2 = dist2b;
		trcnr1st = posns[aidx].nr_;
		trcnr2nd = posns[bidx].nr_;
		frac = 1.0;
	    }
	    continue;
	}

	const float dista = sqrt( dist2a );
	const float distb = sqrt( dist2b );
	const float distc = sqrt( dist2c );
	const float sp = (dista + distb + distc) / 2;
	const float height2 = 4*sp*(sp-dista)*(sp-distb)*(sp-distc) / dist2c;

	if ( mindist2 > height2 )
	{
	    mindist2 = height2;
	    trcnr1st = posns[aidx].nr_;
	    trcnr2nd = posns[bidx].nr_;
	    frac = sqrt( dist2a - height2 ) / distc;
	}
    }
    return mindist2!=MAXFLOAT ? sqrt(mindist2) : -1.0;
}


void Seis2DDisplay::snapToTracePos( Coord3& pos ) const
{
    int trcidx = -1;
    float mindist;
    getNearestTrace( pos, trcidx, mindist );

    if ( trcidx<0 ) return;

    const Coord& crd = geometry_.positions()[trcidx].coord_;
    pos.x = crd.x; pos.y = crd.y; 
}


const MultiID& Seis2DDisplay::lineSetID() const
{ return linesetid_; }


bool Seis2DDisplay::getNearestTrace( const Coord3& pos,
				     int& trcidx, float& mindist ) const
{
    if ( geometry_.isEmpty() ) return false;

    const int nidx = geometry_.nearestIdx( pos, trcdisplayinfo_.rg );
    mindist = geometry_.positions()[nidx].coord_.distTo( pos );
    trcidx = nidx;
    return trcidx >= 0;
}


Coord Seis2DDisplay::getCoord( int trcnr ) const
{
    const int sz = geometry_.positions().size();
    if ( !sz )
	return Coord::udf();
    if ( prevtrcidx_ >= sz )
	prevtrcidx_ = sz-1;

    const int prevnr = geometry_.positions()[prevtrcidx_].nr_;
    int dir = (geometry_.positions()[0].nr_-prevnr)*(trcnr-prevnr)<0 ? 1 : -1;

    for ( int cnt=0; cnt<=1; cnt++ )
    {
	for ( int idx=prevtrcidx_+cnt*dir; idx>=0 && idx<sz; idx+=dir )
	{
	    if ( geometry_.positions()[idx].nr_ == trcnr )
	    {
		prevtrcidx_ = idx;
		return geometry_.positions()[idx].coord_;
	    }
	}
	dir = -dir;
    }
    return Coord::udf();
}


Coord Seis2DDisplay::getNormal( int trcnr ) const
{
    return geometry_.getNormal( trcnr );
}


bool Seis2DDisplay::setZAxisTransform( ZAxisTransform* zat, TaskRunner* tr )
{
    const bool haddatatransform = datatransform_;
    if ( datatransform_ )
    {
	if ( voiidx_!=-1 )
	    datatransform_->removeVolumeOfInterest(voiidx_);
	if ( datatransform_->changeNotifier() )
	    datatransform_->changeNotifier()->remove(
		    mCB(this,Seis2DDisplay,dataTransformCB) );
	datatransform_->unRef();
	datatransform_ = 0;
    }

    datatransform_ = zat;
    voiidx_ = -1;

    if ( datatransform_ )
    {
	datatransform_->ref();
	updateRanges( false, !haddatatransform );
	if ( datatransform_->changeNotifier() )
	    datatransform_->changeNotifier()->notify(
		    mCB(this,Seis2DDisplay,dataTransformCB) );
    }

    return true;
}


const ZAxisTransform* Seis2DDisplay::getZAxisTransform() const
{ return datatransform_; }


void Seis2DDisplay::dataTransformCB( CallBacker* )
{
    updateRanges( false, true );
    updateDataFromCache( 0 );
}


void Seis2DDisplay::updateRanges( bool updatetrc, bool updatez )
{
    // TODO: handle update trcrg
    if ( updatez && datatransform_ )
	setZRange( datatransform_->getZInterval(false) );
}


void Seis2DDisplay::clearTexture( int attribnr )
{
    channels_->setNrVersions( attribnr, 1 );
    channels_->setUnMappedData( attribnr, 0, 0, OD::UsePtr, 0 );
    channels_->turnOn( false );

    Attrib::SelSpec as;
    as.set2DFlag(true);
    setSelSpec( attribnr, as );
}


void Seis2DDisplay::setAnnotColor( Color col )
{
    linename_->getMaterial()->setColor( col );
}


Color Seis2DDisplay::getAnnotColor()
{
    return linename_->getMaterial()->getColor();
}


Seis2DDisplay* Seis2DDisplay::getSeis2DDisplay( const MultiID& lineset,
						const char* linenm )
{
    TypeSet<int> ids;
    visBase::DM().getIds( typeid(visSurvey::Seis2DDisplay), ids );

    for ( int idx=0; idx<ids.size(); idx++ )
    {
	DataObject* dataobj = visBase::DM().getObject( ids[idx] );
	mDynamicCastGet( Seis2DDisplay*, s2dd, dataobj );
	if (s2dd && lineset==s2dd->lineSetID() &&
	    !strcmp(linenm,s2dd->getLineName()) )
	    return s2dd;
    }

    return 0;
}


void Seis2DDisplay::fillPar( IOPar& par, TypeSet<int>& saveids ) const
{
    visSurvey::MultiTextureSurveyObject::fillPar( par, saveids );

    par.set( "GeomID", geomid_.toString() );
    par.set( sKeyLineSetID(), linesetid_ );
    par.setYN( sKeyShowLineName(), lineNameShown() );
    par.set( sKeyTrcNrRange(), trcdisplayinfo_.rg );
    par.set( sKeyZRange(), trcdisplayinfo_.zrg );
}


int Seis2DDisplay::usePar( const IOPar& par )
{
    int textureid = -1;
    if ( par.get(sKeyTextureID(),textureid) )
    {
	int res =  visBase::VisualObjectImpl::usePar( par );
	if ( res != 1 ) return res;

	DataObject* text = visBase::DM().getObject( textureid );
	if ( !text ) return 0;
	if ( typeid(*text)!=typeid(visBase::Texture2) ) return -1;

	RefMan<visBase::Texture2> texture =
	    reinterpret_cast<visBase::Texture2*>(text);
	if ( texture_ )
    	    texture_->setColorTab( 0, texture->getColorTab() );
	else
	{
	    channels_->setColTabMapperSetup( 0,
		    texture->getColorTab().colorMapper().setup_ );
	    channels_->getChannels2RGBA()->setSequence( 0,
		    texture->getColorTab().colorSeq().colors() );
	}

	Attrib::SelSpec as;
	as.usePar( par );
	setSelSpec( 0, as );
    }
    else //new format
    {
	int res =  visSurvey::MultiTextureSurveyObject::usePar( par );
	if ( res!=1 ) return res;

	par.get( sKeyTrcNrRange(), trcdisplayinfo_.rg );
	
	bool showlinename = false;
	par.getYN( sKeyShowLineName(), showlinename );
	showLineName( showlinename );
    }

    par.get( sKeyZRange(), trcdisplayinfo_.zrg );

    BufferString linename( name() );
    par.get( sKeyLineSetID(), linesetid_ );
    BufferString geomidstr;
    if ( par.get("GeomID",geomidstr) )
    {
	geomid_.fromString( geomidstr.buf() );
	PtrMan<IOObj> seis2dobj = IOM().get( linesetid_ );
	if ( !seis2dobj )
	    return -1;
	S2DPOS().setCurLineSet( seis2dobj->name() );
	linename = S2DPOS().getLineName( geomid_.lineid_ );
    }
    
    setName( linename );
    if ( linename_ )
    {
	if ( scene_ )
	    setAnnotColor( scene_->getAnnotColor() );
	linename_->setText( linename.buf() );
    }

    return 1;
}

}; // namespace visSurvey
