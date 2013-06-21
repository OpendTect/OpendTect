/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		October 2008
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "faulttrace.h"

#include "binidvalset.h"
#include "emfaultstickset.h"
#include "emfault3d.h"
#include "emhorizon.h"
#include "emsurfacegeometry.h"
#include "emmanager.h"
#include "emsurfaceiodata.h"
#include "executor.h"
#include "explfaultsticksurface.h"
#include "explplaneintersection.h"
#include "faultsticksurface.h"
#include "ioman.h"
#include "ioobj.h"
#include "posinfo2d.h"
#include "survinfo.h"


int FaultTrace::nextID( int previd ) const
{ return previd >= -1 && previd < coords_.size()-1 ? previd + 1 : -1; }

void FaultTrace::setIndices( const TypeSet<int>& indices )
{ coordindices_ = indices; }

const TypeSet<int>& FaultTrace::getIndices() const
{ return coordindices_; }

int FaultTrace::add( const Coord3& pos )
{
    Threads::Locker locker( lock_ );
    coords_ += pos;
    return coords_.size() - 1;
}


int FaultTrace::add( const Coord3& pos, float trcnr )
{
    Threads::Locker locker( lock_ );
    coords_ += pos;
    trcnrs_ += trcnr;
    return coords_.size() - 1;
}


Coord3 FaultTrace::get( int idx ) const
{ return idx >= 0 && idx < coords_.size() ? coords_[idx] : Coord3::udf(); }

float FaultTrace::getTrcNr( int idx ) const
{ return idx >= 0 && idx < trcnrs_.size() ? trcnrs_[idx] : -1; }

void FaultTrace::set( int idx, const Coord3& pos )
{
    if ( coords_.validIdx(idx) )
	coords_[idx] = pos;
}


void FaultTrace::set( int idx, const Coord3& pos, float trcnr )
{
    if ( coords_.validIdx(idx) )
    {
	coords_[idx] = pos;
	if ( trcnrs_.validIdx(idx) )
	    trcnrs_[idx] = trcnr;
    }
}


void FaultTrace::remove( int idx )
{
    Threads::Locker locker( lock_ );
    coords_.removeSingle( idx );
}


bool FaultTrace::isDefined( int idx ) const
{ return coords_.validIdx(idx) && coords_[idx].isDefined(); }


FaultTrace* FaultTrace::clone()
{
    Threads::Locker locker( lock_ );
    FaultTrace* newobj = new FaultTrace;
    newobj->coords_ = coords_;
    newobj->trcnrs_ = trcnrs_;
    return newobj;
}


bool FaultTrace::getImage( const BinID& bid, float z,
			   const Interval<float>& ztop,
			   const Interval<float>& zbot,
			   const StepInterval<int>& trcrg,
			   BinID& bidimg, float& zimg, bool posdir ) const
{
    if ( isinl_ == editedoncrl_ ) return false;

    float z1 = posdir ? ztop.start : ztop.stop;
    float z2 = posdir ? zbot.start : zbot.stop;

    const float frac = mIsEqual(z1,z2,1e-6) ? 0 : ( z - z1 ) / ( z2 - z1 );
    z1 = posdir ? ztop.stop : ztop.start;
    z2 = posdir ? zbot.stop : zbot.start;
    zimg = z1 + frac * ( z2 - z1 );
    BinID start( isinl_ ? nr_ : trcrange_.start,
		 isinl_ ? trcrange_.start : nr_ );
    BinID stop( isinl_ ? nr_ : trcrange_.stop, isinl_ ? trcrange_.stop : nr_ );
    Coord intsectn = getIntersection( start,zimg, stop, zimg );
    if ( intsectn == Coord::udf() )
	return false;

    const float fidx = trcrg.getfIndex( intsectn.x );
    const int index = posdir ? mNINT32( ceil(fidx) ) : mNINT32( floor(fidx) );
    bidimg.inl = isinl_ ? nr_ : trcrg.atIndex( index );
    bidimg.crl = isinl_ ? trcrg.atIndex( index ) : nr_;
    return true;
}


bool FaultTrace::getHorTerminalPos( const EM::Horizon& hor,
				    BinID& pos1bid, float& pos1z,
				    BinID& pos2bid, float& pos2z ) const
{
    pos2z = mUdf(float);
    const EM::SectionID sid = hor.sectionID( 0 );
    StepInterval<int> trcrg = isinl_ ? hor.geometry().colRange()
				     : hor.geometry().rowRange( sid );
    trcrg.limitTo( trcrange_ );
    for ( int trcnr=trcrg.start; trcnr<=trcrg.stop; trcnr+=trcrg.step )
    {
	pos1bid = BinID( isinl_ ? nr_ : trcnr, isinl_ ? trcnr : nr_ );
	pos1z = (float) hor.getPos( sid, pos1bid.toInt64() ).z;
	if ( mIsUdf(pos1z) )
	    continue;

	if ( !mIsUdf(pos2z) )
	{
	    Coord intsectn = getIntersection( pos1bid, pos1z, pos2bid, pos2z );
	    if ( intsectn.isDefined() )
		return true;
	}

	pos2bid = pos1bid;
	pos2z = pos1z;
    }

    return false;
}



