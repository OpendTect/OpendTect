/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "horizon2dscanner.h"
#include "binidvalset.h"
#include "emhorizon2d.h"
#include "emhorizonascio.h"
#include "ioman.h"
#include "ioobj.h"
#include "iopar.h"
#include "file.h"
#include "survinfo.h"
#include "oddirs.h"
#include "trckeyzsampling.h"
#include "keystrs.h"
#include "tabledef.h"
#include "uistrings.h"
#include "survgeom2d.h"


Horizon2DScanner::Horizon2DScanner( const BufferStringSet& fnms,
				    Table::FormatDesc& fd )
    : Executor("Scan horizon file(s)")
    , fd_(fd)
    , ascio_(nullptr)
    , bvalset_(nullptr)
    , fileidx_(0)
    , curlinegeom_(nullptr)
    , istracenr_(false)
    , msg_(uiStrings::sScanning())
{
    filenames_ = fnms;
    init();
}


Horizon2DScanner::~Horizon2DScanner()
{}


void Horizon2DScanner::init()
{
    totalnr_ = -1;
    valranges_.erase();
}


uiString Horizon2DScanner::uiMessage() const
{
    return msg_;
}


uiString Horizon2DScanner::uiNrDoneText() const
{
    return tr("Positions handled");
}


od_int64 Horizon2DScanner::nrDone() const
{
    return bvalset_ ? bvalset_->totalSize() : 0;
}


od_int64 Horizon2DScanner::totalNr() const
{
    if ( totalnr_ > 0 ) return totalnr_;

    totalnr_ = 0;
    for ( int idx=0; idx<filenames_.size(); idx++ )
    {
	od_istream strm( filenames_.get(0).buf() );
	if ( !strm.isOK() ) continue;

	BufferString line;
	while ( strm.isOK() )
	    { strm.getLine( line ); totalnr_++; }

	totalnr_ -= fd_.nrhdrlines_;
    }

    return totalnr_;
}


void Horizon2DScanner::report( IOPar& iopar ) const
{
    iopar.setEmpty();

    BufferString str = "Report for horizon file(s):\n";
    for ( int idx=0; idx<filenames_.size(); idx++ )
	str.add( filenames_.get(idx).buf() ).add( "\n" );
    str += "\n\n";
    iopar.setName( str.buf() );

    iopar.add( IOPar::sKeyHdr(), "Geometry" );
    const int nrlines = validnms_.size();
    if ( !nrlines )
    {
	iopar.add( IOPar::sKeySubHdr(), "No valid line names found\n"
		   "Make sure the line names match with those in survey" );
	return;
    }

    iopar.set( "Valid lines found", nrlines );
    iopar.add( "Line names", validnms_.getDispString(-1,false) );
    iopar.add( "Rejected line names", invalidnms_.getDispString(-1,false) );

    const int nrpos = mCast( int, bvalset_ ? bvalset_->totalSize() : 0 );
    if ( !nrpos )
    {
	iopar.add( "No valid positions found",
		   "Please re-examine input files and format definition" );
	return;
    }

    iopar.set( "Total number of positions", nrpos );
    iopar.add( "Value ranges", "" );
    for ( int idx=0; idx<valranges_.size(); idx++ )
	iopar.set( fd_.bodyinfos_[idx+3]->name(), valranges_[idx] );
}


const char* Horizon2DScanner::defaultUserInfoFile()
{
    mDeclStaticString( ret );
    ret = GetProcFileName( "scan_horizon" );
    if ( GetSoftwareUser() )
	{ ret += "_"; ret += GetSoftwareUser(); }
    ret += ".txt";
    return ret.buf();
}


void Horizon2DScanner::launchBrowser( const char* fnm ) const
{
    if ( !fnm || !*fnm )
	fnm = defaultUserInfoFile();
    IOPar iopar; report( iopar );
    iopar.write( fnm, IOPar::sKeyDumpPretty() );

    File::launchViewer( fnm );
}


