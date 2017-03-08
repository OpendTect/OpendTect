/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Aug 2003
-*/


#include "welldisp.h"
#include "welldata.h"
#include "settings.h"
#include "keystrs.h"
#include "survinfo.h"


static const char* sSettingsKey = "welldisp";
static const char* sKeyTrackNmIsAbove = "Track Name Above";
static const char* sKeyTrackNmIsBelow = "Track Name Below";
static const char* sKeyTrackNmFont = "Track Font";
static const char* sKeyMarkerShape = "Marker Shape";
static const char* sKeyMarkerCylinderHeight = "Cylinder Height";
static const char* sKeyMarkerNmFont = "Marker Name Font";
static const char* sKeyMarkerNmColor = "Marker Name Color";
static const char* sKeyMarkerNmSameColor = "Marker Name Color Same as Marker";
static const char* sKeyMarkerSingleColor = "Single Marker Color";
static const char* sKeyMarkerSelected = "Selected Markers";
static const char* sKeyLogNmFont = "Log Font";
static const char* sKeySingleCol = "Single Fill Color";
static const char* sKeyDataRange = "Data Range Bool";
static const char* sKeyOverlap = "Log Overlap";
static const char* sKeyRepeatLog = "Log Number";
static const char* sKeySeisColor = "Log Seismic Style Color";
static const char* sKeyLogName = "Log Name";
static const char* sKeyFillName = "Filled Log name";
static const char* sKeyCliprate = "Cliprate";
static const char* sKeyFillRange = "Filled Log Range";
static const char* sKeyRange = "Log Range";
static const char* sKeyRevertRange = "Revert Range Bool";
static const char* sKeySeqname = "Sequence name";
static const char* sKeyColTabFlipped = "Log Color Table Flipped";
static const char* sKeyColTabCyclic = "Log Color Table Cyclic";
static const char* sKeyScale = "Log scale";
static const char* sKey2DDisplayStrat = "Display Stratigraphy";
static const char* sKeyLogStyle = "Log Style";
static const char* sKeyLogWidthXY = "Log Width XY";

#define mLog1(id) (*logs_[2*id])
#define mLog2(id) (*logs_[2*id+1])


mDefineInstanceCreatedNotifierAccess(Well::DisplayProperties);


Well::DisplayProperties::DisplayProperties( const char* nm )
    : NamedMonitorable(nm)
    , displaystrat_(false)
{
    doAddLogPair();
    usePar( Settings::fetch("welldisp") );
    isdefaults_ = true;
    init();
}


Well::DisplayProperties::DisplayProperties( const DisplayProperties& oth )
    : NamedMonitorable( oth )
{
    copyClassData( oth );
    init();
}


void Well::DisplayProperties::init()
{
    mAttachCB( track_.objectChanged(), DisplayProperties::subobjChgCB );
    mAttachCB( markers_.objectChanged(), DisplayProperties::subobjChgCB );
    mTriggerInstanceCreatedNotifier();
}


Well::DisplayProperties::~DisplayProperties()
{
    sendDelNotif();
}


mImplMonitorableAssignment( Well::DisplayProperties, NamedMonitorable )


void Well::DisplayProperties::copyClassData( const DisplayProperties& oth )
{
    track_ = oth.track_;
    markers_ = oth.markers_;
    displaystrat_ = oth.displaystrat_;
    isdefaults_ = oth.isdefaults_;
    copyLogPairsFrom( oth );
}


Monitorable::ChangeType Well::DisplayProperties::compareClassData(
					const DisplayProperties& oth ) const
{
    if ( logs_.size() != oth.logs_.size()
	|| track_ != oth.track_ || markers_ != oth.markers_ )
	return cEntireObjectChange();

    for ( int idx=0; idx<logs_.size(); idx++ )
	if ( *logs_[idx] != *oth.logs_[idx] )
	    return cEntireObjectChange();

    mDeliverSingCondMonitorableCompare( displaystrat_ == oth.displaystrat_,
					cDispStratChg() );
}


void Well::DisplayProperties::copyLogPairsFrom( const DisplayProperties& oth )
{
    logs_.setEmpty();
    for ( int idx=0; idx<oth.logs_.size(); idx++ )
    {
	logs_ += new LogDispProps( *oth.logs_[idx] );
	if ( idx%2 == 1 )
	    addCBsToLogPair( pairID4Idx(idx-1) );
    }
}


