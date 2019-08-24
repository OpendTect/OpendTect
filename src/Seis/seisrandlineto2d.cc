/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Raman Singh
 Date:		May 2008
________________________________________________________________________

-*/

#include "trckeyzsampling.h"
#include "iopar.h"
#include "ioobj.h"
#include "progressmeter.h"
#include "seisioobjinfo.h"
#include "seisrandlineto2d.h"
#include "randomlinegeom.h"
#include "seisbuf.h"
#include "seisprovider.h"
#include "seistableseldata.h"
#include "seistrc.h"
#include "seisstorer.h"
#include "survinfo.h"
#include "od_ostream.h"
#include "uistrings.h"


SeisRandLineTo2D::SeisRandLineTo2D( const IOObj& inobj, const IOObj& outobj,
				    const Pos::GeomID geomid, int trcinit,
				    const Geometry::RandomLine& rln )
    : Executor("Saving 2D Line")
    , geomid_(geomid)
    , prov_(0)
    , storer_(0)
    , nrdone_(0)
    , seldata_(*new Seis::TableSelData)
{
    uiRetVal uirv;
    prov_ = Seis::Provider::create( inobj, &uirv );
    if ( !prov_ )
	{ errmsg_ = uirv; return; }
    if ( rln.nrNodes() < 2 )
	{ errmsg_ = mINTERNAL("Empty random line"); return; }
    storer_ = new Seis::Storer( outobj );
    if ( !storer_->isUsable() )
    {
	errmsg_ = storer_->errNotUsable();
	deleteAndZeroPtr(storer_);
	return;
    }

    SeisIOObjInfo inpsi( inobj );
    TrcKeyZSampling inpcs;
    inpsi.getRanges( inpcs );
    const BinID inpstep = inpcs.hsamp_.step_;

    int trcnr = trcinit;
    const Interval<float> zrg = rln.zRange();
    TypeSet<float> vals( 4, 0 );
    BinID startbid = rln.nodePosition( 0 );
    Coord startpos = SI().transform( startbid );
    vals[0] = zrg.start;
    vals[1] = (float) startpos.x_; vals[2] = (float) startpos.y_;
    vals[3] = (float)trcnr;
    seldata_.binidValueSet().allowDuplicatePositions( true );
    seldata_.binidValueSet().setNrVals( 4 );
    seldata_.binidValueSet().add( startbid, vals );
    trcnr += 1;
    for ( int idx=1; idx<rln.nrNodes(); idx++ )
    {
	const BinID& stopbid = rln.nodePosition( idx );
	Coord stoppos = SI().transform( stopbid );
	const double dist = startpos.distTo<double>( stoppos );
	const double unitdist = mMAX( inpstep.inl() * SI().inlDistance(),
				      inpstep.crl() * SI().crlDistance() );
	const int nrsegs = mNINT32( dist / unitdist );
	const double unitx = ( stoppos.x_ - startpos.x_ ) / nrsegs;
	const double unity = ( stoppos.y_ - startpos.y_ ) / nrsegs;
	for ( int nidx=1; nidx<nrsegs; nidx++ )
	{
	    const double curx = startpos.x_ + nidx * unitx;
	    const double cury = startpos.y_ + nidx * unity;
	    Coord curpos( curx, cury );
	    vals[0] = zrg.start;
	    vals[1] = (float) curpos.x_; vals[2] = (float) curpos.y_;
	    vals[3] = (float)trcnr;
	    const BinID curbid = SI().transform(curpos);
	    seldata_.binidValueSet().add( curbid, vals );
	    trcnr += 1;
	}

	vals[0] = zrg.stop;
	vals[1] = (float) stoppos.x_; vals[2] = (float) stoppos.y_;
	vals[3] = (float)trcnr;
	seldata_.binidValueSet().add( stopbid, vals );
	trcnr += 1;
	startbid = stopbid;
	startpos = stoppos;
    }

    totnr_ = mCast( int, seldata_.binidValueSet().totalSize() );
    if ( prov_ )
	prov_->setSelData( new Seis::TableSelData(seldata_) );

    seldata_.binidValueSet().next( pos_ );
    buf_ = new SeisTrcBuf( true );
}


