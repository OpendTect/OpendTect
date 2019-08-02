/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          Feb 2005
________________________________________________________________________

-*/

#include "horizonscanner.h"
#include "emhorizonascio.h"
#include "binnedvalueset.h"
#include "emhorizon3d.h"
#include "posinfodetector.h"
#include "iopar.h"
#include "od_istream.h"
#include "survinfo.h"
#include "oddirs.h"
#include "tabledef.h"
#include "fileview.h"
#include "staticstring.h"


HorizonScanner::HorizonScanner( const BufferStringSet& fnms,
				Table::FormatDesc& fd, bool isgeom )
    : Executor("Scan horizon file(s)")
    , dtctor_(*new PosInfo::Detector(PosInfo::Detector::Setup(false)))
    , fd_(fd)
    , isgeom_(isgeom)
    , ascio_(0)
    , isxy_(false)
    , selxy_(false)
    , bvalset_(0)
    , fileidx_(0)
    , curmsg_(uiStrings::sScanning())
    , nrdone_(0)
    , strm_(0)
{
    filenames_ = fnms;
    init();
}


HorizonScanner::~HorizonScanner()
{
    delete &dtctor_;
    deepErase( sections_ );
    delete strm_;
}


void HorizonScanner::init()
{
    totalnr_ = -1;
    firsttime_ = true;
    valranges_.erase();
    dtctor_.reInit();
    analyseData();
}


uiString HorizonScanner::message() const
{
    return curmsg_;
}


uiString HorizonScanner::nrDoneText() const
{
    return tr("Positions handled");
}


od_int64 HorizonScanner::nrDone() const
{
    return nrdone_;
}


od_int64 HorizonScanner::totalNr() const
{
    if ( totalnr_ > 0 ) return totalnr_;

    totalnr_ = 0;
    for ( int idx=0; idx<filenames_.size(); idx++ )
    {
	od_istream strm( filenames_.get(idx).buf() );
	if ( !strm.isOK() )
	    continue;

	BufferString buf;
	while ( strm.isOK() )
	{
	    strm.getLine( buf );
	    totalnr_++;
	}
	totalnr_ -= fd_.nrhdrlines_;
    }

    return totalnr_;
}


void HorizonScanner::report( IOPar& iopar ) const
{
    iopar.setEmpty();

    const int firstattribidx = isgeom_ ? 1 : 0;
    BufferString str = "Report for horizon file";
    str += filenames_.size() > 1 ? "s:\n" : ": ";
    for ( int idx=0; idx<filenames_.size(); idx++ )
	{ str += filenames_.get(idx).buf(); str += "\n"; }
    str += "\n\n";
    iopar.setName( str.buf() );

    if ( isxy_ != selxy_ )
    {
	iopar.add( IOPar::sKeyHdr(), "Warning" );
	const char* selected = selxy_ ? "X/Y" : "Inl/Crl";
	const char* actual = isxy_ ? "X/Y" : "Inl/Crl";
	iopar.add( "You have selected positions in", selected );
	iopar.add( "But the positions in input file appear to be in", actual );
	BufferString msg = "OpendTect will use ";
	msg += actual; msg += " for final import";
        iopar.add( msg.buf(), "" );
    }

    iopar.add( IOPar::sKeyHdr(), "Geometry" );
    dtctor_.report( iopar );
    if ( isgeom_ && valranges_.size() > 0 )
	iopar.set( SI().zIsTime() ? "Time range (s)" : "Z Range",valranges_[0]);

    if ( valranges_.size() > firstattribidx )
    {
	iopar.add( IOPar::sKeySubHdr(), "Data values" );
	for ( int idx=firstattribidx; idx<valranges_.size(); idx++ )
	{
	    const char* attrnm = fd_.bodyinfos_[idx+1]->name().buf();
	    iopar.set( IOPar::compKey(attrnm,"Minimum value"),
		       valranges_[idx].start );
	    iopar.set( IOPar::compKey(attrnm,"Maximum value"),
		       valranges_[idx].stop );
	}
    }
    else
	iopar.add( IOPar::sKeySubHdr(), "No attribute data values" );

    if ( nrPositions() == 0 )
    {
	iopar.add( "No Valid positions found",
		   "Please re-examine input file and format definition" );
	return;
    }

    if ( !rejectedlines_.isEmpty() )
    {
	iopar.add( IOPar::sKeyHdr(), "Warning" );
	iopar.add( "These positions were rejected", "" );
	for ( int idx=0; idx<rejectedlines_.size(); idx++ )
	    iopar.add( toString(idx), rejectedlines_.get(idx).buf() );
    }
}



const char* HorizonScanner::defaultUserInfoFile()
{
    mDeclStaticString( ret );
    ret = GetProcFileName( "scan_horizon" );
    if ( GetSoftwareUser() )
	{ ret += "_"; ret += GetSoftwareUser(); }
    ret += ".txt";
    return ret.buf();
}


void HorizonScanner::launchBrowser( const char* fnm ) const
{
    if ( !fnm || !*fnm )
	fnm = defaultUserInfoFile();
    IOPar iopar; report( iopar );
    iopar.write( fnm, IOPar::sKeyDumpPretty() );

    File::launchViewer( fnm );
}


bool HorizonScanner::reInitAscIO( const char* fnm )
{
    delete ascio_; ascio_ = 0;
    delete strm_; strm_ = new od_istream( fnm );
    if ( !strm_->isOK() )
	return false;

    ascio_ = new EM::Horizon3DAscIO( fd_, *strm_ );
    return true;
}


#define mGetZFac SI().zIsTime() ? 0.001f : 1