Well::DisplayProperties::LogPairID Well::DisplayProperties::doAddLogPair()
{
    LogDispProps* newlog1 = new LogDispProps;
    LogDispProps* newlog2 = new LogDispProps;

    if ( !logs_.isEmpty() ) // i.e. we are not in the constructor
    {
	const DisplayProperties& defs = defaults();
	if ( defs.nrLogPairs() > 0 )
	{
	    *newlog1 = defs.log( true );
	    *newlog2 = defs.log( false );
	}
    }
    newlog1->setFillLeft( true );
    newlog2->setFillRight( true );
    logs_ += newlog1; logs_ += newlog2;

    const LogPairID id = pairID4Idx( logs_.size() - 1 );
    addCBsToLogPair( id );
    return id;
}


void Well::DisplayProperties::subobjChgCB( CallBacker* )
{
    touch();
}


void Well::DisplayProperties::addCBsToLogPair( LogPairID id )
{
    mAttachCB( mLog1(id).objectChanged(), DisplayProperties::subobjChgCB );
    mAttachCB( mLog2(id).objectChanged(), DisplayProperties::subobjChgCB );
}


int Well::DisplayProperties::nrLogPairs() const
{
    mLock4Read();
    return nrPairs();
}


Well::LogDispProps& Well::DisplayProperties::log( bool fst, LogPairID id )
{
    mLock4Read();
    if ( id >= nrPairs() )
	{ pErrMsg("ID out of bounds"); id = 0; }
    return fst ? mLog1( id ) : mLog2( id );
}


const Well::LogDispProps& Well::DisplayProperties::log( bool fst,
							  LogPairID id ) const
{
    mLock4Read();
    if ( id >= nrPairs() )
	{ pErrMsg("ID out of bounds"); id = 0; }
    return fst ? mLog1( id ) : mLog2( id );
}


Well::DisplayProperties::LogPairID Well::DisplayProperties::addLogPair()
{
    mLock4Write();
    const LogPairID id = doAddLogPair();
    mSendChgNotif( cLogPairAdded(), id );
    return id;
}


void Well::DisplayProperties::setNrLogPairs( int nr )
{
    mLock4Read();
    int nrlps = nrPairs();
    if ( nr == nrlps )
	return;

    if ( !mLock2Write() )
    {
	nrlps = nrPairs();
	if ( nr == nrlps )
	    return;
    }

    while ( logs_.size() > 2*nr )
	logs_.removeSingle( logs_.size()-1 );
    nrlps = nrPairs();
    for ( LogPairID id=nrlps; id<nr; id++ )
	doAddLogPair();

    mSendEntireObjChgNotif();
}


bool Well::DisplayProperties::removeLogPair( LogPairID id )
{
    mLock4Read();
    if ( !isIDAvailable(id) )
	return false;

    if ( !mLock2Write() && !isIDAvailable(id) )
	return false;

    mSendChgNotif( cLogPairRemove(), id );
    logs_.removeSingle( id*2+1 );
    logs_.removeSingle( id*2 );
    return true;
}


void Well::MarkerDispProps::addSelMarkerName( const char* nm )
{
    mLock4Write();
    selmarkernms_.addIfNew( nm );
    mSendChgNotif( cMarkerNmsChg(), 0 );
}


void Well::MarkerDispProps::removeSelMarkerName( const char* nm )
{
    mLock4Write();
    const int idxof = selmarkernms_.indexOf( nm );
    if ( idxof >= 0 )
    {
	selmarkernms_.removeSingle( idxof );
	mSendChgNotif( cMarkerNmsChg(), 0 );
    }
}



Well::BasicDispProps::BasicDispProps( SizeType sz )
    : size_(sz)
    , color_(Color::White())
    , font_(cDefaultFontSize())
{
}


Well::BasicDispProps::BasicDispProps( const BasicDispProps& oth )
    : Monitorable(oth)
    , font_(*new FontData(cDefaultFontSize()))
{
    copyClassData( oth );
}


Well::BasicDispProps::~BasicDispProps()
{
    sendDelNotif();
}


mImplMonitorableAssignment( Well::BasicDispProps, Monitorable );


void Well::BasicDispProps::copyClassData( const BasicDispProps& oth )
{
    color_ = oth.color_;
    size_ = oth.size_;
    font_ = oth.font_;
}