SeisRandLineTo2D::~SeisRandLineTo2D()
{
    delete prov_; delete storer_;
    delete &seldata_; delete buf_;
}


static void addTrcToBuffer( SeisTrc* trc, SeisTrcBuf* buf )
{
    if ( !trc || !buf )
	return;

    for ( int idx=0; idx<buf->size(); idx++ )
    {
	const SeisTrc* buftrc = buf->get( idx );
	if ( buftrc->info().trcNr() > trc->info().trcNr() )
	    { buf->insert( trc, idx ); return; }
    }

    buf->add( trc );
}


bool SeisRandLineTo2D::writeTraces()
{
    if ( !buf_ || !storer_ )
	return false;

    bool res = true;
    for ( int idx=0; idx<buf_->size(); idx++ )
    {
	const auto uirv = storer_->put( *buf_->get(idx) );
	if ( !uirv.isOK() )
	    { errmsg_ = uirv; res = false; break; }
    }

    return res;
}


int SeisRandLineTo2D::nextStep()
{
    if ( !prov_ || !storer_ || !totnr_ )
	return Executor::ErrorOccurred();

    SeisTrc* trc = new SeisTrc;
    const uiRetVal uirv = prov_->getNext( *trc );
    if ( !uirv.isOK() )
    {
	if ( isFinished(uirv) )
	    writeTraces() ? Executor::Finished() : Executor::ErrorOccurred();

	return Executor::ErrorOccurred();
    }

    BinID bid = trc->info().binID();
    bool geommatching = false;
    do
    {
	if ( seldata_.binidValueSet().getBinID(pos_) == bid )
	{
	    geommatching = true;
	    break;
	}
    } while ( seldata_.binidValueSet().next(pos_) );

    if ( !geommatching )
	return Executor::ErrorOccurred();

    float vals[4];
    seldata_.binidValueSet().get( pos_, bid, vals );
    const Coord coord( vals[1], vals[2] );
    const int trcnr = mNINT32( vals[3] );
    trc->info().setPos( Bin2D(geomid_,trcnr) );
    trc->info().coord_ = coord;
    addTrcToBuffer( trc, buf_ );

    nrdone_++;
    while ( seldata_.binidValueSet().next(pos_) )
    {
	const BinID nextbid = seldata_.binidValueSet().getBinID( pos_ );
	if ( nextbid != bid )
	    break;

	seldata_.binidValueSet().get( pos_, bid, vals );
	const Coord nextcoord( vals[1], vals[2] );
	SeisTrc* nexttrc = new SeisTrc( *trc );
	const int nexttrcnr = mNINT32( vals[3] );
	nexttrc->info().setPos( Bin2D(geomid_,nexttrcnr) );
	nexttrc->info().coord_ = nextcoord;
	addTrcToBuffer( nexttrc, buf_ );
	nrdone_++;
    }

    return Executor::MoreToDo();
}


uiString SeisRandLineTo2D::message() const
{ return errmsg_.isEmpty() ? m3Dots(tr("Writing traces")) : errmsg_; }

uiString SeisRandLineTo2D::nrDoneText() const
{ return tr("Traces written"); }

od_int64 SeisRandLineTo2D::nrDone() const
{ return nrdone_; }

od_int64 SeisRandLineTo2D::totalNr() const
{ return totnr_; }


#define mNotOKRet(s) \
	{ isok_ = false; strm_ << s << od_endl; return; }

