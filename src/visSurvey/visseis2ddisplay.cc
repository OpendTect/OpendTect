/*+
 ________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          August 2004
 ________________________________________________________________________

-*/

static const char* rcsID mUsedVar = "$Id$";

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
#include "survgeom.h"
#include "survinfo.h"
#include "task.h"
#include "zaxistransform.h"

//For parsing old pars
#include "attribsel.h"
#include "vistexture2.h"

#define mMaxImageSize 300000000	// 32767

mCreateFactoryEntry( visSurvey::Seis2DDisplay );

using namespace Attrib;

namespace visSurvey
{

const char* Seis2DDisplay::sKeyLineSetID()	{ return "LineSet ID"; }
const char* Seis2DDisplay::sKeyTrcNrRange()	{ return "Trc Nr Range"; }
const char* Seis2DDisplay::sKeyZRange()		{ return "Z Range"; }
const char* Seis2DDisplay::sKeyShowLineName()	{ return "Show linename"; }
const char* Seis2DDisplay::sKeyTextureID()	{ return "Texture ID"; }

Seis2DDisplay::Seis2DDisplay()
    : MultiTextureSurveyObject( true )
    , transformation_(0)
    , geometry_(*new PosInfo::Line2DData)
    , triangles_( visBase::SplitTextureSeis2D::create() )	 
    , geomchanged_(this)
    , maxtrcnrrg_(INT_MAX,INT_MIN,1)
    , datatransform_(0)
    , voiidx_(-1)
    , prevtrcidx_(0)
{
    geometry_.setZRange( StepInterval<float>(mUdf(float),mUdf(float),1) );
    cache_.allowNull();

    linename_ = visBase::Text2::create();
    linename_->ref();
    insertChild( childIndex(channels_->getInventorNode()),
	linename_->getInventorNode() );
    
    triangles_->ref();
    triangles_->removeSwitch();
    addChild( triangles_->getInventorNode() );
    
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

#ifdef mNew2DGeometryImpl
    geomid_ = Survey::GM().getGeomID( lnm );
    if ( geomid_ < 0 )
	geomid_ = Survey::GM().getGeomID( Survey::Geometry2D::makeUniqueLineName
					(seis2dobj->name(),lnm) );
#else
    oldgeomid_ = S2DPOS().getGeomID( seis2dobj->name(), lnm );
#endif
    setName( lnm );
    if ( linename_ )
    {
	if ( scene_ )
	    setAnnotColor( scene_->getAnnotColor() );
	linename_->setText( lnm );
    }
}


const char* Seis2DDisplay::getLineName() const
{
#ifdef mNew2DGeometryImpl
    return Survey::GM().getName( geomid_ );
#else
    if ( !oldgeomid_.isOK() )
	return name();

    S2DPOS().setCurLineSet( oldgeomid_.lsid_ );
    return S2DPOS().getLineName( oldgeomid_.lineid_ );
#endif
}


PosInfo::GeomID Seis2DDisplay::getGeomID() const
{ return oldgeomid_; }


int Seis2DDisplay::geomID() const
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
    const bool alreadytransformed = alreadyTransformed( attrib );
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
    trcdisplayinfo_.rg.start = -1;
    trcdisplayinfo_.rg.stop = -1;

    if ( maxtrcnrrg_.isRev() )
    {
	pErrMsg("Geometry not set");
	return;
    }

    const Interval<int> rg( maxtrcnrrg_.limitValue(trcrg.start),
			    maxtrcnrrg_.limitValue(trcrg.stop) );

    if ( !rg.width() )
	return;

    for ( int idx=0; idx<trcdisplayinfo_.alltrcnrs.size(); idx++ )
    {
	if ( trcdisplayinfo_.alltrcnrs[idx]>=rg.start )
	{
	    trcdisplayinfo_.rg.start = idx;
	    break;
	}
    }

    for ( int idx=mMIN(trcdisplayinfo_.alltrcnrs.size(),mMaxImageSize)-1;
	  idx>=0; idx-- )
    {
	if ( trcdisplayinfo_.alltrcnrs[idx]<=rg.stop )
	{
	    trcdisplayinfo_.rg.stop = idx;
	    break;
	}
    }

    if ( trcdisplayinfo_.rg.stop<0 || trcdisplayinfo_.rg.start<0 )
	mRetErrGeo;

    trcdisplayinfo_.size = trcdisplayinfo_.rg.width(false) + 1;
    updateVizPath();
}


