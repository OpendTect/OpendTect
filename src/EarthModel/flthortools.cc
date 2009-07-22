/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		October 2008
________________________________________________________________________

-*/
static const char* rcsID = "$Id: flthortools.cc,v 1.16 2009-07-22 14:30:30 bert Exp $";

#include "flthortools.h"

#include "emfaultstickset.h"
#include "emhorizon2d.h"
#include "emmanager.h"
#include "executor.h"
#include "faultsticksurface.h"
#include "ioman.h"
#include "ioobj.h"
#include "posinfo.h"
#include "seis2dline.h"
#include "survinfo.h"


namespace SSIS
{

FaultStickSubSampler::FaultStickSubSampler( const EM::FaultStickSet& flt,
					    int sticknr, float zstep )
    : fault_(flt)
    , sticknr_(sticknr)
    , zstep_(zstep)
{
    fault_.ref();
}


FaultStickSubSampler::~FaultStickSubSampler()
{
    fault_.unRef();
}


bool FaultStickSubSampler::execute()
{
    crds_.erase();
    EM::SectionID fltsid( 0 );
    const int nrknots = fault_.geometry().nrKnots( fltsid, sticknr_ );
    const Geometry::FaultStickSet* fltgeom =
	fault_.geometry().sectionGeometry( fltsid );
    const Coord3 firstknot = fltgeom->getKnot( RowCol(sticknr_,0) );
    const Coord3 lastknot = fltgeom->getKnot( RowCol(sticknr_,nrknots-1) );
    const int nrzvals = mNINT( (lastknot.z-firstknot.z) / zstep_ );

    for ( int idx=0; idx<nrzvals; idx++ )
    {
	const float curz = firstknot.z + idx*zstep_;
	bool found = false;
	for ( int knotidx=0; knotidx<nrknots-1 && !found; knotidx++ )
	{
	    const Coord3 knot1 = fltgeom->getKnot( RowCol(sticknr_,knotidx) );
	    const Coord3 knot2 = fltgeom->getKnot( RowCol(sticknr_,knotidx+1) );
	    if ( mIsEqual(curz,knot1.z,1e-3) )
	    {
		crds_ += knot1;
		found = true;
	    }
	    else if ( mIsEqual(curz,knot2.z,1e-3) )
	    {
		crds_ += knot2;
		found = true;
	    }
	    else if ( curz>knot1.z && curz<knot2.z )
	    {
		Coord newcrd = knot1.coord() + (knot2.coord()-knot1.coord())*
					    (curz-knot1.z)/(knot2.z-knot1.z);
		crds_ += Coord3( newcrd, curz );
		found = true;
	    }
	}
    }

    return crds_.size() > 0;
}


Coord3 FaultStickSubSampler::getCoord( float zval ) const
{
    if ( zval < crds_.first().z )
	return crds_.first();
    if ( zval > crds_.last().z )
	return crds_.last();

    const float diff = zval - crds_[0].z;
    const float fidx = diff / zstep_;
    const int idx = mNINT( fidx );
    return crds_.validIdx( idx ) ? crds_[idx] : Coord3::udf();
}


const TypeSet<Coord3>& FaultStickSubSampler::getCoordList() const
{ return crds_; }


// ***** FaultHorizon2DIntersectionFinder *****
FaultHorizon2DIntersectionFinder::FaultHorizon2DIntersectionFinder(
	const EM::FaultStickSet& flt, int sticknr, const EM::Horizon2D& hor )
    : flt_(flt)
    , hor_(hor)
    , sticknr_(sticknr)
{
    flt_.ref();
    hor_.ref();
}


FaultHorizon2DIntersectionFinder::~FaultHorizon2DIntersectionFinder()
{
    flt_.unRef();
    hor_.unRef();
}


bool FaultHorizon2DIntersectionFinder::find( float& trcnr, float& zval )
{
    trcnr = 0; zval = 0;

    FaultStickSubSampler sampler( flt_, sticknr_, SI().zStep() );
    sampler.execute();
    TypeSet<Coord3> crds = sampler.getCoordList();

    // TODO: optimize
    EM::SectionID sid( 0 );
    const MultiID* lsid = flt_.geometry().lineSet( sid, sticknr_ );
    if ( !lsid ) return false;
    PtrMan<IOObj> lsioobj = IOM().get( *lsid );
    if ( !lsioobj ) return false;

    Seis2DLineSet ls( *lsioobj );
    const char* lnm = flt_.geometry().lineName( sid, sticknr_ );
    const int lineidx = ls.indexOfFirstOccurrence( lnm );
    PosInfo::Line2DData lineposinfo;
    if ( !ls.getGeometry(lineidx,lineposinfo) )
	return false;

    PosInfo::Line2DPos firstpos, lastpos;
    if ( !lineposinfo.getPos(crds.first(),firstpos) ||
	 !lineposinfo.getPos(crds.last(),lastpos) )
	return false;

    const int lidx = hor_.geometry().lineIndex( lnm );

    Interval<int> trcrg( firstpos.nr_, lastpos.nr_ ); trcrg.sort();
    trcrg.start -= 5; trcrg.stop += 5;
    HorSampling hs; hs.set( Interval<int>(0,0), trcrg );

    bool init = false;
    bool negside = true;
    for ( int crlidx=0; crlidx<hs.nrCrl(); crlidx++ )
    {
	const int crl = trcrg.atIndex( crlidx, 1 );
	const float z = hor_.getPos( sid, lidx, crl ).z;
	const Coord3 fltcrd = sampler.getCoord( z );
	PosInfo::Line2DPos fltpos;
	const bool res = lineposinfo.getPos( fltcrd, fltpos );
	if ( !res ) continue;

	const bool side = fltpos.nr_>crl;
	if ( !init )
	{
	    negside = side;
	    init = true;
	}
	else if ( negside != side )
	{
	    trcnr = crl; zval = z;
	    return true;
	}
    }

    return false;
}


// ***** FaultHorizon2DLocationField *****
FaultHorizon2DLocationField::FaultHorizon2DLocationField(
	const EM::FaultStickSet& flt, int sticknr,
	const EM::Horizon2D& h1, const EM::Horizon2D& h2 )
    : Array2DImpl<char>(1,1)
    , flt_(flt)
    , tophor_(h1)
    , bothor_(h2)
    , sticknr_(sticknr)
{
    flt_.ref();
    tophor_.ref();
    bothor_.ref();

    linenm_ = flt.geometry().lineName( EM::SectionID(0), sticknr_ );
}


FaultHorizon2DLocationField::~FaultHorizon2DLocationField()
{
    flt_.unRef();
    tophor_.unRef();
    bothor_.unRef();
}


bool FaultHorizon2DLocationField::calculate()
{
    FaultStickSubSampler sampler( flt_, sticknr_, SI().zStep() );
    sampler.execute();
    TypeSet<Coord3> crds = sampler.getCoordList();

    EM::SectionID sid( 0 );
    const MultiID* lsid = flt_.geometry().lineSet( sid, sticknr_ );
    if ( !lsid ) return false;
    PtrMan<IOObj> lsioobj = IOM().get( *lsid );
    if ( !lsioobj ) return false;

    Seis2DLineSet ls( *lsioobj );
    const char* lnm = flt_.geometry().lineName( sid, sticknr_ );
    const int lineidx = ls.indexOfFirstOccurrence( lnm );
    PosInfo::Line2DData lineposinfo;
    if ( !ls.getGeometry(lineidx,lineposinfo) )
	return false;

    PosInfo::Line2DPos pos2d;
    Interval<int> trcrg( mUdf(int), -mUdf(int) );
    for ( int idx=0; idx<crds.size(); idx++ )
    {
	if ( lineposinfo.getPos(crds[idx],pos2d) )
	    trcrg.include( pos2d.nr_, false );
    }

    if ( mIsUdf(trcrg.start) )
	return false;

    const int lidxtop = tophor_.geometry().lineIndex( lnm );
    const int lidxbot = bothor_.geometry().lineIndex( lnm );

    cs_.hrg.set( Interval<int>(0,0), trcrg );
    Interval<float> zrg =
	tophor_.geometry().sectionGeometry(sid)->zRange(lidxtop);
    zrg.include( bothor_.geometry().sectionGeometry(sid)->zRange(lidxbot) );
    SI().snapZ( zrg.start, -1 ); SI().snapZ( zrg.stop, 1 );
    cs_.zrg.setFrom( zrg );
    setSize( cs_.nrCrl(), cs_.nrZ() );

    if ( GetEnvVarYN("OD_PRINT_FAULTFIELD") )
	std::cout << cs_.zrg.start << '\t' << cs_.zrg.step << '\t' << cs_.nrZ()
	    	  << std::endl;

    for ( int crlidx=0; crlidx<cs_.nrCrl(); crlidx++ )
    {
	const int crl = trcrg.atIndex( crlidx, 1 );
	const float topz = tophor_.getPos( sid, lidxtop, crl ).z;
	const float botz = bothor_.getPos( sid, lidxbot, crl ).z;
	for ( int zidx=0; zidx<cs_.nrZ(); zidx++ )
	{
	    const float zval = cs_.zAtIndex( zidx );
	    if ( zval<topz || zval>botz )
		set( crlidx, zidx, sOutside() );
	    else
	    {
		const Coord3 fltcrd = sampler.getCoord( zval );
		PosInfo::Line2DPos fltpos;
		const bool res = lineposinfo.getPos( fltcrd, fltpos );
		if ( !res )
		    set( crlidx, zidx, sOutside() );
		else if ( fltpos.nr_>crl )
		    set( crlidx, zidx, sInsideNeg() );
		else if ( fltpos.nr_<=crl )
		    set( crlidx, zidx, sInsidePos() );
		else
		    set( crlidx, zidx, sOutside() );
	    }
	}

	if ( GetEnvVarYN("OD_PRINT_FAULTFIELD") )
	{
	    PosInfo::Line2DPos pos; lineposinfo.getPos( crl, pos );
	    std::cout << crl << '\t' << pos.coord_.x << '\t' << pos.coord_.y;
	    for ( int zidx=0; zidx<cs_.nrZ(); zidx++ )
		std::cout << '\t' << get(crlidx,zidx);
	    std::cout << std::endl;
	}
    }

    return true;
}


const char FaultHorizon2DLocationField::getPos( int trcnr, float z ) const
{
    if ( !cs_.zrg.includes(z,false) )
	return sOutside();

    const int idx0 = cs_.crlIdx( trcnr );
    const int idx1 = cs_.zIdx( z );
    return get( idx0, idx1 );
}


// ***** FaultStickThrow *****
FaultStickThrow::FaultStickThrow( const EM::FaultStickSet& flt, int sticknr,
			const EM::Horizon2D& hor1, const EM::Horizon2D& hor2 )
    : flt_(flt)
    , tophor_(hor1)
    , bothor_(hor2)
    , sticknr_(sticknr)
{
    flt_.ref();
    tophor_.ref();
    bothor_.ref();

    linenm_ = flt.geometry().lineName( EM::SectionID(0), sticknr_ );
    topzneg_ = topzpos_, botzneg_, botzpos_ = mUdf(float);
    init();
}


FaultStickThrow::~FaultStickThrow()
{
    flt_.unRef();
    tophor_.unRef();
    bothor_.unRef();
}


bool FaultStickThrow::findInterSections( float& toptrcnr, float& bottrcnr )
{
    float topz, botz;
    toptrcnr = bottrcnr = mUdf(float);
    FaultHorizon2DIntersectionFinder findertop( flt_, sticknr_, tophor_ );
    const bool res1 = findertop.find( toptrcnr, topz );
    FaultHorizon2DIntersectionFinder finderbot( flt_, sticknr_, bothor_ );
    const bool res2 = finderbot.find( bottrcnr, botz );
    return res1 && res2;
}


bool FaultStickThrow::init()
{
    EM::SectionID sid( 0 );
    const MultiID* lsid = flt_.geometry().lineSet( sid, sticknr_ );
    PtrMan<IOObj> lsioobj = IOM().get( *lsid );
    if ( !lsioobj ) return false;

    const char* lnm = flt_.geometry().lineName( sid, sticknr_ );
    Seis2DLineSet ls( *lsioobj );
    const int lineidx = ls.indexOfFirstOccurrence( lnm );
    PosInfo::Line2DData lineposinfo;
    if ( !ls.getGeometry(lineidx,lineposinfo) )
	return false;

    const int lidxtop = tophor_.geometry().lineIndex( lnm );
    const int lidxbot = bothor_.geometry().lineIndex( lnm );

    float toptrcnr, bottrcnr;
    if ( !findInterSections(toptrcnr,bottrcnr) )
	return false;

    topzneg_ = tophor_.getPos( sid, lidxtop, mNINT(toptrcnr-5) ).z;
    topzpos_ = tophor_.getPos( sid, lidxtop, mNINT(toptrcnr+5) ).z;
    botzneg_ = bothor_.getPos( sid, lidxbot, mNINT(bottrcnr-5) ).z;
    botzpos_ = bothor_.getPos( sid, lidxbot, mNINT(bottrcnr+5) ).z;

    return true;
}


float FaultStickThrow::getValue( float z, bool negtopos ) const
{
    const float topshift = topzpos_ - topzneg_;
    const float botshift = botzpos_ - botzneg_;

    float thr = mUdf(float);
    if ( negtopos )
    {
	if ( z < topzneg_ )
	    thr = topshift;
	else if ( z > botzneg_ )
	    thr = botshift;
	else
	{
	    const float fact = (botshift-topshift) / (botzneg_-topzneg_);
	    thr = topshift + fact*(z-topzneg_);
	}
    }
    else
    {
	if ( z < topzpos_ )
	    thr = -topshift;
	else if ( z > botzpos_ )
	    thr = -botshift;
	else
	{
	    const float fact = (botshift-topshift) / (botzpos_-topzpos_);
	    thr = -(topshift + fact*(z-topzpos_));
	}
    }

    return thr;
}


} // namespace SSIS
