/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "horizonscanner.h"
#include "emhorizonascio.h"
#include "binidvalset.h"
#include "emhorizon3d.h"
#include "posinfodetector.h"
#include "ptrman.h"
#include "iopar.h"
#include "od_istream.h"
#include "survinfo.h"
#include "oddirs.h"
#include "trckeyzsampling.h"
#include "keystrs.h"
#include "tabledef.h"
#include "unitofmeasure.h"
#include "zaxistransform.h"
#include "file.h"

#include "hiddenparam.h"

static HiddenParam<HorizonScanner,const ZDomain::Info*> emobjecthpmgr_(nullptr);


HorizonScanner::HorizonScanner(const BufferStringSet& fnms,
    Table::FormatDesc& fd, bool isgeom, const ZDomain::Info& zinfo )
    : Executor("Scan horizon file(s)")
    , nrdone_(0)
    , dtctor_(*new PosInfo::Detector(PosInfo::Detector::Setup(false)))
    , ascio_(nullptr)
    , fileidx_(0)
    , isgeom_(isgeom)
    , isxy_(false)
    , selxy_(false)
    , doscale_(false)
    , fd_(fd)
    , bvalset_(nullptr)
{
    emobjecthpmgr_.setParam( this, new ZDomain::Info(zinfo) );
    zdomain_ = &zinfo.def_;
    filenames_ = fnms;
    init();
}


HorizonScanner::HorizonScanner( const BufferStringSet& fnms,
				Table::FormatDesc& fd, bool isgeom )
    : Executor("Scan horizon file(s)")
    , nrdone_(0)
    , dtctor_(*new PosInfo::Detector(PosInfo::Detector::Setup(false)))
    , ascio_(nullptr)
    , fileidx_(0)
    , isgeom_(isgeom)
    , isxy_(false)
    , selxy_(false)
    , doscale_(false)
    , fd_(fd)
    , bvalset_(nullptr)
    , zdomain_(&ZDomain::SI())
    , curmsg_(tr("Scanning"))
{
    emobjecthpmgr_.setParam( this, new ZDomain::Info(SI().zDomainInfo()) );
    filenames_ = fnms;
    init();
}


HorizonScanner::HorizonScanner( const BufferStringSet& fnms,
				Table::FormatDesc& fd, bool isgeom,
				ZAxisTransform* trans, bool iszdepth )
    : Executor("Scan horizon file(s)")
    , nrdone_(0)
    , dtctor_(*new PosInfo::Detector(PosInfo::Detector::Setup(false)))
    , ascio_(nullptr)
    , fileidx_(0)
    , isgeom_(isgeom)
    , isxy_(false)
    , selxy_(false)
    , fd_(fd)
    , bvalset_(nullptr)
    , zdomain_(nullptr)
{
    filenames_ = fnms;
    const ZDomain::Info& zinf = SI().zIsTime() ? ZDomain::TWT() :
	iszdepth && SI().zInFeet() ? ZDomain::DepthFeet() :
	ZDomain::DepthMeter();
    emobjecthpmgr_.setParam( this, new ZDomain::Info(zinf) );
    zdomain_ = &zinf.def_;
    init();
}


HorizonScanner::~HorizonScanner()
{
    delete &dtctor_;
    deepErase( sections_ );
    if ( transform_ )
	transform_->unRef();

    emobjecthpmgr_.removeAndDeleteParam( this );
}


void HorizonScanner::init()
{
    totalnr_ = -1;
    firsttime_ = true;
    valranges_.erase();
    dtctor_.reInit();
    analyzeData();
}


uiString HorizonScanner::uiMessage() const
{
    return curmsg_;
}


uiString HorizonScanner::uiNrDoneText() const
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


void HorizonScanner::setZAxisTransform( ZAxisTransform* transform )
{
    if ( transform_ )
	transform_->unRef();

    transform_ = transform;
    if ( transform_ )
	transform_->ref();
}


const ZAxisTransform* HorizonScanner::getZAxisTransform() const
{
    return transform_;
}


mStartAllowDeprecatedSection

bool HorizonScanner::isZInDepth() const
{
    return zdomain_->isDepth();
}


void HorizonScanner::setZInDepth()
{
    if ( isZInDepth() )
	return;

    zdomain_ = &ZDomain::Depth();
}


void HorizonScanner::setZInTime()
{
    if ( !isZInDepth() )
	return;

    zdomain_ = &ZDomain::Time();
}

mStopAllowDeprecatedSection


