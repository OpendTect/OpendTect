/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          Feb 2005
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "horizon2dscanner.h"
#include "binidvalset.h"
#include "emhorizon2d.h"
#include "ioman.h"
#include "iopar.h"
#include "strmprov.h"
#include "survinfo.h"
#include "oddirs.h"
#include "cubesampling.h"
#include "keystrs.h"
#include "tabledef.h"
#include "uiseispartserv.h"


Horizon2DScanner::Horizon2DScanner( const BufferStringSet& fnms,
				    const MultiID& setid,
				    Table::FormatDesc& fd )
    : Executor("Scan horizon file(s)")
    , fd_(fd)
    , ascio_(0)
    , setid_(setid)
    , bvalset_(0)
    , fileidx_(0)
{
    filenames_ = fnms;
    init();
}


void Horizon2DScanner::init()
{
    totalnr_ = -1;
    valranges_.erase();
}


const char* Horizon2DScanner::message() const
{
    return "Scanning";
}


const char* Horizon2DScanner::nrDoneText() const
{
    return "Positions handled";
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
	StreamProvider sp( filenames_.get(0).buf() );
	StreamData sd = sp.makeIStream();
	if ( !sd.usable() ) continue;

	char buf[180];
	while ( *sd.istrm )
	{
	    sd.istrm->getline( buf, 180 );
	    totalnr_++;
	}

	sd.close();
	totalnr_ -= fd_.nrhdrlines_;
    }

    return totalnr_;
}


void Horizon2DScanner::report( IOPar& iopar ) const
{
    iopar.setEmpty();

    BufferString str = "Report for horizon file(s):\n";
    for ( int idx=0; idx<filenames_.size(); idx++ )
    {
	str += filenames_.get(idx).buf(); str += "\n";
    }
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
    BufferString msg;
    for ( int idx=0; idx<validnms_.size(); idx++ )
    {
	if ( idx ) msg += ", ";
	msg += validnms_.get( idx );
    }

    iopar.add( "Line names", msg );
    const int nrinvalids = invalidnms_.size();
    if ( nrinvalids )
	{
	msg = "";
	for ( int idx=0; idx<nrinvalids; idx++ )
	{
	    if ( idx ) msg += ", ";
	    msg += invalidnms_.get( idx );
	}

	iopar.add( "Rejected line names", msg );
    }

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
    static BufferString ret;
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

    ExecuteScriptCommand( "od_FileBrowser", fnm );
}


bool Horizon2DScanner::reInitAscIO( const char* fnm )
{
    StreamProvider sp( fnm );
    StreamData sd = sp.makeIStream();
    if ( !sd.usable() )
	return false;

    ascio_ = new EM::Horizon2DAscIO( fd_, *sd.istrm );
    return true;
}


int Horizon2DScanner::nextStep()
{
    if ( fileidx_ >= filenames_.size() )
	return Executor::Finished();

    if ( !ascio_ && !reInitAscIO( filenames_.get(fileidx_).buf() ) )
	return Executor::ErrorOccurred();

    BufferString linenm;
    Coord crd; int trcnr;
    TypeSet<float> data;
    const int ret = ascio_->getNextLine( linenm, crd, trcnr, data );
    if ( ret < 0 ) return Executor::ErrorOccurred();
    if ( ret == 0 ) 
    {
	fileidx_++;
	delete ascio_;
	ascio_ = 0;
	return Executor::MoreToDo();
    }

    if ( curline_.isEmpty() || curline_ != linenm )
    {
	if ( invalidnms_.indexOf(linenm) >= 0 )
	    return Executor::MoreToDo();

	PtrMan<IOObj> lsobj = IOM().get( setid_ );
	if ( !lsobj ) return Executor::ErrorOccurred();

	linegeom_.setEmpty();
	linegeom_.setLineName( linenm );
	S2DPOS().setCurLineSet( lsobj->name() );

	if ( !S2DPOS().getGeometry(linegeom_) )
	{
	    invalidnms_.addIfNew( linenm );
	    return Executor::MoreToDo();
	}

	validnms_.addIfNew( linenm );
	curline_ = linenm;
    }

    if ( linegeom_.isEmpty() )
	return Executor::ErrorOccurred();

    PosInfo::Line2DPos pos;
    if ( !mIsUdf(trcnr) )
    {
	if ( !linegeom_.getPos(trcnr,pos) )
	    return Executor::MoreToDo();

	crd = pos.coord_;
    }
    else if ( crd.isDefined() )
    {
	if ( !linegeom_.getPos(crd,pos,SI().inlDistance()) )
	    return Executor::MoreToDo();
    }
    else
	// no valid x/y nor trcnr
	return Executor::ErrorOccurred();

    if ( !bvalset_ )
	bvalset_ = new BinIDValueSet( data.size(), false );

    int validx = 0;
    const int nrvals = data.size();
    int nrvalidvals = 0;
    Interval<float> validzrg( linegeom_.zRange().start,
			      linegeom_.zRange().stop );
    validzrg.widen( validzrg.width() );
    while ( validx < nrvals )
    {
	while ( valranges_.size() < nrvals )
	    valranges_ += Interval<float>(mUdf(float),-mUdf(float));

	const float val = data[validx];

	if ( mIsUdf(val) || !validzrg.includes(val,false) )
	    data[validx] = mUdf(float);
	else
	{
	    nrvalidvals++;
	    valranges_[validx].include( val, false );
	}

	validx++;
    }
    
    if ( nrvalidvals )
    {
	const int lineidx = validnms_.indexOf( linenm );
	const BinID bid( lineidx, pos.nr_ );
	bvalset_->add( bid, data.arr() );
    }

    return Executor::MoreToDo();
}


bool Horizon2DScanner::getLineNames( BufferStringSet& nms ) const
{
    deepErase( nms );
    nms = validnms_;
    return nms.size();
}
