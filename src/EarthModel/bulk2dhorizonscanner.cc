/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Prajjaval Singh
 Date:		March 2021
________________________________________________________________________

-*/

#include "bulk2dhorizonscanner.h"
#include "binidvalset.h"
#include "emhorizon2d.h"
#include "emhorizonascio.h"
#include "file.h"
#include "ioman.h"
#include "ioobj.h"
#include "iopar.h"
#include "keystrs.h"
#include "oddirs.h"
#include "posidxpairvalset.h"
#include "ranges.h"
#include "survgeom2d.h"
#include "survinfo.h"
#include "trckeyzsampling.h"
#include "tabledef.h"

#include "uistrings.h"

namespace EM
{

BulkHorizon2DScanner::BulkHorizon2DScanner( const BufferStringSet& fnms,
				    Table::FormatDesc& fd )
    : Executor("Scan horizon file(s)")
    , fd_(fd)
    , ascio_(nullptr)
    , bvalset_(nullptr)
    , fileidx_(0)
    , curlinegeom_(nullptr)
{
    filenames_ = fnms;
    init();
}


BulkHorizon2DScanner::~BulkHorizon2DScanner()
{
    deleteAndZeroPtr( ascio_ );
    deleteAndZeroPtr( bvalset_ );
    data_.setEmpty();
}


void BulkHorizon2DScanner::init()
{
    totalnr_ = -1;
}


uiString BulkHorizon2DScanner::uiMessage() const
{
    return msg_;
}


uiString BulkHorizon2DScanner::uiNrDoneText() const
{
    return tr("Positions handled");
}


od_int64 BulkHorizon2DScanner::nrDone() const
{
    return nrdone_;
}


od_int64 BulkHorizon2DScanner::totalNr() const
{
    if ( totalnr_ > 0 )
	return totalnr_;

    totalnr_ = 0;
    for ( int idx=0; idx<filenames_.size(); idx++ )
    {
	od_istream strm( filenames_.get(0).buf() );
	if ( !strm.isOK() )
	    continue;

	BufferString line;
	while ( strm.isOK() )
	{
	    strm.getLine( line );
	    totalnr_++;
	}

	totalnr_ -= fd_.nrhdrlines_;
    }

    return totalnr_;
}


void BulkHorizon2DScanner::report( IOPar& iopar ) const
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
    iopar.add( "Rejected line names", invalidnms_.getDispString(-1,false) );

    iopar.set( "Total number of positions", totalnr_ );
    iopar.add( "Value ranges", "" );
    for ( int kidx=0; kidx<hornmset_.size(); kidx++ )
    {
	iopar.add( "Horizon Name", hornmset_.get(kidx) );
	TypeSet<Interval<float>> valranges = valrangesset_[kidx];
	iopar.add( "Line names", validnms_[kidx].getDispString(-1,false) );

	for ( int idx=0; idx<valranges.size(); idx++ )
	    iopar.set( validnms_[kidx].get(idx), valrangesset_[kidx][idx] );
    }
}


const char* BulkHorizon2DScanner::defaultUserInfoFile()
{
    mDeclStaticString( ret );
    ret = GetProcFileName( "scan_horizon" );
    if ( GetSoftwareUser() )
    {
	ret += "_";
	ret += GetSoftwareUser();
    }

    ret += ".txt";
    return ret.buf();
}


void BulkHorizon2DScanner::launchBrowser( const char* fnm ) const
{
    if ( !fnm || !*fnm )
	fnm = defaultUserInfoFile();

    IOPar iopar; report( iopar );
    iopar.write( fnm, IOPar::sKeyDumpPretty() );

    File::launchViewer( fnm );
}


bool BulkHorizon2DScanner::reInitAscIO( const char* fnm )
{
    deleteAndZeroPtr( ascio_ );
    ascio_ = new EM::BulkHorizon2DAscIO( fd_, fnm );
    if ( !ascio_->isOK() )
    {
	msg_ = uiStrings::phrCannotOpenForRead( fnm );
	deleteAndZeroPtr( ascio_ );
	return false;
    }
    return true;
}