bool FaultTrace::getHorIntersection( const EM::Horizon& hor, BinID& bid ) const
{
    BinID pos1bid, pos2bid;
    float pos1z, pos2z;
    if ( !getHorTerminalPos(hor,pos1bid,pos1z,pos2bid,pos2z) )
	return false;

    const EM::SectionID sid = hor.sectionID( 0 );
    StepInterval<int> trcrg = isinl_ ? hor.geometry().colRange()
				     : hor.geometry().rowRange( sid );
    trcrg.limitTo( trcrange_ );

    Coord intsectn = getIntersection( pos1bid, pos1z, pos2bid, pos2z );
    bid.inl = isinl_ ? nr_ : trcrg.snap( mNINT32(intsectn.x) );
    bid.crl = isinl_ ? trcrg.snap( mNINT32(intsectn.x) ) : nr_;
    return true;
}


bool FaultTrace::handleUntrimmed( const BinIDValueSet& bvs,
			Interval<float>& zintv, const BinID& negbid,
			const BinID& posbid, bool istop ) const
{
    BinID bid = negbid;
    float prevz = mUdf(float);
    int& trcvar = isinl_ ? bid.crl : bid.inl;
    BinID origin( isinl_ ? nr_ : trcrange_.stop+10,
	    	  isinl_ ? trcrange_.stop+10 : nr_ );
    for ( int idx=0; idx<1024; idx++,trcvar-- )
    {
	BinIDValueSet::Pos pos = bvs.findFirst( bid );
	if ( !pos.valid() )
	    continue;

	BinID dummy;
	float topz, botz;
	bvs.get( pos, dummy, topz, botz );
	const float z = istop ? topz : botz;
	if ( mIsUdf(z) )
	    continue;

	Coord intersectn = getIntersection( origin, z, bid, z );
	if ( !intersectn.isDefined() )
	    return false;

	if ( fabs(intersectn.x - (float)trcvar) > 10 )
	    break;

	prevz = z;
    }

    zintv.start = prevz;
    bid = posbid;
    prevz = mUdf(float);
    origin = BinID( isinl_ ? nr_ : trcrange_.start-10,
	    	    isinl_ ? trcrange_.start-10 : nr_ );
    for ( int idx=0; idx<1024; idx++,trcvar++ )
    {
	BinIDValueSet::Pos pos = bvs.findFirst( bid );
	if ( !pos.valid() )
	    continue;

	BinID dummy;
	float topz, botz;
	bvs.get( pos, dummy, topz, botz );
	const float z = istop ? topz : botz;
	if ( mIsUdf(z) )
	    continue;

	Coord intersectn = getIntersection( origin, z, bid, z );
	if ( !intersectn.isDefined() )
	    return false;

	if ( fabs(intersectn.x - (float)trcvar) > 10 )
	    break;

	prevz = z;
    }

    zintv.stop = prevz;
    return true;
}


bool FaultTrace::getHorCrossings( const BinIDValueSet& bvs,
				  Interval<float>& ztop,
				  Interval<float>& zbot ) const
{
    int step = isinl_ ? SI().crlStep() : SI().inlStep();
    BinID start( isinl_ ? nr_ : trcrange_.start - 10*step,
	         isinl_ ? trcrange_.start - 10*step : nr_ );
    float starttopz=mUdf(float), startbotz=mUdf(float);
    int& startvar = isinl_ ? start.crl : start.inl;
    const od_int64 bvssz = bvs.totalSize();
    od_int64 idx = 0;
    for ( idx=0; idx<bvssz; idx++,startvar += step )
    {
	BinIDValueSet::Pos pos = bvs.findFirst( start );
	if ( !pos.valid() )
	    continue;

	BinID dummy;
	bvs.get( pos, dummy, starttopz, startbotz );
	if ( !mIsUdf(starttopz) && !mIsUdf(startbotz) )
	    break;
    }

    if ( idx == bvssz )
	return false;

    BinID stop( isinl_ ? nr_ : trcrange_.stop + 10*step,
	    	isinl_ ? trcrange_.stop + 10*step: nr_ );
    int& stopvar = isinl_ ? stop.crl : stop.inl;
    step = -step;
    float stoptopz, stopbotz;
    float prevstoptopz = mUdf(float), prevstopbotz = mUdf(float);
    BinID stoptopbid, stopbotbid, prevstoptopbid, prevstopbotbid;
    bool foundtop = false, foundbot = false;
    bool tophasisect = false, bothasisect = false;
    for ( idx=0; idx<bvssz; idx++,stopvar += step )
    {
	if ( foundtop && foundbot )
	    break;

	BinIDValueSet::Pos pos = bvs.findFirst( stop );
	if ( !pos.valid() )
	    continue;

	BinID dummy;
	float z1=mUdf(float), z2=mUdf(float);
	bvs.get( pos, dummy, z1, z2 );
	if ( !foundtop && !mIsUdf(z1) )
	{
	    stoptopbid = stop;
	    stoptopz = z1;
	    Coord intsectn = getIntersection( start, starttopz,
		    			      stop, stoptopz );
	    if ( !tophasisect )
		tophasisect = intsectn != Coord::udf();

	    if ( tophasisect && intsectn == Coord::udf() )
		foundtop = true;
	    else
	    {
		prevstoptopbid = stop;
		prevstoptopz = stoptopz;
	    }
	}

	if ( !foundbot && !mIsUdf(z2) )
	{
	    stopbotbid = stop;
	    stopbotz = z2;
	    Coord intsectn = getIntersection( start, startbotz,
		    			      stop, stopbotz );

	    if ( !bothasisect )
		bothasisect = intsectn != Coord::udf();

	    if ( bothasisect && intsectn == Coord::udf() )
		foundbot = true;
	    else
	    {
		prevstopbotbid = stop;
		prevstopbotz = stopbotz;
	    }
	}
    }

    if ( !tophasisect && !bothasisect )
	return false;

    if ( !tophasisect && !mIsUdf(zrange_.start) )
	prevstoptopz = stoptopz = zrange_.start;
    if ( !bothasisect && !mIsUdf(zrange_.stop) )
	prevstopbotz = stopbotz = zrange_.stop;

    ztop.set( stoptopz, prevstoptopz );
    zbot.set( stopbotz, prevstopbotz );
    if ( trcnrs_.size() ) // that is 2D.
    {
	BinID stepbid( isinl_ ? 0 : step, isinl_ ? step : 0 );
	bool isuntrimmed = (stoptopbid - prevstoptopbid) == stepbid;
	if ( tophasisect && isuntrimmed && !handleUntrimmed(bvs,ztop,stoptopbid,
		    			     		  prevstoptopbid,true) )
	    return false;
	isuntrimmed = (stopbotbid - prevstopbotbid) == stepbid;
	if ( bothasisect && isuntrimmed && !handleUntrimmed(bvs,zbot,stopbotbid,
		    			     		 prevstopbotbid,false) )
	    return false;
    }

    return true;
}