Interval<int> Seis2DDisplay::getTraceNrRange() const
{
    if ( !trcdisplayinfo_.alltrcnrs.validIdx( trcdisplayinfo_.rg.start ) ||
	 !trcdisplayinfo_.alltrcnrs.validIdx( trcdisplayinfo_.rg.stop ) )
	return Interval<int>( mUdf(int), mUdf(int) );

    return Interval<int>( trcdisplayinfo_.alltrcnrs[trcdisplayinfo_.rg.start],
		          trcdisplayinfo_.alltrcnrs[trcdisplayinfo_.rg.stop]);
}


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

    setTraceData( attrib, *dp2d->dataarray(), tr );

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
				  const Attrib::Data2DArray& dataset,
				  TaskRunner* tr )
{
    setData( attrib, dataset, tr );
    if ( cache_[attrib] ) cache_[attrib]->unRef();

    cache_.replace( attrib, &dataset );
    cache_[attrib]->ref();
}


const Attrib::Data2DArray* Seis2DDisplay::getCache( int attrib ) const
{ return cache_[attrib]; }


void Seis2DDisplay::setData( int attrib,
			     const Attrib::Data2DArray& data2dh,
       			     TaskRunner* tr )
{
    if ( data2dh.isEmpty() ) 
    {
	channels_->setUnMappedVSData( attrib, 0, 0, OD::UsePtr, tr );
	channels_->turnOn( false );
	return;
    }

    const SamplingData<float>& sd = data2dh.trcinfoset_[0]->sampling;
    const int nrseries = data2dh.dataset_->info().getSize( 0 );

    channels_->setNrVersions( attrib, nrseries );

    Array2DSlice<float> slice2d( *data2dh.dataset_ );
    slice2d.setDimMap( 0, 1 );
    slice2d.setDimMap( 1, 2 );

    int sz0=mUdf(int), sz1=mUdf(int);
    MouseCursorChanger cursorlock( MouseCursor::Wait );

    for ( int seriesidx=0; seriesidx<nrseries; seriesidx++ )
    {
	slice2d.setPos( 0, seriesidx );
	slice2d.init();

	const bool alreadytransformed = alreadyTransformed( attrib );
	PtrMan<Array2D<float> > tmparr = 0;
	Array2D<float>* usedarr = 0;
	if ( alreadytransformed || !datatransform_ )
	{
	    const int nrsamples = slice2d.info().getSize(1);
	    const int nrdisplaytraces = trcdisplayinfo_.rg.width()+1;
	    const int nrdisplaysamples = trcdisplayinfo_.zrg.nrSteps()+1;
	    if ( slice2d.info().getSize(0)==nrdisplaytraces && 
		 nrsamples==nrdisplaysamples )
	    {
		usedarr = &slice2d;
	    }
	    else
	    {
		mTryAlloc( tmparr, 
		    Array2DImpl<float>( nrdisplaytraces, nrdisplaysamples) );
		usedarr = tmparr;
		const int startidx = trcdisplayinfo_.rg.start;
		float* sampleptr = tmparr->getData();
		const bool canuseptr = sampleptr;
		for ( int crlidx=0; crlidx<trcdisplayinfo_.size; crlidx++ )
		{
		    const int trcnr =
			trcdisplayinfo_.alltrcnrs[crlidx+startidx];
		    const int trcidx =  data2dh.indexOf( trcnr );
		    const float* trcptr = slice2d.getData();
		    const ValueSeries<float>* stor = slice2d.getStorage();
		    od_int64 offset = slice2d.info().getOffset( trcidx, 0 );

		    if ( trcptr ) trcptr += offset;
		    OffsetValueSeries<float> trcstor( *stor, offset );

		    for ( int zidx=0; zidx<nrdisplaysamples; zidx++,sampleptr++)
		    {
			if ( trcidx==-1 )
			{
			    if ( canuseptr )
				*sampleptr = mUdf(float);
			    else
				tmparr->set( crlidx, zidx, mUdf(float) );
			    continue;
			}

			const float z = trcdisplayinfo_.zrg.atIndex( zidx );
			const float sample = sd.getfIndex( z );
			float val = mUdf(float);
			if ( trcptr )
			    IdxAble::interpolateReg( trcptr, nrsamples, sample,
						     val, false );
			else
			    IdxAble::interpolateReg( trcstor, nrsamples, sample,
						     val, false );
			if ( canuseptr )
			    *sampleptr = val;
			else
			    tmparr->set( crlidx, zidx, val );
		    }
		}
	    }
	}
	else
	{
	    CubeSampling cs;
	    cs.hrg.start.inl = cs.hrg.stop.inl = 0;
	    cs.hrg.start.crl =
		trcdisplayinfo_.alltrcnrs[trcdisplayinfo_.rg.start];
	    cs.hrg.stop.crl =
		trcdisplayinfo_.alltrcnrs[trcdisplayinfo_.rg.stop];
	    cs.hrg.step.crl = 1;
	    assign( cs.zrg, trcdisplayinfo_.zrg );
	    // use survey step here?
	    if ( voiidx_ < 0 )
		voiidx_ = datatransform_->addVolumeOfInterest2D(
						getLineName(), cs, true );
	    else
		datatransform_->setVolumeOfInterest2D( voiidx_, getLineName(),
						     cs, true );
	    datatransform_->loadDataIfMissing( voiidx_ );

	    ZAxisTransformSampler outpsampler( *datatransform_, true,
		SamplingData<double>(cs.zrg.start,cs.zrg.step), true );
	    outpsampler.setLineName( getLineName() );

	    const int zsz = cs.nrZ();
	    mTryAlloc( tmparr, Array2DImpl<float>(trcdisplayinfo_.size,zsz) );
	    usedarr = tmparr;

	    for ( int crlidx=0; crlidx<trcdisplayinfo_.size; crlidx++ )
	    {
	        Array1DSlice<float> traceslice( slice2d );    
		traceslice.setDimMap( 0, 1 );
		traceslice.setPos( 0, crlidx );
		if ( !traceslice.init() )
		{
		    pErrMsg( "Error reading array for Z-axis transformation." );
		    continue; 
		}

		const int startidx = trcdisplayinfo_.rg.start + crlidx;
		outpsampler.setTrcNr( trcdisplayinfo_.alltrcnrs[startidx] );
		outpsampler.computeCache( Interval<int>(0,zsz-1) );

		SampledFunctionImpl<float,ValueSeries<float> >
		    inputfunc( traceslice, slice2d.info().getSize(1), 
			    sd.start, sd.step );
		inputfunc.setHasUdfs( true );
		inputfunc.setInterpolate( textureInterpolationEnabled() );

		float* outputptr = tmparr->getData() +
				   tmparr->info().getOffset( crlidx, 0 );	
		reSample( inputfunc, outpsampler, outputptr, zsz );
	    }
	}

	if ( !seriesidx )
	{
	    sz0 = usedarr->info().getSize(0) * (resolution_+1);
	    sz1 = usedarr->info().getSize(1) * (resolution_+1);

	    //If the size is too big to display, use low resolution only
	    if ( sz0 > mMaxImageSize && resolution_ > 0 )
		sz0 = usedarr->info().getSize(0);
	    
	    if ( sz1 > mMaxImageSize && resolution_ > 0 )
		sz1 = usedarr->info().getSize(1);
	}

	ValueSeries<float>* stor =
	    !resolution_ && !tmparr ? usedarr->getStorage() : 0;
	bool ownsstor = false;

	//We are only interested in the global, permanent storage
	if ( stor && stor!=data2dh.dataset_->getStorage() )
	    stor = 0; 

	if ( !stor )
	{
	    stor = new MultiArrayValueSeries<float,float>(sz0*sz1);
	    ownsstor = true;
	}

	if ( !stor || !stor->isOK() )
	{
	    channels_->turnOn( false );
	    pErrMsg(
		    "Insufficient memory; cannot display the 2D seismics.");
	    if ( ownsstor ) delete stor;
	    return;
	}
	
	if ( ownsstor )
	{
	    if ( resolution_==0 )
		usedarr->getAll( *stor );
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
		    channels_->turnOn( false );
		    pErrMsg(
			"Insufficient memory; cannot display the 2D seismics.");
		    return;
		}
	    
		sourcearr2d.copyFrom( *usedarr );

		Array2DReSampler<float,float> 
			resampler( sourcearr2d, *stor, sz0, sz1, true );
		resampler.setInterpolate( true );
		TaskRunner::execute( tr, resampler );
	    }
	}

	channels_->setSize( 1, sz0, sz1 );
	channels_->setUnMappedVSData(attrib, seriesidx, stor, 
			ownsstor ? OD::TakeOverPtr : OD::UsePtr, tr);
    }

    triangles_->setTextureZPixelsAndPathScale( sz1, resolution_+1 );
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
    s2dd->setTraceNrRange( getTraceNrRange() );
    s2dd->setResolution( getResolution(), tr );
    s2dd->setLineInfo( linesetid_, getLineName() );

    for ( int idx=0; idx<nrAttribs(); idx++ )
    {
	if ( idx )
	    s2dd->addAttrib();

	s2dd->setSelSpec( idx, *getSelSpec(idx) );

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
    if ( !zrg.includes(xytpos.z,false) )
    {
	zdif = (float) mMIN(fabs(xytpos.z-zrg.start), fabs(xytpos.z-zrg.stop));
	const float zscale = scene_
	    ? scene_->getZScale() * scene_->getZStretch()
	    : SI().zScale();
	zdif *= zscale;
    }

    return Math::Sqrt( mindist + zdif*zdif );
}


