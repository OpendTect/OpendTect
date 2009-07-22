/*+
 ________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          August 2004
 ________________________________________________________________________

-*/

static const char* rcsID = "$Id: visseis2ddisplay.cc,v 1.73 2009-07-22 21:49:59 cvsnanne Exp $";

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

#include "arrayndimpl.h"
#include "arrayndslice.h"
#include "attribdataholder.h"
#include "attribdatapack.h"
#include "coltabmapper.h"
#include "genericnumer.h"
#include "iopar.h"
#include "keystrs.h"
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
			      Array2DImpl<float>& array )
    : data2dh_( dh )
    , arr_( array )	
    , valseridx_( dh.dataset_[0]->validSeriesIdx()[seriesid] )
    , s2d_( s2d )			  
{
    const int trnrsz = s2d_.geometry_.posns_.size();
    for ( int idx=0; idx<trnrsz; idx++ )
 	trcnrs_ += s2d_.geometry_.posns_[idx].nr_;
    
    quickSort( trcnrs_.arr() , trnrsz );
}

      	
od_int64 nrIterations() const { return data2dh_.size(); }

private:
bool doWork( od_int64 start, od_int64 stop, int threadid )
{
    const int nrsamp = data2dh_.dataset_[0]->nrsamples_;
   
    const SamplingData<float>& sd = data2dh_.trcinfoset_[0]->sampling; 
    Interval<float> zrg = s2d_.getZRange(!s2d_.datatransform_);
    const int arrzsz = arr_.info().getSize(1);
    StepInterval<int> arraysrg( mNINT(zrg.start/sd.step),
				mNINT(zrg.stop/sd.step),1 );

    const float firstz = data2dh_.dataset_[0]->z0_*sd.step;
    const int firstdhsample = sd.nearestIndex( firstz );
    const bool samplebased = 
	mIsEqual( sd.getIndex(firstz),firstdhsample,1e-3 ) && 
	mIsEqual( sd.step, s2d_.geometry_.zrg_.step, 1e-3 );

    if ( !samplebased )
    {
	pErrMsg("Not impl");
	return false;
    }

    const int trcsz= trcnrs_.size();
    for ( int idx=start; idx<=stop && shouldContinue(); idx++ )
    {
	const int trcnr = data2dh_.trcinfoset_[idx]->nr;
	if ( !s2d_.trcnrrg_.includes(trcnr) )
	{
	    addToNrDone( 1 );
	    continue;
	}

	int trcidx = 0;
	while ( trcnr>trcnrs_[trcidx] && trcidx<trcsz-1 )
  	    trcidx++;

	const DataHolder* dh = data2dh_.dataset_[idx];
	if ( !dh )
	{
	    addToNrDone( 1 );
	    continue;
	}
	
	const ValueSeries<float>* dataseries = dh->series( valseridx_ );
	for ( int idy=0; idy<nrsamp; idy++ )
	{
	    const int smp = firstdhsample+idy;
	    const float val = dh->dataPresent(smp)
		? dataseries->value( smp-dh->z0_ )
		: mUdf(float);
	    const int arrzidx = arraysrg.getIndex( smp );
	    if ( arrzidx<0 || arrzidx>=arrzsz ) 
	    {
		addToNrDone( 1 );
		continue;
	    }
	    
	    arr_.set( trcidx, arrzidx, val );
	}

	addToNrDone( 1 );
    }

    return true;
}
   
    Array2DImpl<float>&			arr_;
    const Attrib::Data2DHolder&		data2dh_;
    const Seis2DDisplay&		s2d_;
    const int				valseridx_;
    TypeSet<int>			trcnrs_;
};