void HorizonScanner::report( StringPairSet& report ) const
{
    report.setEmpty();

    const int firstattribidx = isgeom_ ? 1 : 0;
    BufferString str = "Report for horizon file(s):\n";
    for ( int idx=0; idx<filenames_.size(); idx++ )
    {
	if ( idx > 0 )
	    str += "\n";

	str += filenames_.get(idx).buf();
    }

    report.setName( str.buf() );

    report.add( StringPairSet::sKeyH1(), "Geometry" );
    dtctor_.report( report );
    if ( isgeom_ && valranges_.size() > 0 )
    {
	const ZDomain::Info& zinfo = *emobjecthpmgr_.getParam( this );
	BufferString zrgkey( zinfo.userName().getFullString() );
	zrgkey.addSpace().add( zinfo.unitStr_(true) );
	Interval<float> zrg = valranges_[0];
	zrg.scale( zinfo.userFactor() );
	BufferString zrgstr;
	zrgstr.add( zrg.start, SI().nrZDecimals() ).add( " - " )
	      .add( zrg.stop, SI().nrZDecimals() );
	report.add( zrgkey, zrgstr );
    }

    if ( valranges_.size() > firstattribidx )
    {
	report.add( StringPairSet::sKeyH2(), "Data values" );
	for ( int idx=firstattribidx; idx<valranges_.size(); idx++ )
	{
	    const char* attrnm = fd_.bodyinfos_[idx+1]->name().buf();
	    report.add( IOPar::compKey(attrnm,"Minimum value"),
		       valranges_[idx].start );
	    report.add( IOPar::compKey(attrnm,"Maximum value"),
		       valranges_[idx].stop );
	}
    }
    else
	report.add( StringPairSet::sKeyH2(), "No attribute data values" );

    if ( nrPositions() == 0 )
    {
	report.add( "No Valid positions found",
		   "Please re-examine input file and format definition" );
	return;
    }

    if ( !rejectedlines_.isEmpty() )
    {
	report.add( StringPairSet::sKeyH1(), "Warning" );
	report.add( "These positions were rejected", "" );
	for ( int idx=0; idx<rejectedlines_.size(); idx++ )
	    report.add( toString(idx), rejectedlines_.get(idx).buf() );
    }
}


mStartAllowDeprecatedSection

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

    iopar.add( IOPar::sKeyHdr(), "Geometry" );
    dtctor_.report( iopar );
    if ( isgeom_ && valranges_.size() > 0 )
    {
	const ZDomain::Info& zinfo = *emobjecthpmgr_.getParam( this );
	BufferString zrgkey( zinfo.userName().getFullString() );
	zrgkey.add( zinfo.unitStr_() );
	Interval<float> zrg = valranges_[0];
	BufferString zrgstr;
	zrgstr.add( zrg.start, SI().nrZDecimals() ).add( " - " )
	      .add( zrg.stop, SI().nrZDecimals() );
	iopar.set( zrgkey, zrgstr );
    }

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

mStopAllowDeprecatedSection


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

    StringPairSet rep;
    report( rep );
    rep.write( fnm );
    File::launchViewer( fnm );
}


bool HorizonScanner::reInitAscIO( const char* fnm )
{
    ascio_ = new EM::Horizon3DAscIO( fd_, fnm );
    if ( !ascio_ || !ascio_->isOK() )
    {
	deleteAndNullPtr( ascio_ );
	return false;
    }

    return true;
}


void HorizonScanner::transformZIfNeeded( const BinID& bid, float& zval ) const
{
    if ( transform_ )
    {
	if ( SI().zIsTime() && SI().depthsInFeet() )
	    zval *= mToFeetFactorF;

	zval = transform_->transformTrc( TrcKey(bid), zval );
    }
}


const Interval<float> HorizonScanner::getReasonableZRange() const
{
    const ZDomain::Info& zinfo = *emobjecthpmgr_.getParam( this );
    return zinfo.getReasonableZRange();
}


void HorizonScanner::getConvValue( float& zval )
{
    if ( !ascio_ && !reInitAscIO(filenames_.get(0).buf()) )
	return;

    const ZDomain::Info& zinfo = *emobjecthpmgr_.getParam( this );
    const UnitOfMeasure* fromuom = zinfo.isDepth() ?
	Table::AscIO::getDepthUnit() : Table::AscIO::getTimeUnit();
    convValue( zval, fromuom, UnitOfMeasure::zUnit(zinfo) );
}