void Seis2DDisplay::setDisplayTransformation( const mVisTrans* tf )
{
    if ( transformation_ ) 
	transformation_->unRef();

    transformation_ = tf;
    transformation_->ref();
    
    triangles_->setDisplayTransformation( transformation_ );
    if ( linename_ )
	linename_->setDisplayTransformation( transformation_ );
}


const mVisTrans* Seis2DDisplay::getDisplayTransformation() const
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
    cache_.removeSingle( attrib );

    DPM( DataPackMgr::FlatID() ).release( datapackids_[attrib] );
    datapackids_.removeSingle( attrib );
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
{ return cache_[attrib] && cache_[attrib]->nrTraces(); }


void Seis2DDisplay::updateDataFromCache( TaskRunner* tr )
{
    for ( int idx=nrAttribs()-1; idx>=0; idx-- )
	if ( cache_[idx] ) setData( idx, *cache_[idx], tr );
}


void Seis2DDisplay::getMousePosInfo( const visBase::EventInfo& evinfo,
				     IOPar& par ) const
{
    par.setEmpty();
    par.set( sKey::XCoord(), evinfo.worldpickedpos.x );
    par.set( sKey::YCoord(), evinfo.worldpickedpos.y );
    par.set( sKey::LineKey(), name() );

    int dataidx = -1;
    float mindist;
    if ( getNearestTrace(evinfo.worldpickedpos,dataidx,mindist) )
	par.set( sKey::TraceNr(), geometry_.positions()[dataidx].nr_ );

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
	info.add( " [" ).add( geometry_.positions()[dataidx].nr_ ).add( "]" );
}