float FaultTrace::getZValFor( const BinID& bid ) const
{
    const StepInterval<float>& zrg = SI().zRange( false );
    Coord intersectn = getIntersection( bid, zrg.start, bid, zrg.stop );
    return (float) intersectn.y;
}


bool FaultTrace::isOnPosSide( const BinID& bid, float z ) const
{
    if ( ( isinl_ && bid.inl != nr_ ) || ( !isinl_ && bid.crl != nr_ ) )
	return false;

    const int trcnr = isinl_ ? bid.crl : bid.inl;
    if ( trcnr >= trcrange_.stop )
	return true;
    if ( trcnr <= trcrange_.start )
	return true;

    const BinID startbid( isinl_ ? nr_ : trcrange_.start,
	    		  isinl_ ? trcrange_.start : nr_ );
    const BinID stopbid( isinl_ ? nr_ : trcrange_.stop,
	    		  isinl_ ? trcrange_.stop : nr_ );
    const float zmid = ( zrange_.start + zrange_.stop ) / 2;
    Coord startintersectn = getIntersection( startbid, zmid, bid, z );
    if ( startintersectn.isDefined() )
	return true;

    Coord stopintersectn = getIntersection( stopbid, zmid, bid, z );
    if ( stopintersectn.isDefined() )
	return false;

    return false;
}


void FaultTrace::getAllActNames( BufferStringSet& bss )
{
    bss.erase();
    bss.add( "AllowCross (Default)" );
    bss.add( "ForbidCross (DataDriven)" );
    bss.add( "ForbidCrossHigh (ModelDriven)" );
    bss.add( "ForbidCrossLow (ModelDriven)" );
}


bool FaultTrace::includes( const BinID& bid ) const
{
    return isinl_ ? bid.inl == nr_ && trcrange_.includes( bid.crl, true )
		  : bid.crl == nr_ && trcrange_.includes( bid.inl, true );
}


void FaultTrace::computeTraceSegments()
{
    tracesegs_.erase();
    for ( int idx=1; idx<coordindices_.size(); idx++ )
    {
	const int curidx = coordindices_[idx];
	const int previdx = coordindices_[idx-1];
	if ( curidx < 0 || previdx < 0 )
	    continue;

	const Coord3& pos1 = get( previdx );
	const Coord3& pos2 = get( curidx );
	Coord nodepos1, nodepos2;
	const bool is2d = !trcnrs_.isEmpty();
	if ( is2d )
	{
	    nodepos1.setXY( getTrcNr(previdx),
		    	    pos1.z * SI().zDomain().userFactor() );
	    nodepos2.setXY( getTrcNr(curidx),
		    	    pos2.z * SI().zDomain().userFactor() );
	}
	else
	{
	    Coord posbid1 = SI().binID2Coord().transformBackNoSnap( pos1 );
	    Coord posbid2 = SI().binID2Coord().transformBackNoSnap( pos2 );
	    nodepos1.setXY( isinl_ ? posbid1.y : posbid1.x,
		    	    pos1.z * SI().zDomain().userFactor() );
	    nodepos2.setXY( isinl_ ? posbid2.y : posbid2.x,
		    	    pos2.z * SI().zDomain().userFactor() );
	}

	tracesegs_ += Line2( nodepos1, nodepos2 );
    }
}


