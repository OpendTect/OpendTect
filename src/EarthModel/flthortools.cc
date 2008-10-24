/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nanne Hemstra
 Date:		October 2008
 RCS:		$Id: flthortools.cc,v 1.2 2008-10-24 12:38:47 nanne Exp $
________________________________________________________________________

-*/

#include "flthortools.h"

#include "emfault2d.h"
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

Fault2DSubSampler::Fault2DSubSampler( const EM::Fault2D& flt )
    : fault_(flt)
{
    fault_.ref();
}


Fault2DSubSampler::~Fault2DSubSampler()
{
    fault_.unRef();
}


void Fault2DSubSampler::getCoordList( float zstep, TypeSet<Coord3>& crds ) const
{
    EM::SectionID fltsid( 0 );
    const int sticknr = 0;
    const int nrknots = fault_.geometry().nrKnots( fltsid, sticknr );
    const Geometry::FaultStickSurface* fltgeom =
	fault_.geometry().sectionGeometry( fltsid );
    const Coord3 firstknot = fltgeom->getKnot( RowCol(sticknr,0) );
    const Coord3 lastknot = fltgeom->getKnot( RowCol(sticknr,nrknots-1) );
    const int nrzvals = (lastknot.z-firstknot.z) / zstep;

    for ( int idx=0; idx<nrzvals; idx++ )
    {
	const float curz = firstknot.z + idx*zstep;
	for ( int knotidx=0; knotidx<nrknots-1; knotidx++ )
	{
	    const Coord3 knot1 = fltgeom->getKnot( RowCol(sticknr,knotidx) );
	    const Coord3 knot2 = fltgeom->getKnot( RowCol(sticknr,knotidx+1) );
	    if ( mIsEqual(curz,knot1.z,1e-3) )
		crds += knot1;
	    else if ( mIsEqual(curz,knot2.z,1e-3) )
		crds += knot2;
	    else
		crds += knot1 + (knot2-knot1)*(curz-knot1.z)/(knot2.z-knot1.z);
	}
//	std::cout << idx << '\t' << crds[idx].x << '\t' << crds[idx].y << '\t' << crds[idx].z << std::endl;
    }
}


FaultHorizon2DIntersectionFinder::FaultHorizon2DIntersectionFinder(
	const MultiID& fltid, const MultiID& horid )
    : fltid_(fltid)
    , horid_(horid)
{}


bool FaultHorizon2DIntersectionFinder::find( float& trcnr, float& zval )
{
    EM::EMManager& emm = EM::EMM();
    Executor* exec = emm.objectLoader( horid_ );
    if ( !exec ) return false;
    exec->execute();
    mDynamicCastGet(EM::Horizon2D*,hor2d,emm.getObject(emm.getObjectID(horid_)))
    if ( !hor2d ) return false;
    hor2d->ref();

    delete exec;
    exec = emm.objectLoader( fltid_ );
    if ( !exec ) return false;
    exec->execute();
    mDynamicCastGet(EM::Fault2D*,flt2d,emm.getObject(emm.getObjectID(fltid_)))
    if ( !flt2d ) return false;
    flt2d->ref();

    Fault2DSubSampler sampler( *flt2d );
    TypeSet<Coord3> crds;
    sampler.getCoordList( SI().zStep(), crds );

    return true;
}



FaultHorizon2DLocationField::FaultHorizon2DLocationField(
	const EM::Fault2D& flt, const EM::Horizon2D& h1,
	const EM::Horizon2D& h2, const IOObj& lsioobj, const char* lnm )
    : Array2DImpl<char>(1,1)
    , fault_(flt)
    , tophor_(h1)
    , bothor_(h2)
    , lsioobj_(lsioobj)
    , linename_(lnm)
{
    fault_.ref();
    tophor_.ref();
    bothor_.ref();
}


FaultHorizon2DLocationField::~FaultHorizon2DLocationField()
{
    fault_.unRef();
    tophor_.unRef();
    bothor_.unRef();
}


bool FaultHorizon2DLocationField::calculate()
{
    Fault2DSubSampler sampler( fault_ );
    TypeSet<Coord3> crds;
    sampler.getCoordList( SI().zStep(), crds );

    Seis2DLineSet ls( lsioobj_ );
    const int lineidx = ls.indexOf( linename_.buf() );
    PosInfo::Line2DData lineposinfo;
    if ( !ls.getGeometry(lineidx,lineposinfo) )
	return false;

    PosInfo::Line2DPos firstpos, lastpos;
    if ( !lineposinfo.getPos(crds.first(),firstpos) ||
	 !lineposinfo.getPos(crds.last(),lastpos) )
	return false;

    Interval<int> trcrg( firstpos.nr_, lastpos.nr_ ); trcrg.sort();
    CubeSampling cs; cs.hrg.set( Interval<int>(0,0), trcrg );
// TODO: cs.zrg
    setSize( cs.nrCrl(), cs.nrZ() );

    const int lidxtop = tophor_.geometry().lineIndex( linename_ );
    const int lidxbot = bothor_.geometry().lineIndex( linename_ );

    for ( int crlidx=0; crlidx<cs.nrCrl(); crlidx++ )
    {
	const int crl = trcrg.atIndex( crlidx, 1 );
	for ( int zidx=0; zidx<cs.nrZ(); zidx++ )
	{
	    const float zval = cs.zAtIndex( zidx );
	    set( crlidx, zidx, sOutside() );
	}
    }

    return true;
}


} // namespace SSIS