Seis2DDisplay::Seis2DDisplay()
    : MultiTextureSurveyObject( true )
    , transformation_(0)
    , geometry_(*new PosInfo::Line2DData)
    , triangles_( visBase::SplitTextureSeis2D::create() )	 
    , geomchanged_(this)
    , maxtrcnrrg_(INT_MAX,INT_MIN)
    , trcnrrg_(-1,-1)
    , datatransform_(0)
    , voiidx_(-1)
    , prevtrcidx_(0)
{
    geometry_.zrg_.start = geometry_.zrg_.stop = mUdf(float);
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
    setDataTransform( 0 );
}


void Seis2DDisplay::setLineName( const char* lnm )
{
    setName( lnm );
    if ( linename_ )
	linename_->setText( lnm );
}


const char* Seis2DDisplay::getLineName() const
{ return name(); }


void Seis2DDisplay::setGeometry( const PosInfo::Line2DData& geometry )
{
    geometry_ = geometry;
    const TypeSet<PosInfo::Line2DPos>& linepositions = geometry.posns_;
    maxtrcnrrg_.set( INT_MAX, INT_MIN );

    for ( int idx=linepositions.size()-1; idx>=0; idx-- )
	maxtrcnrrg_.include( linepositions[idx].nr_, false );

    triangles_->setPath( linepositions );

    setTraceNrRange( maxtrcnrrg_ );
    setZRange( geometry_.zrg_ );
    updateRanges( false, true );

    geomchanged_.trigger();
}


StepInterval<float> Seis2DDisplay::getMaxZRange( bool displayspace ) const
{
    if ( !datatransform_ || displayspace )
	return geometry_.zrg_;

    StepInterval<float> zrg;
    zrg.setFrom( datatransform_->getZInterval(false) );
    zrg.step = geometry_.zrg_.step;
    return zrg;
}


void Seis2DDisplay::setZRange( const Interval<float>& nzrg )
{
    if ( mIsUdf(geometry_.zrg_.start) )
	return;

    const StepInterval<float> maxzrg = getMaxZRange( false );
    const Interval<float> zrg( mMAX(maxzrg.start,nzrg.start),
			       mMIN(maxzrg.stop,nzrg.stop) );
    if ( curzrg_.isEqual(zrg,mDefEps) )
	return;

    curzrg_.setFrom( zrg );
    updateVizPath();
    geomchanged_.trigger();
}


Interval<float> Seis2DDisplay::getZRange( bool displayspace ) const
{
    if ( datatransform_ && !displayspace )
	return datatransform_->getZInterval( true );
    return curzrg_;
}


const Interval<int> Seis2DDisplay::getSampleRange() const
{
    StepInterval<float> maxzrg = getMaxZRange( true );
    Interval<int> samplerg( maxzrg.nearestIndex(curzrg_.start),
	    		    maxzrg.nearestIndex(curzrg_.stop) );
    return samplerg;
}


void Seis2DDisplay::setTraceNrRange( const Interval<int>& trcrg )
{
    if ( maxtrcnrrg_.isRev() )
    {
	pErrMsg("Geometry not set");
	return;
    }

    const Interval<int> rg( maxtrcnrrg_.limitValue(trcrg.start),
			    maxtrcnrrg_.limitValue(trcrg.stop) );

    if ( !rg.width() || trcnrrg_==rg )
	return;

    trcnrrg_.start = rg.start;
    trcnrrg_.stop = rg.stop;
    updateVizPath();
}


const Interval<int>& Seis2DDisplay::getTraceNrRange() const
{ return trcnrrg_; }