Coord FaultTrace::getIntersection( const BinID& bid1, float z1,
				   const BinID& bid2, float z2  ) const
{
    if ( !getSize() )
	return Coord::udf();

    const bool is2d = trcnrs_.size();
    if ( is2d && trcnrs_.size() != getSize() )
	return Coord::udf();

    Interval<float> zrg( z1, z2 );
    zrg.sort();
    z1 *= SI().zDomain().userFactor();
    z2 *= SI().zDomain().userFactor();
    if ( ( isinl_ && (bid1.inl != nr_ || bid2.inl != nr_) )
	    || ( !isinl_ && (bid1.crl != nr_ || bid2.crl != nr_) ) )
	return Coord::udf();

    Coord pt1( isinl_ ? bid1.crl : bid1.inl, z1 );
    Coord pt2( isinl_ ? bid2.crl : bid2.inl, z2 );
    Line2 line( pt1, pt2 );

    for ( int idx=0; idx<tracesegs_.size(); idx++ )
    {
	const Line2& fltseg = tracesegs_[idx];
	if ( line.start_ == line.stop_ )
	{
	    if ( fltseg.isOnLine(line.start_) )
		return line.start_;
	    continue;
	}

	Coord interpos = line.intersection( fltseg );
	if ( interpos != Coord::udf() )
	{
	    interpos.y /= SI().zDomain().userFactor();
	    return interpos;
	}
    }

    return Coord::udf();
}


bool FaultTrace::getIntersection( const BinID& bid1, float z1,
				  const BinID& bid2, float z2,
				  BinID& bid, float& z,
				  const StepInterval<int>* intv ) const
{
    const Coord intersection = getIntersection( bid1, z1, bid2, z2 );
    if ( !intersection.isDefined() )
	return false;

    int trcnr = mNINT32( intersection.x );
    if ( intv )
	trcnr = intv->snap( trcnr );

    bid.inl = isinl_ ? nr_ : trcnr;
    bid.crl = isinl_ ? trcnr : nr_;
    z = (float) intersection.y;
    return !intv || intv->includes( trcnr, true );
}


bool FaultTrace::isOnFault( const BinID& bid, float z,
			    float thresholddist ) const
{
    const int trcnr = isinl_ ? bid.inl : bid.crl;
    const int alttrcnr = !isinl_ ? bid.inl : bid.crl;
    if ( trcnr != nr_ || !trcrange_.includes(alttrcnr,false) ) return false;
    BinID startbid( isinl_ ? nr_ : trcrange_.start,
	    	    isinl_ ? trcrange_.start : nr_ );

    BinID stopbid( isinl_ ? nr_ : trcrange_.stop,
	    	   isinl_ ? trcrange_.stop : nr_ );
    Coord intersection = getIntersection( bid, z, startbid, z );
    if ( !intersection.isDefined() )
	intersection = getIntersection( bid, z, stopbid, z );
    if ( !intersection.isDefined() )
	return false;
    Coord pt( isinl_ ? bid.crl : bid.inl, z );
    const double dist = pt.distTo( intersection );
    return mIsEqual(dist,0.0,thresholddist);
}


bool FaultTrace::isCrossing( const BinID& bid1, float z1,
			     const BinID& bid2, float z2  ) const
{
    if ( (isinl_ && bid1.inl != bid2.inl) || (!isinl_ && bid1.crl != bid2.crl) )
	return false;

    Interval<int> trcrg( isinl_ ? bid1.crl : bid1.inl,
	    		 isinl_ ? bid2.crl : bid2.inl );
    trcrg.sort();
    if ( !trcrg.overlaps(trcrange_,false) )
	return false;

    const Coord intersection = getIntersection( bid1, z1, bid2, z2 );
    return intersection.isDefined();
}


void FaultTrace::computeRange()
{
    trcrange_.set( mUdf(int), -mUdf(int) );
    zrange_.set( mUdf(float), -mUdf(float) );
    if ( trcnrs_.size() )
    {
	Interval<float> floattrcrg( mUdf(float), -mUdf(float) );
	for ( int idx=0; idx<trcnrs_.size(); idx++ )
	    floattrcrg.include( trcnrs_[idx], false );

	trcrange_.set( (int) floattrcrg.start, (int) ceil(floattrcrg.stop) );

	for ( int idx=0; idx<coords_.size(); idx++ )
	    zrange_.include( (float) coords_[idx].z, false );
    }
    else
    {
	for ( int idx=0; idx<coords_.size(); idx++ )
	{
	    const BinID bid = SI().transform( coords_[idx] );
	    trcrange_.include( isinl_ ? bid.crl : bid.inl, false );
	    zrange_.include( (float) coords_[idx].z, false );
	}
    }

    computeTraceSegments();
}


bool FaultTrace::isOK() const
{
    if ( coords_.isEmpty() ) return false;

    double prevz = coords_[0].z;
    for ( int idx=1; idx<coords_.size(); idx++ )
    {
	if ( coords_[idx].z < prevz )
	    return false;

	prevz = coords_[idx].z;
    }

    return true;
}


//FaultTrcHolder
FaultTrcHolder::FaultTrcHolder()
    : hs_(false)
{
    traces_.allowNull( true );
}


FaultTrcHolder::~FaultTrcHolder()
{ deepUnRef( traces_ ); }


const FaultTrace* FaultTrcHolder::getTrc( int linenr, bool isinl ) const
{
    const int idx = indexOf( linenr, isinl );
    return traces_.validIdx(idx) ? traces_[idx] : 0;
}


