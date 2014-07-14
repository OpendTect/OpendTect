/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Apr 2010
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "seiscube2linedata.h"
#include "keystrs.h"
#include "posinfo2d.h"
#include "seisread.h"
#include "seiswrite.h"
#include "seis2dline.h"
#include "seis2dlineio.h"
#include "seisbuf.h"
#include "seistrc.h"
#include "seistrctr.h"
#include "seisinfo.h"
#include "survgeom2d.h"
#include "survinfo.h"
#include "ioobj.h"


class Seis2DFrom3DGeomIDProvider : public GeomIDProvider
{
public:
Seis2DFrom3DGeomIDProvider( const Seis2DFrom3DExtractor& extr )
    : extr_(extr)
{}

Pos::GeomID geomID() const
{ return extr_.curGeomID(); }

const Seis2DFrom3DExtractor& extr_;
};


Seis2DFrom3DExtractor::Seis2DFrom3DExtractor(
			const IOObj& cubein, const IOObj& dsout,
			const TypeSet<Pos::GeomID>& geomids )
    : Executor("Extract 3D data into 2D lines")
    , rdr_(*new SeisTrcReader(&cubein))
    , wrr_(*new SeisTrcWriter(&dsout))
    , geomids_(geomids)
    , nrdone_(0)
    , totalnr_(0)
    , curgeom2d_(0)
    , curlineidx_(-1)
    , curtrcidx_(-1)
{
    for ( int idx=0; idx<geomids.size(); idx++ )
    {
	mDynamicCastGet( const Survey::Geometry2D*, geom2d,
			 Survey::GM().getGeometry(geomids[idx]) );
	if ( geom2d )
	    totalnr_ += geom2d->data().positions().size();
    }
}


Seis2DFrom3DExtractor::~Seis2DFrom3DExtractor()
{
    delete &wrr_; delete &rdr_;
}


Pos::GeomID Seis2DFrom3DExtractor::curGeomID() const
{
    return geomids_.validIdx(curlineidx_) ? geomids_[curlineidx_]
					  : Survey::GM().cUndefGeomID();
}

#define mErrRet(s) { msg_ = s; return ErrorOccurred(); }

int Seis2DFrom3DExtractor::goToNextLine()
{
    curlineidx_++;
    if ( curlineidx_ >= geomids_.size() )
	return Finished();

    if ( !curlineidx_ && !rdr_.prepareWork() )
	mErrRet( rdr_.errMsg() )

    mDynamicCast( const Survey::Geometry2D*, curgeom2d_,
		  Survey::GM().getGeometry(geomids_[curlineidx_]) );
    if ( !curgeom2d_ )
	mErrRet( "Line geometry not available" )

    curtrcidx_ = 0;
    const GeomIDProvider* gip = wrr_.geomIDProvider();
    if ( !gip ) gip = new Seis2DFrom3DGeomIDProvider( *this );
    wrr_.setGeomIDProvider( gip );
    return MoreToDo();
}


int Seis2DFrom3DExtractor::nextStep()
{
    if ( !curgeom2d_ || curtrcidx_ >= curgeom2d_->data().positions().size() )
	return goToNextLine();

    return handleTrace();
}


int Seis2DFrom3DExtractor::handleTrace()
{
    const PosInfo::Line2DPos& curpos =
		curgeom2d_->data().positions()[curtrcidx_++];
    if ( !rdr_.seisTranslator()->goTo( SI().transform(curpos.coord_) ) )
	return MoreToDo();

    SeisTrc trc;
    if ( !rdr_.get(trc) )
	return MoreToDo();

    trc.info().nr = curpos.nr_;
    trc.info().coord = curpos.coord_;
    if ( !wrr_.put(trc) )
	mErrRet( wrr_.errMsg() )

    nrdone_++;
    return MoreToDo();
}
class Cube2LineDataLineKeyProvider : public GeomIDProvider
{
public:
Cube2LineDataLineKeyProvider( SeisCube2LineDataExtracter& lde ) : lde_(lde) {}

Pos::GeomID geomID() const
{
    return lde_.usedlinenames_.isEmpty() ? Survey::GM().cUndefGeomID()
		    : Survey::GM().getGeomID(lde_.usedlinenames_[0]->str() );
}

    SeisCube2LineDataExtracter&	lde_;

};