const Interval<int>& Seis2DDisplay::getMaxTraceNrRange() const
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
	if ( texture_ )
	{
	    texture_->setData( attrib, 0, 0 );
	    texture_->turnOn( false );
	}
	else
	{
	    channels_->setUnMappedData( attrib, 0, 0, OD::UsePtr, tr );
	    channels_->turnOn( false );
	}
	return;
    }

    const SamplingData<float>& sd = data2dh.trcinfoset_[0]->sampling;

    StepInterval<float> arrayzrg;
    arrayzrg.setFrom( getZRange(!datatransform_) );
    arrayzrg.step = sd.step;
    const int arrzsz = arrayzrg.nrSteps()+1;
    StepInterval<int> arraysrg( mNINT(arrayzrg.start/sd.step),
	    			mNINT(arrayzrg.stop/sd.step), 1 );

    mDeclareAndTryAlloc( PtrMan<Array2DImpl<float> >, arr,
	    Array2DImpl<float>( geometry_.posns_.size(), arrzsz ) );
    if ( !arr->isOK() )
	return;

    const DataHolder* firstdh = data2dh.dataset_[0];
    TypeSet<int> valididxs = firstdh->validSeriesIdx();
    const int nrseries = valididxs.size();
    if ( nrseries == 0 )
	return;

    if ( texture_ )
	texture_->setNrVersions( attrib, nrseries );
    else
	channels_->setNrVersions( attrib, nrseries );

    for ( int sidx=0; sidx<nrseries; sidx++ )
    {
	arr->setAll( mUdf(float) );

	Seis2DTextureDataArrayFiller arrayfiller( *this, data2dh, sidx, *arr );
	if ( !arrayfiller.execute() )
	    continue;

	PtrMan<Array2D<float> > tmparr = 0;
	Array2D<float>* usedarr = 0;
	const char* zdomain = getSelSpec(attrib)->zDomainKey();
	const bool alreadytransformed = zdomain && *zdomain;
	if ( alreadytransformed || !datatransform_ )
	    usedarr = arr;
	else
	{
	    CubeSampling cs;
	    cs.hrg.start.inl = cs.hrg.stop.inl = 0;
	    cs.hrg.start.crl = trcnrrg_.start;
	    cs.hrg.stop.crl = trcnrrg_.stop;
	    assign( cs.zrg, curzrg_ );
	    if ( voiidx_ < 0 )
		voiidx_ = datatransform_->addVolumeOfInterest( cs, true );
	    else
		datatransform_->setVolumeOfInterest( voiidx_, cs, true );
	    datatransform_->loadDataIfMissing( voiidx_ );

	    ZAxisTransformSampler outpsampler( *datatransform_,true,BinID(0,0),
				SamplingData<double>(cs.zrg.start,cs.zrg.step));
	    mTryAlloc( tmparr, Array2DImpl<float>( cs.nrCrl(), cs.nrZ() ) );
	    usedarr = tmparr;
	    const int inl = datatransform_->lineIndex( getLineName() );
	    for ( int crlidx=0; crlidx<cs.nrCrl() && inl>=0; crlidx++ )
	    {
		BinID bid = cs.hrg.atIndex( 0, crlidx );
		bid.inl = inl;
		outpsampler.setBinID( bid );
		outpsampler.computeCache( Interval<int>(0,cs.nrZ()-1) );

		const float* inputptr = arr->getData() +
					arr->info().getOffset( crlidx, 0 );
		const float z0 = data2dh.dataset_[0]->z0_ * sd.step;
		SampledFunctionImpl<float,const float*>
		    inputfunc( inputptr, arrzsz, z0, sd.step );
		inputfunc.setHasUdfs( true );
		inputfunc.setInterpolate( !isClassification(attrib) );

		float* outputptr = tmparr->getData() +
				   tmparr->info().getOffset( crlidx, 0 );	
		reSample( inputfunc, outpsampler, outputptr, cs.nrZ() );
	    }
	}

	Array2DSlice<float> slice( *usedarr );
	slice.setDimMap( 0, 1 );
	slice.setDimMap( 1, 0 );
	if ( !slice.init() )
	    continue;

	if ( texture_ )
	{
	    if ( resolution_ )
		texture_->setDataOversample( attrib, sidx, resolution_, 
			!isClassification(attrib), &slice, true );
	    else
	    {
		texture_->splitTexture( true );
    		texture_->setData( attrib, sidx, &slice, true );
	    }
	}
	else
	{
	    channels_->setSize( 1, slice.info().getSize(1),
		    		   slice.info().getSize(0) );
	    channels_->setUnMappedData( attrib, sidx, usedarr->getData(), 
		    			OD::CopyPtr, tr );
	}
    
	triangles_->setTextureZPixels( slice.info().getSize(0) );
    }

    if ( texture_ )
	texture_->turnOn( true );
    else
	channels_->turnOn( true );
}