int FaultTrcHolder::indexOf( int linenr, bool isinl ) const
{
    return isinl ? hs_.inlIdx(linenr)
		 : hs_.crlIdx(linenr) + ( hs_.nrCrl()==1 ? 0 : hs_.nrInl() );
}


bool FaultTrcHolder::isEditedOnCrl() const
{
    for ( int idx=0; idx<traces_.size(); idx++ )
	if ( traces_[idx] )
	    return traces_[idx]->isEditedOnCrl();

    return false;
}


// FaultTraceExtractor

FaultTraceExtractor::FaultTraceExtractor( const EM::Fault& flt,
					  FaultTrcHolder& holder )
  : ParallelTask("Extracting Fault Traces")
  , fault_(flt)
  , holder_(holder)
  , totalnr_(0)
{ fault_.ref(); }

FaultTraceExtractor::~FaultTraceExtractor()
{ fault_.unRef(); }

od_int64 FaultTraceExtractor::nrIterations() const
{ return totalnr_; }

const char* FaultTraceExtractor::message() const
{ return "Extracting Fault Traces"; }

bool FaultTraceExtractor::doPrepare( int nrthreads )
{
    deepUnRef( holder_.traces_ );
    for ( int idx=0; idx<totalnr_; idx++ )
	holder_.traces_ += 0;

    return true;
}


bool FaultTraceExtractor::doWork( od_int64 start, od_int64 stop,
				  int nrthreads )
{
    for ( int idx=mCast(int,start); idx<=stop; idx++ )
    {
	if ( !extractFaultTrace(idx) )
	    return false;

	addToNrDone( 1 );
    }

    return true;
}


FaultTraceExtractor3D::FaultTraceExtractor3D( const EM::Fault& flt,
					      FaultTrcHolder& holder )
  : FaultTraceExtractor(flt,holder)
  , fltsurf_(0)
{
    totalnr_ = holder_.hs_.nrInl() == 1 || holder_.hs_.nrCrl() == 1 ? 1
				: holder_.hs_.nrInl() + holder_.hs_.nrCrl();
}


FaultTraceExtractor3D::~FaultTraceExtractor3D()
{ delete fltsurf_; }

bool FaultTraceExtractor3D::doPrepare( int nrthreads )
{
    mDynamicCastGet(const EM::Fault3D*,cfault,&fault_)
    if ( !cfault )
	return false;

    editedoncrl_ = cfault && cfault->geometry().areEditPlanesMostlyCrossline();
    EM::SectionID sid = fault_.sectionID( 0 );
    EM::Fault3D* fault3d = const_cast<EM::Fault3D*>(cfault);
    Geometry::FaultStickSurface* fss = fault3d->geometry().sectionGeometry(sid);
    if ( !fss )
	return false;

    if ( !fltsurf_ )
    {
	fltsurf_ = new Geometry::ExplFaultStickSurface( fss, SI().zScale() );
	fltsurf_->setCoordList( new FaultTrace, new FaultTrace, 0 );
	fltsurf_->update( true, 0 );
    }

    return FaultTraceExtractor::doPrepare( nrthreads );
}


bool FaultTraceExtractor3D::extractFaultTrace( int idx )
{
    mDynamicCastGet(const EM::Fault3D*,cfault,&fault_)
    if ( !cfault )
	return false;

    const HorSampling& hs = holder_.hs_;
    const bool isinl = hs.nrCrl() > 1 && idx < hs.nrInl();
    const int linenr = isinl ? hs.start.inl + idx * hs.step.inl
	: hs.start.crl + (hs.nrCrl()==1 ? 0 : (idx-hs.nrInl()) * hs.step.crl );

    const StepInterval<float>& zrg = SI().zRange( false );
    BinID start( isinl ? linenr : holder_.hs_.start.inl,
	    	 isinl ? holder_.hs_.start.crl : linenr );
    BinID stop( isinl ? linenr : holder_.hs_.stop.inl,
	    	isinl ? holder_.hs_.stop.crl : linenr );
    Coord3 p0( SI().transform(start), zrg.start );
    Coord3 p1( SI().transform(start), zrg.stop );
    Coord3 p2( SI().transform(stop), zrg.stop );
    Coord3 p3( SI().transform(stop), zrg.start );
    TypeSet<Coord3> pts;
    pts += p0; pts += p1; pts += p2; pts += p3;
    const Coord3 normal = (p1-p0).cross(p3-p0).normalize();

    PtrMan<Geometry::ExplPlaneIntersection> insectn =
					new Geometry::ExplPlaneIntersection;
    insectn->setShape( *fltsurf_ );
    insectn->addPlane( normal, pts );
    Geometry::IndexedShape* idxdshape = insectn;
    RefMan<FaultTrace> clist = new FaultTrace;
    RefMan<FaultTrace> normallist = new FaultTrace;
    idxdshape->setCoordList( clist, normallist, 0 );
    if ( !idxdshape->update(true,0) )
	return false;

    const Geometry::IndexedGeometry* idxgeom = idxdshape->getGeometry()[0];
    if ( !idxgeom ) return false;

    FaultTrace* flttrc = clist->clone();
    flttrc->ref();
    flttrc->setIndices( idxgeom->coordindices_ );
    flttrc->setIsInl( isinl );
    flttrc->setEditedOnCrl( editedoncrl_ );
    flttrc->setLineNr( linenr );
    flttrc->computeRange();
    holder_.traces_.replace( idx, flttrc );
    return true;
}