Monitorable::ChangeType Well::BasicDispProps::compareClassData(
					const BasicDispProps& oth ) const
{
    mStartMonitorableCompare();
    mHandleMonitorableCompare( color_, cColorChg() );
    mHandleMonitorableCompare( size_, cSizeChg() );
    mHandleMonitorableCompare( font_, cFontChg() );
    mDeliverMonitorableCompare();
}


#define mGetIOPKey(ky) IOPar::compKey( subj, ky )


void Well::BasicDispProps::baseUsePar( const IOPar& iop,
			      const char* fontky, const char* szky )
{
    const char* subj = subjectName();
    iop.get( mGetIOPKey(sKey::Color()), color_ );
    iop.get( mGetIOPKey(sKey::Size()), size_ );
    const FixedString fontdata = iop.find( mGetIOPKey(fontky) );
    if ( fontdata )
	font_.getFrom( fontdata );
    else
    {
	int sz = 0;
	iop.get( mGetIOPKey(szky), sz );
	font_.setPointSize( sz );
    }
}


void Well::BasicDispProps::baseFillPar( IOPar& iop,
						const char* fontky ) const
{
    const char* subj = subjectName();
    iop.set( mGetIOPKey(sKey::Color()), color_ );
    iop.set( mGetIOPKey(sKey::Size()), size_ );
    BufferString fontdata; font_.putTo( fontdata );
    iop.set( mGetIOPKey(fontky), fontdata );
}


Well::TrackDispProps::TrackDispProps()
    : BasicDispProps(1)
    , dispabove_(true)
    , dispbelow_(true)
{
}


Well::TrackDispProps::TrackDispProps( const TrackDispProps& oth )
    : BasicDispProps(1)
{
    copyClassData( oth );
}


Well::TrackDispProps::~TrackDispProps()
{
    sendDelNotif();
}


mImplMonitorableAssignment( Well::TrackDispProps, Well::BasicDispProps );


void Well::TrackDispProps::copyClassData( const TrackDispProps& oth )
{
    dispabove_ = oth.dispabove_;
    dispbelow_ = oth.dispbelow_;
}


Monitorable::ChangeType Well::TrackDispProps::compareClassData(
					const TrackDispProps& oth ) const
{
    mDeliverSingCondMonitorableCompare(
	dispabove_ == oth.dispabove_ && dispbelow_ == oth.dispbelow_,
	cDispPosChg() );
}


void Well::TrackDispProps::usePar( const IOPar& iop )
{
    mLock4Write();
    baseUsePar( iop, sKeyTrackNmFont, "Track Name Size" );
    const char* subj = subjectName();

    iop.getYN( mGetIOPKey(sKeyTrackNmIsAbove), dispabove_ );
    iop.getYN( mGetIOPKey(sKeyTrackNmIsBelow), dispbelow_ );
}


void Well::TrackDispProps::fillPar( IOPar& iop ) const
{
    mLock4Read();
    baseFillPar( iop, sKeyTrackNmFont );
    const char* subj = subjectName();

    iop.setYN( mGetIOPKey(sKeyTrackNmIsAbove), dispabove_ );
    iop.setYN( mGetIOPKey(sKeyTrackNmIsBelow), dispbelow_ );
}


Well::MarkerDispProps::MarkerDispProps()
    : BasicDispProps(15)
    , shapetype_(0)
    , cylinderheight_(1)
    , issinglecol_(false)
    , samenmcol_(true)
{
}


Well::MarkerDispProps::MarkerDispProps( const MarkerDispProps& oth )
    : BasicDispProps(oth)
{
    copyClassData( oth );
}


Well::MarkerDispProps::~MarkerDispProps()
{
    sendDelNotif();
}


mImplMonitorableAssignment( Well::MarkerDispProps, Well::BasicDispProps );


void Well::MarkerDispProps::copyClassData( const MarkerDispProps& oth )
{
    shapetype_ = oth.shapetype_;
    cylinderheight_ = oth.cylinderheight_;
    issinglecol_ = oth.issinglecol_;
    nmcol_ = oth.nmcol_;
    samenmcol_ = oth.samenmcol_;
    selmarkernms_ = oth.selmarkernms_;
}