SeisCube2LineDataExtracter::SeisCube2LineDataExtracter(
			const IOObj& cubein, const IOObj& lsout,
			const char* attrnm, const BufferStringSet* lnms )
    : Executor("Extract 3D data into 2D lines")
    , attrnm_(attrnm)
    , tbuf_(*new SeisTrcBuf(true))
    , rdr_(*new SeisTrcReader(&cubein))
    , ls_(*new Seis2DLineSet(lsout))
    , wrr_(*new SeisTrcWriter(&lsout))
    , nrdone_(0)
    , totalnr_( 0 )
    , c2ldlkp_(0)
{
    if ( lnms ) lnms_ = *lnms;
}


SeisCube2LineDataExtracter::~SeisCube2LineDataExtracter()
{
    closeDown();
    delete &tbuf_; delete &wrr_; delete &rdr_; delete &ls_;
}


void SeisCube2LineDataExtracter::closeDown()
{
    tbuf_.deepErase();
    rdr_.close(); wrr_.close();
    deepErase( fetchers_ );
    usedlinenames_.erase();
    delete c2ldlkp_; c2ldlkp_ = 0;
    wrr_.setGeomIDProvider(0);
}


int SeisCube2LineDataExtracter::nextStep()
{
    if ( fetchers_.isEmpty() )
    {
	if ( !rdr_.prepareWork() )
	    msg_ = rdr_.errMsg();
	else if ( ls_.nrLines() < 1 )
	    msg_ = "Empty or invalid Line Set";
	else
	{
	    c2ldlkp_ = new Cube2LineDataLineKeyProvider( *this );
	    wrr_.setGeomIDProvider( c2ldlkp_ );
	    msg_ = "Handling traces";
	}

	return getFetchers()
	    ? MoreToDo()
	    : ErrorOccurred();
    }

    int res = fetchers_[0]->doStep();
    if ( res != 1 )
    {
	if ( res > 1 )
	    return res;
	else if ( res == 0 )
	{
	    delete fetchers_.removeSingle( 0 );
	    usedlinenames_.removeSingle( 0 );
	    return fetchers_.isEmpty() ? Finished() : MoreToDo();
	}
	else
	{
	    msg_ = fetchers_[0]->uiMessage();
	    return ErrorOccurred();
	}
    }

    res = handleTrace();
    msg_ = fetchers_[0]->uiMessage();
    return res;
}


bool SeisCube2LineDataExtracter::getFetchers()
{
    totalnr_ = 0;
    usedlinenames_.erase();
    
    for ( int lidx=0; lidx<ls_.nrLines(); lidx++ )
    {
	const BufferString lnm = ls_.lineName( lidx );
	if ( usedlinenames_.isPresent( lnm ) )
	    continue;
	if ( !lnms_.isEmpty() && !lnms_.isPresent(lnm) )
	    continue;

	int inplidx = -1;
	const LineKey deflk( lnm );
	if ( ls_.lineKey(lidx) == deflk )
	    inplidx = lidx;
	else
	{
	    for ( int iln=0; iln<ls_.nrLines(); iln++ )
	    {
		if ( ls_.lineKey(iln) == deflk )
		    { inplidx = iln; break; }
	    }
	}
	if ( inplidx < 0 )
	    inplidx = lidx;

	Executor* fetcher = ls_.lineFetcher( inplidx, tbuf_, 1 );
	if ( fetcher )
	{
	    totalnr_ += fetcher->totalNr();
	    fetchers_ += fetcher;
	    usedlinenames_.add( lnm );
	}
    }

    return !fetchers_.isEmpty();
}


int SeisCube2LineDataExtracter::handleTrace()
{
    SeisTrc* trc = tbuf_.remove( 0 );
    SeisTrcInfo ti( trc->info() );
    delete trc;

    if ( !rdr_.seisTranslator()->goTo( SI().transform(ti.coord) ) )
	return MoreToDo();

    SeisTrc trc3d;
    if ( !rdr_.get(trc3d) )
	return MoreToDo();

    ti.sampling = trc3d.info().sampling;
    trc3d.info() = ti;
    if ( !wrr_.put(trc3d) )
	{ msg_ = wrr_.errMsg(); return ErrorOccurred(); }

    nrdone_++;
    return MoreToDo();
}