FaultTraceExtractor2D::FaultTraceExtractor2D( const EM::Fault& flt,
					  FaultTrcHolder& holder,
					  const PosInfo::GeomID& geomid )
  : FaultTraceExtractor(flt,holder)
  , geomid_(geomid)
  , linegeom_(*new PosInfo::Line2DData)
{
    mDynamicCastGet(const EM::FaultStickSet*,fss,&fault_)
    const EM::SectionID sid = fault_.sectionID( 0 );
    totalnr_ = fss ? fss->geometry().nrSticks( sid ) : 0;
}


FaultTraceExtractor2D::~FaultTraceExtractor2D()
{ delete &linegeom_; }

bool FaultTraceExtractor2D::doPrepare( int nrthreads )
{
    S2DPOS().setCurLineSet( geomid_.lsid_ );
    if ( !S2DPOS().getGeometry(geomid_.lineid_,linegeom_) )
	return false;

    return FaultTraceExtractor::doPrepare( nrthreads );
}


static float getFloatTrcNr( const PosInfo::Line2DData& linegeom,
			    const Coord& crd )
{
    const int index = linegeom.nearestIdx( crd );
    if ( index < 0 ) return mUdf(float);

    const TypeSet<PosInfo::Line2DPos>& posns = linegeom.positions();
    const PosInfo::Line2DPos& pos = posns[index];
    float closestdistfromnode = mUdf(float);
    int index2 = -1;
    if ( index > 0 )
    {
	const PosInfo::Line2DPos& prevpos = posns[index-1];
	const float distfromnode = (float) prevpos.coord_.distTo( crd );
	if ( distfromnode < closestdistfromnode )
	{
	    closestdistfromnode = distfromnode;
	    index2 = index - 1;
	}
    }

    if ( posns.validIdx(index+1) )
    {
	const PosInfo::Line2DPos& nextpos = posns[index+1];
	const float distfromnode = (float) nextpos.coord_.distTo( crd );
	if ( distfromnode < closestdistfromnode )
	{
	    closestdistfromnode = distfromnode;
	    index2 = index + 1;
	}
    }

    if ( index2 < 0 )
	return mUdf( float );

    const Coord linepos1 = pos.coord_;
    const Coord linepos2 = posns[index2].coord_;
    const Line2 line( linepos1, linepos2 );
    Coord posonline = line.closestPoint( crd );
    if ( posonline.distTo(crd) > 100 )
	return mUdf(float);

    const float frac =
	mCast(float,linepos1.distTo(posonline)/linepos1.distTo(linepos2));
    return (float)(pos.nr_ + frac * ( posns[index2].nr_ - pos.nr_ ));
}


bool FaultTraceExtractor2D::doFinish( bool success )
{
    if ( !success )
	return false;

    Interval<int> trcrg( mUdf(int), -mUdf(int) );
    for ( int idx=0; idx<totalnr_; idx++ )
	if ( holder_.traces_[idx] )
	    trcrg.include( holder_.traces_[idx]->trcRange() );

    holder_.hs_.setInlRange( Interval<int>(0,0) );
    holder_.hs_.setCrlRange( trcrg );
    return true;
}


bool FaultTraceExtractor2D::extractFaultTrace( int stickidx )
{
    if ( linegeom_.isEmpty() )
	return false;

    mDynamicCastGet(const EM::FaultStickSet*,fss,&fault_)
    if ( !fss ) return false;

    const EM::SectionID sid = fault_.sectionID( 0 );
    const Geometry::FaultStickSet* fltgeom =
			fss->geometry().sectionGeometry( sid );
    if ( !fltgeom ) return false;

    const int sticknr = fltgeom->rowRange().atIndex( stickidx );
    if ( !fss->geometry().pickedOn2DLine(sid,sticknr) )
	return true;

    const MultiID* lsid = fss->geometry().pickedMultiID( sid, sticknr );
    if ( !lsid ) return true;

    PtrMan<IOObj> lsobj = IOM().get( *lsid );
    if ( !lsobj ) return true;

    const char* linenm = fss->geometry().pickedName( sid, sticknr );
    if ( !linenm ) return true;

    PosInfo::GeomID geomid = S2DPOS().getGeomID( lsobj->name(), linenm );
    if ( !(geomid==geomid_) ) return true;

    const int nrknots = fltgeom->nrKnots( sticknr );
    if ( nrknots < 2 ) return true;

    FaultTrace* flttrc = new FaultTrace;
    flttrc->ref();
    flttrc->setIsInl( true );
    flttrc->setLineNr( 0 );

    StepInterval<int> colrg = fltgeom->colRange( sticknr );
    TypeSet<int> indices;
    for ( int idx=colrg.start; idx<=colrg.stop; idx+=colrg.step )
    {
	const Coord3 knot = fltgeom->getKnot( RowCol(sticknr,idx) );
	const float trcnr = getFloatTrcNr( linegeom_, knot );
	if ( mIsUdf(trcnr) )
	    break;

	indices += flttrc->add( knot, trcnr );
    }

    indices += -1;
    flttrc->setIndices( indices );
    flttrc->computeRange();
    holder_.traces_.replace( stickidx, flttrc );
    return true;
}


