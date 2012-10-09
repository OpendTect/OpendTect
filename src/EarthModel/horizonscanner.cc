/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          Feb 2005
________________________________________________________________________

-*/
static const char* rcsID = "$Id$";

#include "horizonscanner.h"
#include "binidvalset.h"
#include "emhorizon3d.h"
#include "posinfodetector.h"
#include "iopar.h"
#include "strmprov.h"
#include "survinfo.h"
#include "oddirs.h"
#include "cubesampling.h"
#include "keystrs.h"
#include "tabledef.h"


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
    , curmsg_("Scanning")
    , nrdone_(0)
{
    filenames_ = fnms;
    init();
}


HorizonScanner::~HorizonScanner()
{
    delete &dtctor_;
    deepErase( sections_ );
}


void HorizonScanner::init()
{
    totalnr_ = -1;
    firsttime_ = true;
    valranges_.erase();
    dtctor_.reInit();
    analyzeData();
}


const char* HorizonScanner::message() const
{
    return curmsg_.buf();
}


const char* HorizonScanner::nrDoneText() const
{
    return "Positions handled";
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
	StreamProvider sp( filenames_.get(0).buf() );
	StreamData sd = sp.makeIStream();
	if ( !sd.usable() ) continue;

	char buf[80];
	while ( *sd.istrm )
	{
	    sd.istrm->getline( buf, 80 );
	    totalnr_++;
	}
	sd.close();
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
    static BufferString ret;
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

    ExecuteScriptCommand( "od_FileBrowser", fnm );
}


bool HorizonScanner::reInitAscIO( const char* fnm )
{
    StreamProvider sp( fnm );
    StreamData sd = sp.makeIStream();
    if ( !sd.usable() )
	return false;

    ascio_ = new EM::Horizon3DAscIO( fd_, *sd.istrm );
    if ( !ascio_ ) return false;

    return true;
}


#define mGetZFac SI().zIsTime() ? 0.001 : 1
	
bool HorizonScanner::analyzeData()
{
    if ( !reInitAscIO( filenames_.get(0).buf() ) ) return false;

    const bool zistime = SI().zIsTime();
    const float fac = mGetZFac;
    Interval<float> validrg( SI().zRange(false) );
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

	BinID bid( mNINT32(crd.x), mNINT32(crd.y) );

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
    
    Interval<float> zrg( SI().zRange(false) );
    const float zwidth = zrg.width();
    zrg.sort();
    zrg.start -= zwidth;
    zrg.stop += zwidth;
    return zrg.includes( zval, false );
}


#define mErrRet(s) \
    { \
	curmsg_ = s; dtctor_.finish(); \
	return Executor::ErrorOccurred(); \
    }

int HorizonScanner::nextStep()
{
    if ( fileidx_ >= filenames_.size() )
    {
	for ( int idx=0; idx<sections_.size(); idx++ )
	{
	    const BinIDValueSet& bivs = *sections_[idx];
	    BinID bid;
	    BinIDValueSet::Pos pos;
	    while ( bivs.next(pos) )
	    {
		bid = bivs.getBinID( pos );
		dtctor_.add( SI().transform(bid), bid );
	    }
	}

	dtctor_.finish();
	return Executor::Finished();
    }

    if ( !ascio_ && !reInitAscIO( filenames_.get(fileidx_).buf() ) )
	mErrRet("Error during initialization."
		"\nPlease check the format definition")

    Coord crd;
    TypeSet<float> data;
    const int ret = ascio_->getNextLine( crd, data );
    if ( ret < 0 )
	mErrRet("Error during data interpretation."
		"\nPlease check the format definition")
    if ( ret == 0 ) 
    {
	fileidx_++;
	delete ascio_;
	ascio_ = 0;
	sections_ += bvalset_;
	bvalset_ = 0;
	return Executor::MoreToDo();
    }

    if ( data.size() < 1 )
	mErrRet("Not enough data read to analyze")

    if ( !bvalset_ ) bvalset_ = new BinIDValueSet( data.size(), false );

    float fac = 1;
    if ( doscale_ )
	fac = mGetZFac;

    BinID bid;
    if ( isxy_ )
	bid = SI().transform( crd );
    else
    {
	bid.inl = mNINT32( crd.x );
	bid.crl = mNINT32( crd.y );
    }

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
	BufferString rej( "", crd.x, "\t" );
	rej += crd.y;
	if ( isgeom_ )
	    { rej += "\t"; rej += data[0]; }
	rejectedlines_.add( rej.buf() );
    }

    firsttime_ = false;
    nrdone_++;
    return Executor::MoreToDo();
}


int HorizonScanner::nrPositions() const
{ return dtctor_.nrPositions(); }

StepInterval<int> HorizonScanner::inlRg() const
{ return dtctor_.getRange(true); }

StepInterval<int> HorizonScanner::crlRg() const
{ return dtctor_.getRange(false); }

bool HorizonScanner::gapsFound( bool inl ) const
{ return dtctor_.haveGaps(inl); }