void Seis2DDisplay::updateVizPath()
{
    triangles_->setDisplayedGeometry( trcnrrg_, curzrg_ );
    if ( trcnrrg_.width() )
    	updateLineNamePos();
   
   if ( texture_ ) 
       texture_->clearAll();
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
    s2dd->setZRange( curzrg_ );
    s2dd->setTraceNrRange( trcnrrg_ );
    s2dd->setResolution( getResolution(), tr );

    s2dd->setLineSetID( linesetid_ );
    s2dd->setLineName( getLineName() );

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
    Coord3 xytpos = scene_->getUTM2DisplayTransform()->transformBack( pos );
    
    int trcidx; float mindist;
    getNearestTrace( xytpos, trcidx, mindist );
    if ( mindist<0 || mIsUdf(mindist) )
	return mUdf(float);

    Interval<float> zrg = getZRange( true );
    float zdif = 0;
    if ( !zrg.includes(xytpos.z) )
    {
	zdif = mMIN( fabs(xytpos.z-zrg.start), fabs(xytpos.z-zrg.stop) );
	const float zscale = scene_
	    ? scene_->getZScale() *scene_->getZStretch()
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
{
    return texture_ ? (texture_->usesShading() ? 1 : 3) : 1;
}


void Seis2DDisplay::setResolution( int res, TaskRunner* tr )
{
    if ( !texture_ || texture_->canUseShading() )
	return;

    if ( res==resolution_ )
	return;

    texture_->clearAll();
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
    par.clear();
    par.set( sKey::XCoord, evinfo.worldpickedpos.x );
    par.set( sKey::YCoord, evinfo.worldpickedpos.y );
    par.set( sKey::LineKey, name() );

    int dataidx = -1;
    float mindist;
    if ( getNearestTrace(evinfo.worldpickedpos,dataidx,mindist) )
	par.set( sKey::TraceNr, geometry_.posns_[dataidx].nr_ );

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
	info += geometry_.posns_[dataidx].nr_;
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

    const int trcnr = geometry_.posns_[trcidx].nr_;
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

    return  geometry_.posns_[trcidx].nr_;
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

    for ( int aidx=0; aidx<geometry_.posns_.size()-1; aidx++ )
    {
	const Coord posa = geometry_.posns_[aidx].coord_;
	if ( !posa.isDefined() || !trcrg.includes(geometry_.posns_[aidx].nr_) )
	    continue;

	Coord posb = Coord::udf();
	int bidx = aidx;

	while ( !posb.isDefined() && bidx<geometry_.posns_.size()-1 &&
		trcrg.includes(geometry_.posns_[bidx+1].nr_) )
	{
	    bidx++;
	    posb = geometry_.posns_[bidx].coord_;
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
		trcnr1st = geometry_.posns_[aidx].nr_;
		trcnr2nd = geometry_.posns_[bidx].nr_;
		frac = 0.0;
	    }
	    continue;
	}

	if ( dist2a >= dist2b+dist2c )
	{
	    if ( mindist2 > dist2b )
	    {
		mindist2 = dist2b;
		trcnr1st = geometry_.posns_[aidx].nr_;
		trcnr2nd = geometry_.posns_[bidx].nr_;
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
	    trcnr1st = geometry_.posns_[aidx].nr_;
	    trcnr2nd = geometry_.posns_[bidx].nr_;
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

    const Coord& crd = geometry_.posns_[trcidx].coord_;
    pos.x = crd.x; pos.y = crd.y; 
}


void Seis2DDisplay::setLineSetID( const MultiID& mid )
{ linesetid_ = mid; }


const MultiID& Seis2DDisplay::lineSetID() const
{ return linesetid_; }


bool Seis2DDisplay::getNearestTrace( const Coord3& pos,
				     int& trcidx, float& mindist ) const
{
    trcidx = -1;
    mSetUdf(mindist);

    for ( int idx=geometry_.posns_.size()-1; idx>=0; idx-- )
    {
	if ( !trcnrrg_.includes( geometry_.posns_[idx].nr_ ) ) continue;

	const float dist = pos.Coord::sqDistTo( geometry_.posns_[idx].coord_ );
	if ( dist<mindist )
	{
	    mindist = dist;
	    trcidx = idx;
	}
    }

    return trcidx!=-1;
}


Coord Seis2DDisplay::getCoord( int trcnr ) const
{
    const int sz = geometry_.posns_.size();
    if ( !sz )
	return Coord::udf();
    if ( prevtrcidx_ >= sz )
	prevtrcidx_ = sz-1;

    const int prevnr = geometry_.posns_[prevtrcidx_].nr_;
    int dir = (geometry_.posns_[0].nr_-prevnr)*(trcnr-prevnr)<0 ? 1 : -1;

    for ( int cnt=0; cnt<=1; cnt++ )
    {
	for ( int idx=prevtrcidx_+cnt*dir; idx>=0 && idx<sz; idx+=dir )
	{
	    if ( geometry_.posns_[idx].nr_ == trcnr )
	    {
		prevtrcidx_ = idx;
		return geometry_.posns_[idx].coord_;
	    }
	}
	dir = -dir;
    }
    return Coord::udf();
}


Coord Seis2DDisplay::getNormal( int trcnr ) const
{
    int posid = -1;
    int sz = geometry_.posns_.size();
    for ( int idx=0; idx<sz; idx++ )
    {
	if ( geometry_.posns_[idx].nr_ == trcnr )
	{
	    posid = idx; 
	    break;
	}
    }

    if ( posid == -1 || sz == -1 )
	return Coord(mUdf(float), mUdf(float));

    Coord pos = geometry_.posns_[posid].coord_;
    Coord v1;
    if ( posid+1<sz )    
	v1 = geometry_.posns_[posid+1].coord_- pos; 
    else if ( posid-1>=0 )
	v1 = pos - geometry_.posns_[posid-1].coord_;

    if ( v1.x == 0 )
	return Coord( 1, 0 );
    else if ( v1.y == 0 )
	return Coord( 0, 1 );
    else
    {
	float length = Math::Sqrt( v1.x*v1.x + v1.y*v1.y );
	return Coord( -v1.y/length, v1.x/length );
    }
}


bool Seis2DDisplay::setDataTransform( ZAxisTransform* zat )
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


const ZAxisTransform* Seis2DDisplay::getDataTransform() const
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
    if ( texture_ )
    {
	for ( int idx=0; idx<texture_->nrVersions(attribnr); idx++ )
	    texture_->setData( attribnr, idx, 0 );
	
	texture_->enableTexture( attribnr, false );
    }
    else if ( channels_ )
    {
	channels_->setUnMappedData( attribnr, 0, 0, OD::UsePtr, 0 );
	channels_->turnOn( false );
    }

    Attrib::SelSpec as;
    as.set2DFlag(true);
    setSelSpec( attribnr, as );
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

    par.set( sKeyLineSetID(), linesetid_ );
    par.setYN( sKeyShowLineName(), lineNameShown() );
    par.set( sKeyTrcNrRange(), trcnrrg_ );
    par.set( sKeyZRange(), getSampleRange() );
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

	par.get( sKeyTrcNrRange(), trcnrrg_ );
	
	bool showlinename = false;
	par.getYN( sKeyShowLineName(), showlinename );
	showLineName( showlinename );
    }

    setLineName( name() );
    par.get( sKeyLineSetID(), linesetid_ );

    return 1;
}

}; // namespace visSurvey