//FaultTrcDataProvider
FaultTrcDataProvider::FaultTrcDataProvider()
    : is2d_(false)
{ holders_.allowNull(); }

FaultTrcDataProvider::FaultTrcDataProvider( const PosInfo::GeomID& geomid )
    : geomid_(geomid)
    , is2d_(true)
{ holders_.allowNull(); }

FaultTrcDataProvider::~FaultTrcDataProvider()
{ clear(); }

const char* FaultTrcDataProvider::errMsg() const
{ return errmsg_.buf(); }

int FaultTrcDataProvider::nrFaults() const
{ return holders_.size(); }

bool FaultTrcDataProvider::isEmpty() const
{ return holders_.isEmpty(); }

HorSampling FaultTrcDataProvider::range( int idx ) const
{
    return holders_.validIdx(idx) && holders_[idx] ? holders_[idx]->hs_
						   : HorSampling(false);
}


bool FaultTrcDataProvider::isEditedOnCrl( int idx ) const
{
    return holders_.validIdx(idx) && holders_[idx]
	? holders_[idx]->isEditedOnCrl() : false;
}


void FaultTrcDataProvider::clear()
{ deepErase( holders_ ); }

const FaultTrace* FaultTrcDataProvider::getFaultTrace( int idx, int linenr,
						       bool isinl ) const
{
    return holders_.validIdx(idx) && holders_[idx] && !is2d_
	?  holders_[idx]->getTrc( linenr, isinl ) : 0;
}


int FaultTrcDataProvider::nrSticks( int idx ) const
{
    return holders_.validIdx(idx) && holders_[idx]
	? holders_[idx]->traces_.size() : 0;
}


const FaultTrace* FaultTrcDataProvider::getFaultTrace2D( int fltidx,
							 int stickidx ) const
{
    return holders_.validIdx(fltidx) && holders_[fltidx]
		&& holders_[fltidx]->traces_.validIdx(stickidx)
	? holders_[fltidx]->traces_[stickidx] : 0;
}


bool FaultTrcDataProvider::calcFaultBBox( const EM::Fault& flt,
					  HorSampling& hs ) const
{
    for ( int sidx=0; sidx<flt.nrSections(); sidx++ )
    {
	const EM::SectionID sid = flt.sectionID( sidx );
	mDynamicCastGet(const Geometry::FaultStickSet*,fss,
			flt.geometry().sectionGeometry(sid))
	if ( !fss )
	    continue;

	const StepInterval<int> rowrg = fss->rowRange();
	for ( int idx=rowrg.start; idx<=rowrg.stop; idx+=rowrg.step )
	{
	    const StepInterval<int> colrg = fss->colRange( idx );
	    for ( int idy=colrg.start; idy<=colrg.stop; idy+=colrg.step )
	    {
		const Coord3 knot = fss->getKnot( RowCol(idx,idy) );
		const BinID bid = SI().transform( knot );
		hs.include( bid );
	    }
	}
    }

    return true;
}

# define mErrRet( str ) \
{ errmsg_.setEmpty(); errmsg_ = str; return false; }

bool FaultTrcDataProvider::init( const TypeSet<MultiID>& faultids,
				 const HorSampling& hrg, TaskRunner* tr )
{
    clear();
    EM::SurfaceIOData sd;
    EM::SurfaceIODataSelection sel( sd );
    sd.rg = hrg;
    ExecutorGroup loadergrp( "Loading Faults" );
    for ( int idx=0; idx<faultids.size(); idx++ )
	if ( EM::EMM().getObjectID(faultids[idx]) < 0 )
	    loadergrp.add( EM::EMM().objectLoader(faultids[idx],&sel) );

    const int res = TaskRunner::execute( tr, loadergrp );
    if ( !res )
	mErrRet("Failed to read the faults from disc")

    if ( is2d_ )
	return get2DTraces( faultids, tr );

    TaskGroup taskgrp;
    for ( int idx=0; idx<faultids.size(); idx++ )
    {
	const EM::ObjectID oid = EM::EMM().getObjectID( faultids[idx] );
	mDynamicCastGet(EM::Fault*,flt,EM::EMM().getObject(oid))
	if ( !flt )
	{
	    errmsg_ = BufferString( "Failed to load ", faultids[idx] );
	    holders_ += 0;
	    continue;
	}

	HorSampling hs( false );
	calcFaultBBox( *flt, hs );
	hs.limitTo( hrg );
	FaultTrcHolder* holder = new FaultTrcHolder();
	holder->hs_ = hs;
	holders_ += holder;
	taskgrp.addTask( new FaultTraceExtractor3D(*flt,*holder) );
    }

    const bool ret = TaskRunner::execute( tr, taskgrp );
    if ( !ret )
	mErrRet("Failed to extract Fault traces")

    return true;
}