int BulkHorizon2DScanner::nextStep()
{
    if ( fileidx_ >= filenames_.size() )
	return Executor::Finished();

    if ( !ascio_ && !reInitAscIO( filenames_.get(fileidx_).buf() ) )
	return Executor::ErrorOccurred();

    BufferString hornm;
    BufferString linenm;
    float spnr = mUdf(float);
    Coord3 crd(Coord3::udf());
    int trcnr = mUdf(int);
    const int ret = ascio_->getData( hornm, linenm, crd, trcnr, spnr );
    if ( ret < 0 )
    {
	msg_ = tr("Error while reading file %1: %2")
			.arg(filenames_.get(fileidx_)).arg(ascio_->errMsg());
	return Executor::ErrorOccurred();
    }

    if ( ret == 0 )
    {
	fileidx_++;
	deleteAndZeroPtr( ascio_ );
	nrdone_++;
	return Executor::MoreToDo();
    }

    const bool isnewhor = prevhornm_ != hornm;


    if ( prevhornm_.isEmpty() || isnewhor )
	prevhornm_ = hornm;

    if ( hornm.isEmpty() )
    {
	nrdone_++;
	return Executor::MoreToDo();
    }
    else
	hornmset_.addIfNew( hornm );

    if ( isnewhor )
    {
	horidx_++;
	validnms_.add( BufferStringSet() );
	valrangesset_.add( TypeSet<ZGate>() );
    }

    if ( linenm )
    {
	if ( invalidnms_.isPresent(linenm) )
	{
	    nrdone_++;
	    return Executor::MoreToDo();
	}

	mDynamicCastGet(const Survey::Geometry2D*,linegeom,
		     Survey::GM().getGeometry(linenm))

	if ( !linegeom )
	{
	    invalidnms_.addIfNew( linenm );
	    nrdone_++;
	    return Executor::MoreToDo();
	}

	validnms_[horidx_].addIfNew(linenm);
	curlinegeom_ = linegeom;
    }

    bool isvalidpos = false;
    if ( ascio_->isTrcNr() )
	isvalidpos = curlinegeom_->getPosByTrcNr( trcnr, crd, spnr );
    else if ( !mIsUdf(spnr) )
	isvalidpos = curlinegeom_->getPosBySPNr( spnr, crd, trcnr );
    else if ( crd.isDefined() )
	isvalidpos = curlinegeom_->getPosByCoord( crd, trcnr, spnr );

    if ( !isvalidpos )
    {
	nrdone_++;
	return Executor::MoreToDo();
    }

    int validx = 0;
    if ( !bvalset_ || isnewhor )
    {
	bvalset_ = new BinIDValueSet( 1, false );
	data_ += bvalset_;
    }

    const int nrvals = 1;
    Interval<float> validzrg( curlinegeom_->data().zRange().start,
			      curlinegeom_->data().zRange().stop );
    validzrg.widen( validzrg.width() );
    while ( validx < nrvals )
    {
	while ( valrangesset_[horidx_].size() < nrvals )
	    valrangesset_[horidx_] += Interval<float>(mUdf(float),-mUdf(float));

	const float zval = crd.z;

	if ( mIsUdf(zval) || !validzrg.includes(zval,false) )
	    crd.z = mUdf(float);
	else
	    valrangesset_[horidx_][validx].include( zval, false );

	validx++;
    }

    if ( mIsUdf(crd.z) )
    {
	nrdone_++;
	return Executor::MoreToDo();
    }

    const int lineidx = validnms_[horidx_].indexOf( linenm );
    const BinID bid( lineidx, trcnr );
    bvalset_->add( bid, crd.z );
    if ( isnewhor )
	prevhornm_ = hornm;

    nrdone_++;
    return Executor::MoreToDo();
}


void BulkHorizon2DScanner::getLineNames( TypeSet<BufferStringSet>& nms ) const
{
    nms = validnms_;
}

void BulkHorizon2DScanner::getHorizonName( BufferStringSet& nms ) const
{
    nms = hornmset_;
}


bool BulkHorizon2DScanner::hasGaps()
{
    bool hasgap = false;
    for ( int idx=0; idx<data_.size(); idx++ )
    {
	const BinIDValueSet* bvalset = data_.get( idx );
	if ( !bvalset || bvalset->isEmpty() )
	    continue;

	const Interval<int> rowrng = bvalset->rowRange();
	for ( int rowidx=rowrng.start; rowidx<=rowrng.stop; rowidx++ )
	{
	    const Interval<int> colrng = bvalset->colRange( rowidx );
	    TypeSet<int> seconds;
	    bvalset->getSecondsAtIdx( rowidx, seconds );
	    if ( colrng.width()+1 != seconds.size() )
	    {
		hasgap = true;
		break;
	    }
	}
    }

    return hasgap;
}
}
