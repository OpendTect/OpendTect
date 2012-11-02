/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Raman Singh
 Date:		May 2008
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "cubesampling.h"
#include "ioman.h"
#include "iopar.h"
#include "progressmeter.h"
#include "seisioobjinfo.h"
#include "seisrandlineto2d.h"
#include "randomlinegeom.h"
#include "seisbuf.h"
#include "seisread.h"
#include "seisselectionimpl.h"
#include "seistrc.h"
#include "seiswrite.h"
#include "survinfo.h"


SeisRandLineTo2D::SeisRandLineTo2D( const IOObj& inobj, const IOObj& outobj,
				    const LineKey& lk, int trcinit,
				    const Geometry::RandomLine& rln )
    : Executor("Saving 2D Line")
    , rdr_(0)
    , wrr_(0)
    , nrdone_(0)
    , seldata_(*new Seis::TableSelData)
{
    rdr_ = new SeisTrcReader( &inobj );
    wrr_ = new SeisTrcWriter( &outobj );
    Seis::SelData* seldata = Seis::SelData::get( Seis::Range );
    if ( seldata )
    {
	seldata->lineKey() = lk;
	wrr_->setSelData( seldata );
    }

    if ( rln.nrNodes() < 2 ) return;

    SeisIOObjInfo inpsi( inobj );
    CubeSampling inpcs;
    inpsi.getRanges( inpcs );
    const BinID inpstep = inpcs.hrg.step;

    int trcnr = trcinit;
    const Interval<float> zrg = rln.zRange();
    TypeSet<float> vals( 4, 0 );
    BinID startbid = rln.nodePosition( 0 );
    Coord startpos = SI().transform( startbid );
    vals[0] = zrg.start;
    vals[1] = (float) startpos.x; vals[2] = (float) startpos.y;
    vals[3] = (float)trcnr;
    seldata_.binidValueSet().allowDuplicateBids( true );
    seldata_.binidValueSet().setNrVals( 4 );
    seldata_.binidValueSet().add( startbid, vals );
    trcnr += 1;
    for ( int idx=1; idx<rln.nrNodes(); idx++ )
    {
	const BinID& stopbid = rln.nodePosition( idx );
	Coord stoppos = SI().transform( stopbid );
	const double dist = startpos.distTo( stoppos );
	const double unitdist = mMAX( inpstep.inl * SI().inlDistance(),
				      inpstep.crl * SI().crlDistance() );
	const int nrsegs = mNINT32( dist / unitdist );
	const double unitx = ( stoppos.x - startpos.x ) / nrsegs;
	const double unity = ( stoppos.y - startpos.y ) / nrsegs;
	for ( int nidx=1; nidx<nrsegs; nidx++ )
	{
	    const double curx = startpos.x + nidx * unitx;
	    const double cury = startpos.y + nidx * unity;
	    Coord curpos( curx, cury );
	    vals[0] = zrg.start;
	    vals[1] = (float) curpos.x; vals[2] = (float) curpos.y;
	    vals[3] = (float)trcnr;
	    const BinID curbid = SI().transform(curpos);
	    seldata_.binidValueSet().add( curbid, vals );
	    trcnr += 1;
	}

	vals[0] = zrg.stop;
	vals[1] = (float) stoppos.x; vals[2] = (float) stoppos.y;
	vals[3] = (float)trcnr;
	seldata_.binidValueSet().add( stopbid, vals );
	trcnr += 1;
	startbid = stopbid;
	startpos = stoppos;
    }

    totnr_ = mCast( int, seldata_.binidValueSet().totalSize() );
    if ( rdr_ )
	rdr_->setSelData( new Seis::TableSelData(seldata_) );

    seldata_.binidValueSet().next( pos_ );
    buf_ = new SeisTrcBuf( true );
}


SeisRandLineTo2D::~SeisRandLineTo2D()
{
    delete rdr_; delete wrr_;
    delete &seldata_; delete buf_;
}


static void addTrcToBuffer( SeisTrc* trc, SeisTrcBuf* buf )
{
    if ( !trc || !buf ) return;

    for ( int idx=0; idx<buf->size(); idx++ )
    {
	const SeisTrc* buftrc = buf->get( idx );
	if ( buftrc->info().nr > trc->info().nr )
	{
	    buf->insert( trc, idx );
	    return;
	} 
    }
    
    buf->add( trc );
}