Monitorable::ChangeType Well::MarkerDispProps::compareClassData(
					const MarkerDispProps& oth ) const
{
    mStartMonitorableCompare();
    mHandleMonitorableCompare( shapetype_, cShapeChg() );
    mHandleMonitorableCompare( cylinderheight_, cShapeChg() );
    mHandleMonitorableCompare( issinglecol_, cColorChg() );
    mHandleMonitorableCompare( nmcol_, cColorChg() );
    mHandleMonitorableCompare( samenmcol_, cColorChg() );
    mHandleMonitorableCompare( selmarkernms_, cMarkerNmsChg() );
    mDeliverMonitorableCompare();
}


void Well::MarkerDispProps::usePar( const IOPar& iop )
{
    mLock4Write();
    baseUsePar( iop, sKeyMarkerNmFont, "Marker Name Size" );
    const char* subj = subjectName();

    iop.get( mGetIOPKey(sKeyMarkerShape), shapetype_ );
    iop.get( mGetIOPKey(sKeyMarkerCylinderHeight), cylinderheight_ );
    iop.getYN( mGetIOPKey(sKeyMarkerSingleColor),issinglecol_ );
    iop.get( mGetIOPKey(sKeyMarkerNmColor), nmcol_ );
    iop.getYN( mGetIOPKey(sKeyMarkerNmSameColor), samenmcol_ );
    iop.get( mGetIOPKey(sKeyMarkerSelected), selmarkernms_ );
}


void Well::MarkerDispProps::fillPar( IOPar& iop ) const
{
    mLock4Read();
    baseFillPar( iop, sKeyMarkerNmFont );
    const char* subj = subjectName();

    iop.set( mGetIOPKey(sKeyMarkerShape), shapetype_ );
    iop.set( mGetIOPKey(sKeyMarkerCylinderHeight), cylinderheight_ );
    iop.setYN( mGetIOPKey(sKeyMarkerSingleColor),issinglecol_ );
    iop.set( mGetIOPKey(sKeyMarkerNmColor), nmcol_ );
    iop.setYN( mGetIOPKey(sKeyMarkerNmSameColor), samenmcol_);
    iop.set( mGetIOPKey(sKeyMarkerSelected), selmarkernms_ );
}


Well::LogDispProps::LogDispProps()
    : BasicDispProps(1)
    , cliprate_(0)
    , fillname_("none")
    , fillrange_(mUdf(float),mUdf(float))
    , isleftfill_(false)
    , isrightfill_(false)
    , isdatarange_(true)
    , islogarithmic_(false)
    , islogreverted_(false)
    , issinglecol_(false)
    , logname_("none")
    , logwidth_(250 * ((WidthType)(SI().xyInFeet() ? mToFeetFactorF:1)))
    , range_(mUdf(float),mUdf(float))
    , repeat_(5)
    , repeatovlap_(50)
    , seiscolor_(Color::White())
    , seqname_("Rainbow")
    , sequsemode_(ColTab::UnflippedSingle)
    , style_( 0 )
{
}


Well::LogDispProps::LogDispProps( const LogDispProps& oth )
    : BasicDispProps(oth)
{
    copyClassData( oth );
}


Well::LogDispProps::~LogDispProps()
{
    sendDelNotif();
}


mImplMonitorableAssignment( Well::LogDispProps, Well::BasicDispProps );


void Well::LogDispProps::copyClassData( const LogDispProps& oth )
{
    cliprate_ = oth.cliprate_;
    fillname_ = oth.fillname_;
    fillrange_ = oth.fillrange_;
    isleftfill_ = oth.isleftfill_;
    isrightfill_ = oth.isrightfill_;
    isdatarange_ = oth.isdatarange_;
    islogarithmic_ = oth.islogarithmic_;
    islogreverted_ = oth.islogreverted_;
    issinglecol_ = oth.issinglecol_;
    logname_ = oth.logname_;
    logwidth_ = oth.logwidth_;
    range_ = oth.range_;
    repeat_ = oth.repeat_;
    repeatovlap_ = oth.repeatovlap_;
    seiscolor_ = oth.seiscolor_;
    seqname_ = oth.seqname_;
    sequsemode_ = oth.sequsemode_;
    style_ = oth.style_;
}