bool FaultTrcDataProvider::get2DTraces( const TypeSet<MultiID>& faultids,
					TaskRunner* tr )
{
    TaskGroup taskgrp;
    for ( int idx=0; idx<faultids.size(); idx++ )
    {
	const EM::ObjectID oid = EM::EMM().getObjectID( faultids[idx] );
	mDynamicCastGet(EM::Fault*,flt,EM::EMM().getObject(oid))
	if ( !flt )
	{
	    holders_ += 0;
	    continue;
	}

	StepInterval<int> trcrg;
	FaultTrcHolder* holder = new FaultTrcHolder();
	holders_ += holder;
	taskgrp.addTask( new FaultTraceExtractor2D(*flt,*holder,geomid_) );
    }

    return TaskRunner::execute( tr, taskgrp );
}


bool FaultTrcDataProvider::hasFaults( const BinID& bid ) const
{
    for ( int idx=0; idx<holders_.size(); idx++ )
    {
	if ( !holders_[idx] )
	    continue;

	if ( !is2d_ )
	{
	    const FaultTrace* inltrc = holders_[idx]->getTrc( bid.inl, true );
	    const FaultTrace* crltrc = holders_[idx]->getTrc( bid.crl, false );
	    if ( (inltrc && inltrc->includes(bid))
	      || (crltrc && crltrc->includes(bid)) )
		return true;
	}
	else
	{
	    for ( int sidx=0; sidx<holders_[idx]->traces_.size(); sidx++ )
	    {
		const FaultTrace* flttrc = holders_[idx]->traces_[sidx];
		if ( flttrc && flttrc->includes(bid) )
		    return true;
	    }
	}
    }

    return false;
}


bool FaultTrcDataProvider::isOnFault( const BinID& bid, float z,
				      float thresholddist ) const
{
    if ( is2d_ )
    {
    	for ( int idx=0; idx<nrFaults(); idx++ )
	{
	    const int nrsticks = nrSticks( idx );
	    for ( int idy=0; idy<nrsticks; idy++ )
	    {
		const FaultTrace* flt = getFaultTrace2D( idx, idy );
		if ( flt && flt->isOnFault(bid,z,thresholddist) )
		    return true;
	    }
	}

	return false;
    }

    for ( int idx=0; idx<nrFaults(); idx++ )
    {
	const FaultTrace* flt = getFaultTrace( idx, bid.inl, true );
	if ( !flt )
	    flt = getFaultTrace( idx, bid.crl, false );

	if ( flt && flt->isOnFault(bid,z,thresholddist) )
	    return true;
    }

    return false;
}


bool FaultTrcDataProvider::isCrossingFault( const BinID& bid1, float z1,
					    const BinID& bid2, float z2 ) const
{
    if ( is2d_ )
    {
    	for ( int idx=0; idx<nrFaults(); idx++ )
	{
	    const int nrsticks = nrSticks( idx );
	    for ( int idy=0; idy<nrsticks; idy++ )
	    {
		const FaultTrace* flt = getFaultTrace2D( idx, idy );
		if ( flt && flt->isCrossing(bid1,z1,bid2,z2) )
		    return true;
	    }
	}

	return false;
    }

    const int inldiff = abs( bid1.inl - bid2.inl );
    const int crldiff = abs( bid1.crl - bid2.crl );
    if ( !inldiff && !crldiff )
    {
	for ( int idx=0; idx<nrFaults(); idx++ )
	{
	    const FaultTrace* flt = getFaultTrace( idx, bid1.inl, true );
	    if ( !flt )
		flt = getFaultTrace( idx, bid1.crl, false );

	    if ( flt && flt->isCrossing(bid1,z1,bid2,z2) )
		return true;
	}

	return false;
    }

    const BinID anchorbid( bid2.inl, bid1.crl );
    const float anchorz = z1 + ( z2 - z1 ) * inldiff / ( inldiff + crldiff );
    for ( int idx=0; idx<nrFaults(); idx++ )
    {
	int nrcrossings = 0;
	if ( bid1.inl != bid2.inl )
	{
	    const FaultTrace* crlflt = getFaultTrace( idx, bid1.crl, false );
	    if ( crlflt && crlflt->isCrossing(bid1,z1,anchorbid,anchorz) )
		nrcrossings++;
	}

	if ( bid1.crl != bid2.crl )
	{
	    const FaultTrace* inlflt = getFaultTrace( idx, bid2.inl, true );
	    if ( inlflt && inlflt->isCrossing(bid2,z2,anchorbid,anchorz) )
		nrcrossings++;
	}

	if ( nrcrossings == 1 )
	    return true;
    }

    return false;
}


bool FaultTrcDataProvider::getFaultZVals( const BinID& bid,
					  TypeSet<float>& zvals ) const
{
    zvals.erase();
    for ( int idx=0; idx<nrFaults(); idx++ )
    {
	const int nrsticks = is2d_ ? nrSticks( idx ) : 1;
	for ( int idy=0; idy<nrsticks; idy++ )
	{
	    const FaultTrace* flt = is2d_ ? getFaultTrace2D( idx, idy ) :
		getFaultTrace( idx,bid.inl,true);
	    if ( !is2d_ && !flt )
		flt = getFaultTrace( idx, bid.crl, false );

	    if ( !flt ) continue;

	    const float z = flt->getZValFor( bid );
	    if ( !mIsUdf(z) )
	   	zvals += z;
	}
    }

    return !zvals.isEmpty();
}