bool Horizon2DScanner::reInitAscIO( const char* fnm )
{
    deleteAndZeroPtr( ascio_ );
    ascio_ = new EM::Horizon2DAscIO( fd_, fnm );
    if ( !ascio_->isOK() )
    {
	msg_ = uiStrings::phrCannotOpenForRead( fnm );
	deleteAndZeroPtr( ascio_ );
	return false;
    }
    return true;
}


int Horizon2DScanner::nextStep()
{
    if ( fileidx_ >= filenames_.size() )
	return Executor::Finished();

    if ( !ascio_ && !reInitAscIO( filenames_.get(fileidx_).buf() ) )
	return Executor::ErrorOccurred();

    BufferString linenm;
    Coord crd;
    int nr = mUdf(int);
    float spnr = mUdf(float);
    TypeSet<float> data;
    const int ret = ascio_->getNextLine( linenm, crd, nr, spnr, data );
    if ( ret < 0 )
    {
	msg_ = tr( "Error while reading file %1: %2" )
	    	.arg( filenames_.get(fileidx_) ).arg( ascio_->errMsg() );
	return Executor::ErrorOccurred();
    }
    if ( ret == 0 )
    {
	if ( !istracenr_ )
	    istracenr_ = ascio_->isTraceNr();

	fileidx_++;
	deleteAndZeroPtr( ascio_ );
	return Executor::MoreToDo();
    }

    if ( curline_.isEmpty() || curline_ != linenm )
    {
	if ( invalidnms_.isPresent(linenm) )
	    return Executor::MoreToDo();

	mDynamicCastGet(const Survey::Geometry2D*,linegeom,
			Survey::GM().getGeometry(linenm.buf()))

	if ( !linegeom )
	{
	    invalidnms_.addIfNew( linenm );
	    return Executor::MoreToDo();
	}

	validnms_.addIfNew( linenm );
	curline_ = linenm;
	curlinegeom_ = linegeom;
    }

    bool isvalidpos = false;
    if ( !mIsUdf(nr) )
	isvalidpos = curlinegeom_->getPosByTrcNr( nr, crd, spnr );
    else if ( !mIsUdf(spnr) )
	isvalidpos = curlinegeom_->getPosBySPNr( spnr, crd, nr );
    else if ( crd.isDefined() )
	isvalidpos = curlinegeom_->getPosByCoord( crd, nr, spnr );

    if ( !isvalidpos )
	return Executor::MoreToDo();

    if ( !bvalset_ )
	bvalset_ = new BinIDValueSet( data.size(), false );

    int validx = 0;
    const int nrvals = data.size();

    Interval<float> validzrg( curlinegeom_->data().zRange().start,
			      curlinegeom_->data().zRange().stop );
    validzrg.widen( validzrg.width() );
    while ( validx < nrvals )
    {
	while ( valranges_.size() < nrvals )
	    valranges_ += Interval<float>(mUdf(float),-mUdf(float));

	const float attrval = data[validx];

	if ( mIsUdf(attrval) || !validzrg.includes(attrval,false) )
	    data[validx] = mUdf(float);
	else
	    valranges_[validx].include( attrval, false );

	validx++;
    }

    const int lineidx = validnms_.indexOf( linenm );
    const BinID bid( lineidx, nr );
    bvalset_->add( bid, data.arr() );

    return Executor::MoreToDo();
}


bool Horizon2DScanner::getLineNames( BufferStringSet& nms ) const
{
    nms = validnms_;
    return nms.size();
}


bool Horizon2DScanner::hasGaps()
{
    BufferStringSet lnms;
    getLineNames( lnms );
    int totalpos = 0;
    for ( int idx=0; idx<lnms.size(); idx++ )
    {
	mDynamicCastGet( const Survey::Geometry2D*, curlinegeom,
		      Survey::GM().getGeometry(lnms.get(idx).buf()) );

	PosInfo::Line2DData l2ddata =  curlinegeom->data();
	StepInterval<Pos::TraceID> trcrg = l2ddata.trcNrRange();
	totalpos += trcrg.nrSteps() + 1;
    }

    return totalpos != bvalset_->totalSize();
}