bool HorizonScanner::analyseData()
{
    if ( !reInitAscIO( filenames_.get(0).buf() ) ) return false;

    const bool zistime = SI().zIsTime();
    const float fac = mGetZFac;
    Interval<float> validrg( SI().zRange() );
    const float zwidth = validrg.width();
    validrg.sort();
    validrg.start -= zwidth;
    validrg.stop += zwidth;

    int maxcount = 100;
    int count, nrxy, nrbid, nrscale, nrnoscale;
    count = nrxy = nrbid = nrscale = nrnoscale = 0;
    Coord crd;
    float val;
    TypeSet<float> data;
    while ( ascio_->getNextLine(crd,data) > 0 )
    {
	if ( data.isEmpty() ) break;

	if ( count > maxcount )
	{
	    if ( nrscale == nrnoscale ) maxcount *= 2;
	    else break;
	}

	BinID bid( mNINT32(crd.x_), mNINT32(crd.y_) );

	bool validplacement = false;
	if ( SI().isReasonable(crd) ) { nrxy++; validplacement=true; }
	if ( SI().isReasonable(bid) ) { nrbid++; validplacement=true; }

	if ( !isgeom_ )
	{
	    if ( validplacement ) count++;
	    continue;
	}

	val = data[0];
	bool validvert = false;
	if ( !mIsUdf(val) )
	{
	    if ( validrg.includes(val,false) ) { nrnoscale++; validvert=true; }
	    if ( zistime && validrg.includes(val*fac,false) )
	    { nrscale++; validvert=true; }
	}

	if ( validplacement && validvert )
	    count++;
    }

    isxy_ = nrxy > nrbid;
    selxy_ = ascio_->isXY();
    doscale_ = nrscale > nrnoscale;
    delete ascio_;
    ascio_ = 0;
    return true;
}


static bool isInsideSurvey( const BinID& bid, float zval )
{
    if ( !SI().isReasonable(bid) )
	return false;

    Interval<float> zrg( SI().zRange() );
    const float zwidth = zrg.width();
    zrg.sort();
    zrg.start -= zwidth;
    zrg.stop += zwidth;
    return zrg.includes( zval, false );
}


#define mErrRet(s) \
    { curmsg_ = s; dtctor_.finish(); return ErrorOccurred(); }

int HorizonScanner::nextStep()
{
    if ( fileidx_ >= filenames_.size() )
    {
	for ( int idx=0; idx<sections_.size(); idx++ )
	{
	    PosInfo::Detector* secdtctr = !idx ? &dtctor_
		: new PosInfo::Detector( PosInfo::Detector::Setup(false) );
	    const BinnedValueSet& bivs = *sections_[idx];
	    BinID bid;
	    BinnedValueSet::SPos pos;
	    while ( bivs.next(pos) )
	    {
		bid = bivs.getBinID( pos );
		secdtctr->add( SI().transform(bid), bid );
	    }

	    secdtctr->finish();

	    if ( idx )
	    {
		dtctor_.mergeResults( *secdtctr );
		delete secdtctr;
	    }
	}

	return Finished();
    }

    uiString fmtdefstr = tr("format definition");
    if ( !ascio_ && !reInitAscIO( filenames_.get(fileidx_).buf() ) )
	mErrRet(tr("Error during initialization")
		.appendPhrase(uiStrings::phrPlsCheckThe(fmtdefstr)))

    Coord crd;
    TypeSet<float> data;
    const int ret = ascio_->getNextLine( crd, data );
    if ( ret < 0 )
	mErrRet(tr("Error during data interpretation")
		.appendPhrase(uiStrings::phrPlsCheckThe(fmtdefstr)))
    if ( ret == 0 )
    {
	fileidx_++;
	delete ascio_;
	ascio_ = 0;
	sections_ += bvalset_;
	bvalset_ = 0;
	return MoreToDo();
    }

    if ( data.size() < 1 )
	mErrRet(tr("Not enough data read to analyze"))

    if ( !bvalset_ ) bvalset_ = new BinnedValueSet( data.size(), false );
    bvalset_->allowDuplicatePositions( true );

    float fac = 1;
    if ( doscale_ )
	fac = mGetZFac;

    BinID bid;
    if ( isxy_ )
	bid = SI().transform( crd );
    else
    {
	bid.inl() = mNINT32( crd.x_ );
	bid.crl() = mNINT32( crd.y_ );
    }

    if ( !SI().isReasonable(bid) )
	return MoreToDo();

    bool validpos = true;
    int validx = 0;
    while ( validx < data.size() )
    {
	if ( firsttime_ )
	    valranges_ += Interval<float>(mUdf(float),-mUdf(float));

	const float val = data[validx];
	if ( isgeom_ && validx==0 && !isInsideSurvey(bid,fac*val) )
	{
	    validpos = false;
	    break;
	}

	if ( !mIsUdf(val) )
	    valranges_[validx].include( fac*val, false );
	validx++;
    }

    if ( validpos && validx == 0 )
	validpos = false;

    if ( validpos )
	bvalset_->add( bid, data.arr() );

    else if ( rejectedlines_.size()<1024 )
    {
	BufferString rej( "", crd.x_, "\t" );
	rej += crd.y_;
	if ( isgeom_ )
	    { rej += "\t"; rej += data[0]; }
	rejectedlines_.add( rej.buf() );
    }

    firsttime_ = false;
    nrdone_++;
    return MoreToDo();
}


int HorizonScanner::nrPositions() const
{ return dtctor_.nrPositions(); }

StepInterval<int> HorizonScanner::inlRg() const
{ return dtctor_.getRange(true); }

StepInterval<int> HorizonScanner::crlRg() const
{ return dtctor_.getRange(false); }

bool HorizonScanner::gapsFound( bool inl ) const
{ return dtctor_.haveGaps(inl); }