void Seis2DDisplay::getObjectInfo( BufferString& info ) const
{
    info = "Line: "; info.add( getLineName() );
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
	
	const int sampidx = cache_[attrib]->trcinfoset_[idx]->sampling.nearestIndex( pos.z );
	if ( cache_[attrib]->dataset_->info().validPos( version, idx, sampidx ) )
	{
	    res = cache_[attrib]->dataset_->get( version, idx, sampidx );

	    //TODO: validSeriesIdx ??
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
    Interval<int> trcrg = getTraceNrRange();
    if ( usemaxrange ) trcrg = getMaxTraceNrRange();

    const TypeSet<PosInfo::Line2DPos>& posns = geometry_.positions();
    for ( int aidx=0; aidx<posns.size()-1; aidx++ )
    {
	const Coord posa = posns[aidx].coord_;
	if ( !posa.isDefined() || !trcrg.includes(posns[aidx].nr_,false) )
	    continue;

	Coord posb = Coord::udf();
	int bidx = aidx;

	while ( !posb.isDefined() && bidx<posns.size()-1 &&
		trcrg.includes(posns[bidx+1].nr_,false) )
	{
	    bidx++;
	    posb = posns[bidx].coord_;
	}

	if ( !posb.isDefined() )
	{
	    bidx = aidx;
	    posb = posa;
	}

	const float dist2a = (float) posa.sqDistTo( pos );
	const float dist2b = (float) posb.sqDistTo( pos );
	const float dist2c = (float) posa.sqDistTo( posb );

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

	const float dista = Math::Sqrt( dist2a );
	const float distb = Math::Sqrt( dist2b );
	const float distc = Math::Sqrt( dist2c );
	const float sp = (dista + distb + distc) / 2;
	const float height2 = 4*sp*(sp-dista)*(sp-distb)*(sp-distc) / dist2c;

	if ( mindist2 > height2 )
	{
	    mindist2 = height2;
	    trcnr1st = posns[aidx].nr_;
	    trcnr2nd = posns[bidx].nr_;
	    frac = Math::Sqrt( dist2a - height2 ) / distc;
	}
    }
    return mindist2!=MAXFLOAT ? Math::Sqrt(mindist2) : -1.0f;
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

MultiID Seis2DDisplay::getMultiID() const
{ return linesetid_; }


bool Seis2DDisplay::getNearestTrace( const Coord3& pos,
				     int& trcidx, float& mindist ) const
{
    if ( geometry_.isEmpty() ) return false;

    const Interval<int> trcnrrg(
	    trcdisplayinfo_.alltrcnrs[trcdisplayinfo_.rg.start],
	    trcdisplayinfo_.alltrcnrs[trcdisplayinfo_.rg.stop]);
    const int nidx = geometry_.nearestIdx( pos, trcnrrg );
    mindist = (float) geometry_.positions()[nidx].coord_.distTo( pos );
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
    channels_->setUnMappedVSData( attribnr, 0, 0, OD::UsePtr, 0 );
    channels_->turnOn( false );

    Attrib::SelSpec as;
    as.set2DFlag(true);
    setSelSpec( attribnr, as );
}


void Seis2DDisplay::setAnnotColor( Color col )
{
    linename_->getMaterial()->setColor( col );
}


Color Seis2DDisplay::getAnnotColor() const
{
    return linename_->getMaterial()->getColor();
}


Seis2DDisplay* Seis2DDisplay::getSeis2DDisplay( const MultiID& lineset,
						const char* linenmptr )
{
    FixedString linenm = linenmptr;
    TypeSet<int> ids;
    visBase::DM().getIds( typeid(visSurvey::Seis2DDisplay), ids );

    for ( int idx=0; idx<ids.size(); idx++ )
    {
	DataObject* dataobj = visBase::DM().getObject( ids[idx] );
	mDynamicCastGet( Seis2DDisplay*, s2dd, dataobj );
	if (s2dd && lineset==s2dd->lineSetID() && linenm &&
	    linenm==s2dd->getLineName() )
	    return s2dd;
    }

    return 0;
}


void Seis2DDisplay::fillPar( IOPar& par, TypeSet<int>& saveids ) const
{
    visSurvey::MultiTextureSurveyObject::fillPar( par, saveids );

#ifdef mNew2DGeometryImpl
    par.set( "GeomID", geomid_ );
#else
    par.set( "GeomID", oldgeomid_.toString() );
    par.set( sKeyLineSetID(), linesetid_ );
#endif
    par.setYN( sKeyShowLineName(), lineNameShown() );
    const Interval<int> trcnrrg(
	    trcdisplayinfo_.alltrcnrs[trcdisplayinfo_.rg.start],
	    trcdisplayinfo_.alltrcnrs[trcdisplayinfo_.rg.stop]);
    par.set( sKeyTrcNrRange(), trcnrrg );
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

	channels_->setColTabMapperSetup( 0,
	    texture->getColorTab().colorMapper().setup_ );
	channels_->getChannels2RGBA()->setSequence( 0,
	    texture->getColorTab().colorSeq().colors() );

	Attrib::SelSpec as;
	as.usePar( par );
	setSelSpec( 0, as );
    }
    else //new format
    {
	int res =  visSurvey::MultiTextureSurveyObject::usePar( par );
	if ( res!=1 ) return res;

	Interval<int> trcnrrg;
	if ( par.get( sKeyTrcNrRange(), trcnrrg ) )
	{
	    for ( int idx=0; idx<trcdisplayinfo_.alltrcnrs.size(); idx++ )
	    {
		if ( trcdisplayinfo_.alltrcnrs[idx]>=trcnrrg.start )
		{
		    trcdisplayinfo_.rg.start = idx;
		    break;
		}
	    }

	    for ( int idx=trcdisplayinfo_.alltrcnrs.size()-1; idx>=0; idx-- )
	    {
		if ( trcdisplayinfo_.alltrcnrs[idx]<=trcnrrg.start )
		{
		    trcdisplayinfo_.rg.stop = idx;
		    break;
		}
	    }
	}

	bool showlinename = false;
	par.getYN( sKeyShowLineName(), showlinename );
	showLineName( showlinename );
    }

    par.get( sKeyZRange(), trcdisplayinfo_.zrg );

    BufferString linename( name() );
    if ( !par.get(sKeyLineSetID(), linesetid_) )
    {
	if ( par.get("GeomID",geomid_) )
	{
	    if ( geomid_ < 0 )
		return -1;

	    linename = Survey::GM().getName( geomid_ );
	}
    }
    else
    {
	BufferString geomidstr;
	if ( par.get("GeomID",geomidstr) )
	{
	    oldgeomid_.fromString( geomidstr.buf() );
	    PtrMan<IOObj> seis2dobj = IOM().get( linesetid_ );
	    if ( !seis2dobj )
		return -1;
	    S2DPOS().setCurLineSet( seis2dobj->name() );
	    linename = S2DPOS().getLineName( oldgeomid_.lineid_ );
	}
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