bool HorizonScanner::analyzeData()
{
    if ( !reInitAscIO( filenames_.get(0).buf() ) )
	return false;

    const Interval<float> validrg( getReasonableZRange() );

    int maxcount = 100;
    int count, nrxy, nrbid, nrvalid, nrnotvalid;
    count = nrxy = nrbid = nrvalid = nrnotvalid = 0;
    Coord crd;
    float val;
    TypeSet<float> data;
    selxy_ = ascio_->isXY();
    while ( ascio_->getNextLine(crd,data) > 0 )
    {
	if ( data.isEmpty() )
	    break;

	if ( count > maxcount )
	    break;

	BinID bid( mNINT32(crd.x), mNINT32(crd.y) );

	bool validplacement = false;
	if ( selxy_ )
	{
	    if ( SI().isReasonable(crd) )
	    {
		nrxy++;
		validplacement=true;
	    }
	    else if ( SI().isReasonable(bid) )
	    {
		nrbid++;
		validplacement=true;
	    }
	}
	else
	{
	    if ( SI().isReasonable(bid) )
	    {
		nrbid++;
		validplacement=true;
	    }
	    else if ( SI().isReasonable(crd) )
	    {
		nrxy++;
		validplacement=true;
	    }
	}

	val = data[0];
	bool validvert = false;
	if ( !mIsUdf(val) )
	{
	    getConvValue( val );
	    validvert = true;
	    if ( validrg.includes(val,false) )
		nrvalid++;
	    else
		nrnotvalid++;
	}

	if ( validplacement && validvert )
	    count++;
    }

    const bool apparentisxy = nrxy > nrbid;
    if ( apparentisxy != selxy_ )
    {
	curmsg_ = tr("You have selected positions in %1, "
		     "but the positions in file appear to be in %2.")
		     .arg( selxy_ ? "X/Y" : "Inl/Crl" )
		     .arg( apparentisxy ? "X/Y" : "Inl/Crl" );
    }

    if ( nrnotvalid > nrvalid )
    {
	const UnitOfMeasure* selzunit = ascio_->getSelZUnit();
	uiString zmsg = tr("You have selected Z in %1. "
			   "In this unit many Z values\n"
			   "appear to be outside the survey range.")
			   .arg( selzunit->name() );
	if ( curmsg_.isEmpty() )
	    curmsg_ = zmsg;
	else
	    curmsg_.appendPhrase( zmsg );
    }

    isxy_ = selxy_;
    deleteAndNullPtr( ascio_ );
    return true;
}


bool HorizonScanner::isInsideSurvey( const BinID& bid, float zval ) const
{
    if ( !SI().isReasonable(bid) )
	return false;

    const ZGate zrg = getReasonableZRange();
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
	    PosInfo::Detector* secdtctr = !idx ? &dtctor_
		: new PosInfo::Detector( PosInfo::Detector::Setup(false) );
	    const BinIDValueSet& bivs = *sections_[idx];
	    BinID bid;
	    BinIDValueSet::SPos pos;
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

	return Executor::Finished();
    }

    if ( !ascio_ && !reInitAscIO(filenames_.get(fileidx_).buf()) )
	mErrRet(tr("Error during initialization."
		"\nPlease check the format definition"))

    Coord crd;
    TypeSet<float> data;
    const int ret = ascio_->getNextLine( crd, data );
    if ( ret < 0 )
	mErrRet(tr("Error during data interpretation."
		"\nPlease check the format definition"))
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
	mErrRet(tr("Not enough data read to analyze"))

    if ( !bvalset_ )
	bvalset_ = new BinIDValueSet( data.size(), false );

    bvalset_->allowDuplicateBinIDs(true);
    BinID bid;
    if ( isxy_ )
	bid = SI().transform( crd );
    else
    {
	bid.inl() = mNINT32( crd.x );
	bid.crl() = mNINT32( crd.y );
    }

    if ( !SI().isReasonable(bid) )
	return Executor::MoreToDo();

    getConvValue( data[0] );

    bool validpos = true;
    int validx = 0;
    while ( validx < data.size() )
    {
	if ( firsttime_ )
	    valranges_ += Interval<float>(mUdf(float),-mUdf(float));

	const float val = data[validx];
	if ( isgeom_ && validx == 0)
	{
	    if ( !isInsideSurvey(bid,val) )
	    {
		validpos = false;
		break;
	    }
	}

	if ( !mIsUdf(val) )
	    valranges_[validx].include( val, false );

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