Monitorable::ChangeType Well::LogDispProps::compareClassData(
					const LogDispProps& oth ) const
{
    mStartMonitorableCompare();
    mHandleMonitorableCompare( cliprate_, cScaleChg() );
    mHandleMonitorableCompare( fillname_, cNameChg() );
    mHandleMonitorableCompare( fillrange_, cScaleChg() );
    mHandleMonitorableCompare( isleftfill_, cShapeChg() );
    mHandleMonitorableCompare( isrightfill_, cShapeChg() );
    mHandleMonitorableCompare( isdatarange_, cScaleChg() );
    mHandleMonitorableCompare( islogarithmic_, cScaleChg() );
    mHandleMonitorableCompare( islogreverted_, cScaleChg() );
    mHandleMonitorableCompare( issinglecol_, cColorChg() );
    mHandleMonitorableCompare( logname_, cNameChg() );
    mHandleMonitorableCompare( logwidth_, cShapeChg() );
    mHandleMonitorableCompare( range_, cScaleChg() );
    mHandleMonitorableCompare( repeat_, cShapeChg() );
    mHandleMonitorableCompare( repeatovlap_, cShapeChg() );
    mHandleMonitorableCompare( seiscolor_, cColorChg() );
    mHandleMonitorableCompare( seqname_, cNameChg() );
    mHandleMonitorableCompare( sequsemode_, cColorChg() );
    mHandleMonitorableCompare( style_, cShapeChg() );
    mDeliverMonitorableCompare();
}


static BufferString gtLRKy( bool left, const char* ky )
{
    return BufferString( left ? "Left " : "Right ", ky );
}


static const char* gtFillStr( bool isleft, bool isleftfill )
{
    const char* str;
    if ( isleft )
	str = isleftfill ? "Left Fill Left Log" : "Right Fill Left Log";
    else
	str = isleftfill ? "Right Fill Left Log" : "Left Fill Left Log";
    return IOPar::compKey( "Log", str );
}


#define mGetLRIOpKey(ky) mGetIOPKey( gtLRKy(isleft,ky) )


void Well::LogDispProps::usePar( const IOPar& iop, bool isleft )
{
    mLock4Write();
    baseUsePar( iop, sKeyLogNmFont, "Log Name Size" );
    const char* subj = subjectName();

    iop.get( mGetLRIOpKey(sKeyLogName), logname_ );
    iop.get( mGetLRIOpKey(sKeyRange), range_ );
    iop.get( mGetLRIOpKey(sKeyFillName), fillname_ );
    iop.get( mGetLRIOpKey(sKeyFillRange), fillrange_ );
    iop.getYN( gtFillStr(isleft,true), isleftfill_ );
    iop.getYN( gtFillStr(isleft,false), isrightfill_ );
    iop.getYN( mGetLRIOpKey(sKeyRevertRange),islogreverted_);
    iop.get( mGetLRIOpKey(sKeyCliprate), cliprate_ );
    iop.getYN( mGetLRIOpKey(sKeySingleCol), issinglecol_ );
    iop.getYN( mGetLRIOpKey(sKeyDataRange), isdatarange_ );
    iop.get( mGetLRIOpKey(sKeyRepeatLog), repeat_ );
    iop.get( mGetLRIOpKey(sKeyOverlap), repeatovlap_ );
    iop.get( mGetLRIOpKey(sKeySeisColor), seiscolor_ );
    iop.get( mGetLRIOpKey(sKeySeqname), seqname_ );
    iop.getYN( mGetLRIOpKey(sKeyScale), islogarithmic_ );
    iop.get( mGetLRIOpKey(sKeyLogStyle),style_);
    iop.get( mGetLRIOpKey(sKeyLogWidthXY),logwidth_ );
    bool isflipped = ColTab::isFlipped( sequsemode_ );
    bool iscyclic = ColTab::isCyclic( sequsemode_ );
    iop.getYN( mGetLRIOpKey(sKeyColTabFlipped), isflipped );
    iop.getYN( mGetLRIOpKey(sKeyColTabCyclic), iscyclic );
    sequsemode_ = ColTab::getSeqUseMode( isflipped, iscyclic );

    if ( SI().xyInFeet() )
	logwidth_ = (WidthType)( logwidth_*mToFeetFactorF );
}


