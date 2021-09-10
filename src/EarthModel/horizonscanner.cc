/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          Feb 2005
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
HiddenParam<HorizonScanner,ZAxisTransform*> transform_( nullptr );

HorizonScanner::HorizonScanner( const BufferStringSet& fnms,
				Table::FormatDesc& fd, bool isgeom )
    : Executor("Scan horizon file(s)")
    , dtctor_(*new PosInfo::Detector(PosInfo::Detector::Setup(false)))
    , fd_(fd)
    , isgeom_(isgeom)
    , ascio_(0)
    , isxy_(false)
    , selxy_(false)
    , doscale_(false)
    , bvalset_(0)
    , fileidx_(0)
    , curmsg_(tr("Scanning"))
    , nrdone_(0)
{
    filenames_ = fnms;
    transform_.setParam( this, nullptr );
    init();
}


HorizonScanner::HorizonScanner( const BufferStringSet& fnms,
				Table::FormatDesc& fd, bool isgeom,
				ZAxisTransform* trans )
    : Executor("Scan horizon file(s)")
    , dtctor_(*new PosInfo::Detector(PosInfo::Detector::Setup(false)))
    , fd_(fd)
    , isgeom_(isgeom)
    , ascio_(0)
    , isxy_(false)
    , selxy_(false)
    , bvalset_(0)
    , fileidx_(0)
    , nrdone_(0)
{
    filenames_ = fnms;
    transform_.setParam( this, nullptr );
    setZAxisTransform( trans );
    init();
}



HorizonScanner::~HorizonScanner()
{
    delete &dtctor_;
    deepErase( sections_ );

    ZAxisTransform* transform = transform_.getParam( this );
    if ( transform )
	transform->unRef();
    transform_.removeParam( this );
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
    ZAxisTransform* oldtransform = transform_.getParam( this );
    if ( oldtransform )
	oldtransform->unRef();

    transform_.setParam( this, transform );
    if ( transform )
	transform->ref();
}


const ZAxisTransform* HorizonScanner::getZAxisTransform() const
{ return transform_.getParam( this ); }

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
	BufferString zrgkey =
		SI().zIsTime() ? sKey::TimeRange() : sKey::ZRange();
	zrgkey.add( SI().getZUnitString() );
	Interval<float> zrg = valranges_[0];
	if ( SI().zIsTime() )
	    zrg.scale( SI().showZ2UserFactor() );

	iopar.set( zrgkey, zrg );
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
    ascio_ = new EM::Horizon3DAscIO( fd_, fnm );
    if ( !ascio_ || !ascio_->isOK() )
    {
	deleteAndZeroPtr( ascio_ );
	return false;
    }
    return true;
}


void HorizonScanner::transformZIfNeeded( const BinID& bid, float& zval ) const
{
    ZAxisTransform* transform = transform_.getParam( this );
    if ( transform )
    {
	if ( SI().zIsTime() && SI().depthsInFeet() )
	    zval *= mToFeetFactorF;

	zval = transform->transformTrc( bid, zval );
    }
}


static const UnitOfMeasure* getSuggestedZUnit( const UnitOfMeasure* zunit,
						float scalefac )
{
    if ( !zunit )
	return nullptr;

    ObjectSet<const UnitOfMeasure> zunits;
    UoMR().getRelevant( PropertyRef::Time, zunits );
    const double fac = zunit->scaler().factor * scalefac;
    for ( int idx=0; idx<zunits.size(); idx++ )
    {
	if ( mIsEqual(zunits[idx]->scaler().factor,fac,fac/100) )
	    return zunits[idx];
    }

    return nullptr;
}


bool HorizonScanner::analyzeData()
{
    if ( !reInitAscIO( filenames_.get(0).buf() ) ) return false;

    const bool hastransform = transform_.getParam( this ) != nullptr;
    const bool zistime = SI().zIsTime();
    const bool tryscale = isgeom_ && zistime && !hastransform;
    const float zscalefac = 0.001;
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
    selxy_ = ascio_->isXY();
    while ( ascio_->getNextLine(crd,data) > 0 )
    {
	if ( data.isEmpty() ) break;

	if ( count > maxcount )
	    break;

	BinID bid( mNINT32(crd.x), mNINT32(crd.y) );

	bool validplacement = false;
	if ( selxy_ )
	{
	    if ( SI().isReasonable(crd) ) { nrxy++; validplacement=true; }
	    else if ( SI().isReasonable(bid) ) { nrbid++; validplacement=true; }
	}
	else
	{
	    if ( SI().isReasonable(bid) ) { nrbid++; validplacement=true; }
	    else if ( SI().isReasonable(crd) ) { nrxy++; validplacement=true; }
	}

	if ( !tryscale )
	{
	    if ( validplacement ) count++;
	    continue;
	}

	const BinID selbid = selxy_ ? SI().transform( crd ) : bid;
	val = data[0];
	transformZIfNeeded( selbid, val );
	bool validvert = false;
	if ( !mIsUdf(val) )
	{
	    if ( validrg.includes(val,false) )
	    {
		nrnoscale++;
		validvert = true;
	    }
	    else if ( validrg.includes(val*zscalefac,false) )
	    {
		nrscale++;
		validvert = true;
	    }
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

    const UnitOfMeasure* selzunit = ascio_->getSelZUnit();
    const UnitOfMeasure* suggestedzunit = nullptr;
    if ( nrscale > nrnoscale )
	suggestedzunit = getSuggestedZUnit( selzunit, zscalefac );

    if ( suggestedzunit )
    {
	uiString zmsg = tr("You have selected Z in %1, "
			   "but the Z values in file appear to be in %2." )
			   .arg( selzunit->name() )
			   .arg( suggestedzunit->name() );
	if ( curmsg_.isEmpty() )
	    curmsg_ = zmsg;
	else
	    curmsg_.appendPhrase( zmsg );
    }

    isxy_ = selxy_;
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

    if ( !ascio_ && !reInitAscIO( filenames_.get(fileidx_).buf() ) )
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

    if ( !bvalset_ ) bvalset_ = new BinIDValueSet( data.size(), false );
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

    transformZIfNeeded( bid, data[0] );
    bool validpos = true;
    int validx = 0;
    while ( validx < data.size() )
    {
	if ( firsttime_ )
	    valranges_ += Interval<float>(mUdf(float),-mUdf(float));

	float& val = data[validx];
	if ( isgeom_ && validx==0 )
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