bool SeisRandLineTo2D::writeTraces()
{
    if ( !buf_ || !wrr_ ) return false;

    bool res = true;
    for ( int idx=0; idx<buf_->size(); idx++ )
    {
	SeisTrc* trc = buf_->get( idx );
	if ( !wrr_->put(*trc) )
	    res = false;
    }

    return res;
}


int SeisRandLineTo2D::nextStep()
{
    if ( !rdr_ || !wrr_ || !totnr_ )
	return Executor::ErrorOccurred();

    SeisTrc* trc = new SeisTrc;
    const int rv = rdr_->get( trc->info() );
    if ( rv == 0 ) return writeTraces() ? Executor::Finished()
					: Executor::ErrorOccurred();
    else if ( rv !=1 ) return Executor::ErrorOccurred();

    if ( !rdr_->get(*trc) ) return Executor::ErrorOccurred();

    BinID bid = trc->info().binid;
    bool geommatching = false;
    do
    {
	if ( seldata_.binidValueSet().getBinID(pos_) == bid )
	{
	    geommatching = true;
	    break;
	}
    } while ( seldata_.binidValueSet().next(pos_) );

    if ( !geommatching ) return Executor::ErrorOccurred();

    float vals[4];
    seldata_.binidValueSet().get( pos_, bid, vals );
    const Coord coord( vals[1], vals[2] );
    const int trcnr = mNINT32( vals[3] );
    trc->info().nr = trcnr;
    trc->info().coord = coord;
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
	nexttrc->info().nr = nexttrcnr;
	nexttrc->info().coord = nextcoord;
	addTrcToBuffer( nexttrc, buf_ );
	nrdone_++;
    }

    return Executor::MoreToDo();
}


const char* SeisRandLineTo2D::message() const
{ return "Writing traces..."; }

const char* SeisRandLineTo2D::nrDoneText() const
{ return "Traces written"; }

od_int64 SeisRandLineTo2D::nrDone() const
{ return nrdone_; }

od_int64 SeisRandLineTo2D::totalNr() const
{ return totnr_; }


bool SeisRandLineTo2D::execute( std::ostream* strm, bool b1, bool b2, int i )
{
    if ( !strm )
	return Executor::execute(strm,b1,b2,i);

    TextStreamProgressMeter progressmeter( *strm );
    setProgressMeter( &progressmeter );
    bool res = SequentialTask::execute();
    if ( !res )
	*strm << "Error: " << message() << std::endl;
    else
	*strm << "\nFinished: " << std::endl;

    setProgressMeter( 0 );
    return res;
}

#define mNotOKRet(s) { isok_ = false; strm_ << s << std::endl; return; }
SeisRandLineTo2DGrid::SeisRandLineTo2DGrid( const IOPar& par, std::ostream& s )
    : isok_(true),strm_(s)
    , inpobj_(0),outpobj_(0)
{
    MultiID inpid, outpid;
    if ( !par.get(SeisRandLineTo2DGrid::sKeyInputID(),inpid) )
	mNotOKRet("Error: Input ID is missing")

    if ( !par.get(SeisRandLineTo2DGrid::sKeyOutputID(),outpid) )
	mNotOKRet("Error: Output ID is missing")

    inpobj_ = IOM().get( inpid );
    if ( !inpobj_ )
	mNotOKRet("Error: Input seismic cube cannot be found")

    outpobj_ = IOM().get( outpid );
    if ( !outpobj_ )
	mNotOKRet("Error: Output lineset cannot be found")

    const char* attrnm = par.find( SeisRandLineTo2DGrid::sKeyOutpAttrib() );
    outpattrib_ = attrnm && *attrnm ? attrnm : LineKey::sKeyDefAttrib();
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


#undef mNotOKRet
#define mFalseRet(s) { strm_ << s << std::endl; return false; }
bool SeisRandLineTo2DGrid::createGrid()
{
    CubeSampling cs;
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
	LineKey lk( linenm, outpattrib_.buf() );
	SeisRandLineTo2D exec( *inpobj_, *outpobj_, lk, 1, *rln );
	strm_ << "Creating 2D line " << linenm << ":" << std::endl;
	strm_.flush();
	if ( !exec.execute(&strm_) )
	    strm_ << "Failedto create line " << linenm << std::endl;
    }

    strm_ << "Finished processing." << std::endl;
    if ( !SI().has2D() )
    {
	strm_ << "PLEASE NOTE THAT YOU NEED TO CHANGE SURVEY TYPE" << std::endl;
	strm_ << " TO 'Both 2D and 3D' TO DISPLAY THE 2D LINES" << std::endl;
    }

    return true;
}

#undef mFalseRet