void Well::LogDispProps::fillPar( IOPar& iop, bool isleft ) const
{
    mLock4Read();
    baseFillPar( iop, sKeyLogNmFont );
    const WidthType logwidth = logwidth_*
	(int)( SI().xyInFeet() ? mFromFeetFactorF : 1.0f );
    const char* subj = subjectName();

    iop.set( mGetLRIOpKey(sKeyLogName), logname_ );
    iop.set( mGetLRIOpKey(sKeyRange), range_ );
    iop.set( mGetLRIOpKey(sKeyFillName), fillname_ );
    iop.set( mGetLRIOpKey(sKeyFillRange), fillrange_ );
    iop.setYN( gtFillStr(isleft,true), isleftfill_ );
    iop.setYN( gtFillStr(isleft,false), isrightfill_ );
    iop.setYN( mGetLRIOpKey(sKeyRevertRange),islogreverted_);
    iop.set( mGetLRIOpKey(sKeyCliprate), cliprate_ );
    iop.setYN( mGetLRIOpKey(sKeySingleCol), issinglecol_ );
    iop.setYN( mGetLRIOpKey(sKeyDataRange), isdatarange_ );
    iop.set( mGetLRIOpKey(sKeyRepeatLog), repeat_ );
    iop.set( mGetLRIOpKey(sKeyOverlap), repeatovlap_ );
    iop.set( mGetLRIOpKey(sKeySeisColor), seiscolor_ );
    iop.set( mGetLRIOpKey(sKeySeqname), seqname_ );
    iop.setYN( mGetLRIOpKey(sKeyScale), islogarithmic_ );
    iop.setYN( mGetLRIOpKey(sKeyColTabFlipped), ColTab::isFlipped(sequsemode_));
    iop.setYN( mGetLRIOpKey(sKeyColTabCyclic), ColTab::isCyclic(sequsemode_) );
    iop.set( mGetLRIOpKey(sKeyLogStyle),style_);
    iop.set( mGetLRIOpKey(sKeyLogWidthXY),logwidth );
}


void Well::DisplayProperties::usePar( const IOPar& inpiop )
{
    IOPar* iop = inpiop.subselect( subjectName() );
    if ( !iop )
	iop = new IOPar( inpiop );

    track_.usePar( *iop );
    markers_.usePar( *iop );

    mLog1(0).usePar( *iop, true );
    mLog2(0).usePar( *iop, false );

    int lpidx=1;
    IOPar* welliop = iop->subselect( toString(lpidx) );
    if ( welliop || logs_.size() > 2 )
    {
	mLock4Write();
	logs_.setEmpty();
	for ( int idx=logs_.size()-1; idx>1; idx-- )
	    logs_.removeSingle( idx );
	while ( welliop )
	{
	    LogPairID lpid = doAddLogPair();
	    mLog1(lpid).usePar( *welliop, true );
	    mLog2(lpid).usePar( *welliop, false );
	    lpid++;
	    delete welliop;
	    welliop = iop->subselect( toString(lpid) );
	}
	mSendEntireObjChgNotif();
    }

    bool dispstrat = displaystrat_;
    iop->getYN( sKey2DDisplayStrat, dispstrat );
    delete iop;
    setDisplayStrat( dispstrat );
    isdefaults_ = false;
}


void Well::DisplayProperties::fillPar( IOPar& iop ) const
{
    mLock4Read();

    IOPar subpar;
    track_.fillPar( subpar );
    markers_.fillPar( subpar );
    for ( LogPairID id=0; id<nrPairs(); id++ )
    {
	IOPar logpairiop;
	mLog1(id).fillPar( logpairiop, true );
	mLog2(id).fillPar( logpairiop, false );
	subpar.mergeComp( logpairiop, id > 0 ? toString( id ) : "" );
    }
    const char* subj = subjectName();
    subpar.setYN( mGetIOPKey(sKey2DDisplayStrat), displaystrat_ );

    iop.mergeComp( subpar, subj );
}


Well::DisplayProperties& Well::DisplayProperties::defaults()
{
    mDefineStaticLocalObject( PtrMan<Well::DisplayProperties>, ret, = 0 );

    if ( !ret )
    {
	Settings& setts = Settings::fetch( sSettingsKey );
	Well::DisplayProperties* newret = new DisplayProperties;
	newret->usePar( setts );
	newret->isdefaults_ = true;

	ret.setIfNull( newret, true );
    }

    return *ret;
}


void Well::DisplayProperties::commitDefaults()
{
    Settings& setts = Settings::fetch( sSettingsKey );
    defaults().fillPar( setts );
    setts.write();
}