SeisRandLineTo2DGrid::SeisRandLineTo2DGrid( const IOPar& par, od_ostream& s )
    : isok_(true),strm_(s)
    , inpobj_(0),outpobj_(0)
    , rln_(*new Geometry::RandomLine)
{
    rln_.ref();

    DBKey inpid, outpid;
    if ( !par.get(SeisRandLineTo2DGrid::sKeyInputID(),inpid) )
	mNotOKRet("Error: Input ID is missing")

    if ( !par.get(SeisRandLineTo2DGrid::sKeyOutputID(),outpid) )
	mNotOKRet("Error: Output ID is missing")

    inpobj_ = inpid.getIOObj();
    if ( !inpobj_ )
	mNotOKRet("Error: Input seismic cube cannot be found")

    outpobj_ = outpid.getIOObj();
    if ( !outpobj_ )
	mNotOKRet("Error: Output dataset cannot be found")

    const char* parpref = par.find( SeisRandLineTo2DGrid::sKeyParPrefix() );
    parprefix_ = parpref && *parpref ? parpref : "Parallel";
    const char* perpref = par.find( SeisRandLineTo2DGrid::sKeyPerpPrefix() );
    perprefix_ = perpref && *perpref ? perpref : "Perpendicular";

    if ( !par.get(SeisRandLineTo2DGrid::sKeyGridSpacing(),gridspacing_) )
	mNotOKRet("Error: Grid spacing not specified")

    PtrMan<IOPar> randlnpar = par.subselect(
				SeisRandLineTo2DGrid::sKeyRandomLine() );
    if ( !randlnpar )
	mNotOKRet("Error: Base Random line missing")

    BinID start, stop;
    if ( !randlnpar->get(SeisRandLineTo2DGrid::sKeyStartBinID(),start)
	    || !randlnpar->get(SeisRandLineTo2DGrid::sKeyStopBinID(),stop) )
	mNotOKRet("Error: Base Random line definition incomplete")

    rln_.addNode( start );
    rln_.addNode( stop );
}


SeisRandLineTo2DGrid::~SeisRandLineTo2DGrid()
{
    rln_.unRef();
}


#undef mNotOKRet
#define mFalseRet(s) { strm_ << s << od_endl; return false; }

bool SeisRandLineTo2DGrid::createGrid()
{
    TrcKeyZSampling cs;
    SeisIOObjInfo info( inpobj_ );
    info.getRanges( cs );
    Geometry::RandomLineSet parset( rln_, gridspacing_, true );
    Geometry::RandomLineSet perpset( rln_, gridspacing_, false );
    parset.limitTo( cs );
    perpset.limitTo( cs );
    if ( !parset.size() && !perpset.size() )
	mFalseRet("Error: failed to generate grid lines")

    return mk2DLines(parset,true) && mk2DLines(perpset,false);
}


bool SeisRandLineTo2DGrid::mk2DLines( const Geometry::RandomLineSet& rlset,
				      bool parll )
{
    BufferString strsuffix;
    int numsuffix = 1;
    for ( int idx=0; idx<rlset.size(); idx++ )
    {
	const Geometry::RandomLine* rln = rlset.lines()[idx];
	if ( !rln || rln->nrNodes() != 2 )
	    continue;

	BufferString linenm( parll ? parprefix_ : perprefix_ );
	strsuffix = numsuffix++;
	if ( strsuffix.size() == 1 )
	    linenm += "00";
	else if ( strsuffix.size() == 2 )
	    linenm += "0";
	linenm += strsuffix;
	Pos::GeomID geomid = SurvGeom::getGeomID( linenm );
	SeisRandLineTo2D exec( *inpobj_, *outpobj_, geomid, 1, *rln );
	strm_ << "Creating 2D line " << linenm << ":" << od_endl;
	if ( !exec.go(strm_) )
	    strm_ << "Failedto create line " << linenm << od_endl;
    }

    strm_ << "Finished processing." << od_endl;
    if ( !SI().has2D() )
    {
	strm_ << "Please note that you need to change SURVEY TYPE\n"
		<< " to 'Both 2D and 3D' to display the 2D lines";
	strm_.flush();
    }

    return true;
}

#undef mFalseRet
