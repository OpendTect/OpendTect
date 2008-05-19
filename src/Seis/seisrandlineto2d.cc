/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Raman Singh
 Date:		May 2008
 RCS:		$Id: seisrandlineto2d.cc,v 1.2 2008-05-19 06:25:43 cvsraman Exp $
________________________________________________________________________

-*/

#include "seisrandlineto2d.h"
#include "randomlinegeom.h"
#include "seisread.h"
#include "seisselectionimpl.h"
#include "seistrc.h"
#include "seiswrite.h"
#include "survinfo.h"

SeisRandLineTo2D::SeisRandLineTo2D( IOObj* inobj, IOObj* outobj,
				    const LineKey& lk, Interval<int> trcinit,
				    const Geometry::RandomLine& rln )
    : Executor("Saving 2D Line")
    , rdr_(0)
    , wrr_(0)
    , nrdone_(0)
    , seldata_(*new Seis::TableSelData)
{
    rdr_ = new SeisTrcReader( inobj );
    wrr_ = new SeisTrcWriter( outobj );
    Seis::SelData* seldata = Seis::SelData::get( Seis::Range );
    if ( seldata )
    {
	seldata->lineKey() = lk;
	wrr_->setSelData( seldata );
    }

    const int trstart = trcinit.start;
    const int trstep = trcinit.stop;
    if ( rln.nrNodes() < 2 ) return;

    int trcnr = trstart;
    const Interval<float> zrg = rln.zRange();
    TypeSet<float> vals( 4, 0 );
    BinID startbid = rln.nodePosition( 0 );
    Coord startpos = SI().transform( startbid );
    vals[0] = zrg.start;
    vals[1] = startpos.x; vals[2] = startpos.y;
    vals[3] = (float)trcnr;
    seldata_.binidValueSet().allowDuplicateBids( true );
    seldata_.binidValueSet().setNrVals( 4 );
    seldata_.binidValueSet().add( startbid, vals );
    trcnr += trstep;
    for ( int idx=1; idx<rln.nrNodes(); idx++ )
    {
	const BinID& stopbid = rln.nodePosition( idx );
	Coord stoppos = SI().transform( stopbid );
	const double dist = startpos.distTo( stoppos );
	const double unitdist = SI().inlDistance();
	const int nrsegs = mNINT( dist / unitdist );
	const float unitx = ( stoppos.x - startpos.x ) / nrsegs;
	const float unity = ( stoppos.y - startpos.y ) / nrsegs;
	for ( int nidx=1; nidx<nrsegs; nidx++ )
	{
	    const double curx = startpos.x + nidx * unitx;
	    const double cury = startpos.y + nidx * unity;
	    Coord curpos( curx, cury );
	    vals[0] = zrg.start;
	    vals[1] = curpos.x; vals[2] = curpos.y;
	    vals[3] = (float)trcnr;
	    const BinID curbid = SI().transform(curpos);
	    seldata_.binidValueSet().add( curbid, vals );
	    trcnr += trstep;
	}

	vals[0] = zrg.stop;
	vals[1] = stoppos.x; vals[2] = stoppos.y;
	vals[3] = (float)trcnr;
	seldata_.binidValueSet().add( stopbid, vals );
	trcnr += trstep;
	startbid = stopbid;
	startpos = stoppos;
    }

    totnr_ = seldata_.binidValueSet().totalSize();
    if ( rdr_ )
	rdr_->setSelData( new Seis::TableSelData(seldata_) );

    seldata_.binidValueSet().next( pos_ );
}


SeisRandLineTo2D::~SeisRandLineTo2D()
{
    delete rdr_; delete wrr_;
    delete &seldata_;
}


int SeisRandLineTo2D::nextStep()
{
    if ( !rdr_ || !wrr_ || !totnr_ )
	return Executor::ErrorOccurred;

    SeisTrc trc;
    const int rv = rdr_->get( trc.info() );
    if ( rv == 0 ) return Executor::Finished;
    else if ( rv !=1 ) return Executor::ErrorOccurred;

    if ( !rdr_->get(trc) ) return Executor::ErrorOccurred;

    BinID bid = trc.info().binid;
    bool geommatching = false;
    do
    {
	if ( seldata_.binidValueSet().getBinID(pos_) == bid )
	{
	    geommatching = true;
	    break;
	}
    } while ( seldata_.binidValueSet().next(pos_) );

    if ( !geommatching ) return Executor::ErrorOccurred;

    float vals[4];
    seldata_.binidValueSet().get( pos_, bid, vals );
    const Coord coord( vals[1], vals[2] );
    const int trcnr = mNINT( vals[3] );
    trc.info().nr = trcnr;
    trc.info().coord = coord;
    if ( !wrr_->put(trc) )
	return Executor::ErrorOccurred;

    nrdone_++;
    if ( seldata_.binidValueSet().next(pos_) )
    {
	const BinID nextbid = seldata_.binidValueSet().getBinID( pos_ );
	if ( nextbid == bid )
	{
	    seldata_.binidValueSet().get( pos_, bid, vals );
	    const Coord nextcoord( vals[1], vals[2] );
	    const int nexttrcnr = mNINT( vals[3] );
	    trc.info().nr = nexttrcnr;
	    trc.info().coord = nextcoord;
	    if ( !wrr_->put(trc) )
		return Executor::ErrorOccurred;
	}
    }

    return Executor::MoreToDo;
}


const char* SeisRandLineTo2D::message() const
{ return "Writing traces..."; }

const char* SeisRandLineTo2D::nrDoneText() const
{ return "Traces written"; }

int SeisRandLineTo2D::nrDone() const
{ return nrdone_; }

int SeisRandLineTo2D::totalNr() const
{ return totnr_; }
