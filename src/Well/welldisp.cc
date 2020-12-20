/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
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

static const char* sKeyNrPanels = "Nr Panels";
static const char* sKeyNrLogs = "Nr Logs";
static const char* sKeyFillLeftYN = "Fill Left";
static const char* sKeyFillRightYN = "Fill right";


mDefineInstanceCreatedNotifierAccess(Well::DisplayProperties);


Well::DisplayProperties::DisplayProperties( const char* nm )
    : NamedMonitorable(nm)
    , displaystrat_(false)
{
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
}


Monitorable::ChangeType Well::DisplayProperties::compareClassData(
					const DisplayProperties& oth ) const
{
    if ( track_ != oth.track_ || markers_ != oth.markers_ )
	return cEntireObjectChange();

    mDeliverSingCondMonitorableCompare( displaystrat_ == oth.displaystrat_,
					cDispStratChg() );
}


void Well::DisplayProperties::subobjChgCB( CallBacker* )
{
    touch();
}


void Well::DisplayProperties::usePar( const IOPar& inpiop )
{
    PtrMan<IOPar> subobjpar = inpiop.subselect( subjectName() );
    const IOPar& ioptouse = subobjpar ? *subobjpar : inpiop;

    track_.usePar( ioptouse );
    markers_.usePar( ioptouse );
    bool dispstrat = displaystrat_;
    ioptouse.getYN( sKey2DDisplayStrat, dispstrat );

    setDisplayStrat( dispstrat );
    isdefaults_ = false;
}


#define mGetIOPKey(ky) IOPar::compKey( subj, ky )

void Well::DisplayProperties::fillPar( IOPar& iop ) const
{
    mLock4Read();

    IOPar subpar;
    track_.fillPar( subpar );
    markers_.fillPar( subpar );
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

//DisplayProperties3D
Well::DisplayProperties3D::DisplayProperties3D()
    : DisplayProperties( sKey3DDispProp() )
{
    leftlog_ = new LogDispProps;
    rightlog_ = new LogDispProps;
    logtube_ = new LogDispProps;
}

Well::DisplayProperties3D::DisplayProperties3D( const DisplayProperties3D& oth )
    : DisplayProperties(oth)
{
    copyClassData( oth );
}


Well::DisplayProperties3D::~DisplayProperties3D()
{
    delete leftlog_;
    delete rightlog_;
    delete logtube_;
}


void Well::DisplayProperties3D::addLog( const DisplayProperties3D::Position pos)
{
    mLock4Write();
    switch( pos )
    {
	case Well::DisplayProperties3D::Left:
	    leftlog_ = new LogDispProps();
	    break;
	case Well::DisplayProperties3D::Right:
	    rightlog_ = new LogDispProps();
	    break;
	case Well::DisplayProperties3D::Tube:
	    logtube_ = new LogDispProps();
	    break;
    }

    mSendChgNotif( cLogAdd(), pos );
}


void Well::DisplayProperties3D::removeLog(
				const DisplayProperties3D::Position pos )
{
    mLock4Write();
    mSendChgNotif( cLogRemove(), 0 );
    switch( pos )
    {
	case Well::DisplayProperties3D::Left:
	    deleteAndZeroPtr( leftlog_ );
	    break;
	case Well::DisplayProperties3D::Right:
	    deleteAndZeroPtr( rightlog_ );
	    break;
	case Well::DisplayProperties3D::Tube:
	    deleteAndZeroPtr( logtube_ );
	    break;
    }
}


mImplMonitorableAssignment(Well::DisplayProperties3D, Well::DisplayProperties);

void Well::DisplayProperties3D::copyClassData(
					const Well::DisplayProperties3D& oth )
{
    delete leftlog_;
    leftlog_ = new LogDispProps( *oth.leftLog() );
    delete rightlog_;
    rightlog_ = new LogDispProps( *oth.rightLog() );
    delete logtube_;
    logtube_ = new LogDispProps( *oth.logTube() );
}


Monitorable::ChangeType Well::DisplayProperties3D::compareClassData(
				const Well::DisplayProperties3D& oth ) const
{
    if ( leftlog_ != oth.leftLog() )
	return cLogChange();
    else if ( rightlog_ != oth.rightLog() )
	return cLogChange();
    else if ( logtube_ != oth.logTube() )
	return cLogChange();
    else
	return cNoChange();
}


void Well::DisplayProperties3D::usePar( const IOPar& iop )
{
    DisplayProperties::usePar( iop );
    BufferString commonkey( IOPar::compKey(sKey3DDispProp(),sKey::Log()) );
    ConstPtrMan<IOPar> logpar = iop.subselect( commonkey );
    if ( !logpar )
	return;

//TODO Support old usepar
    ConstPtrMan<IOPar> lfetlogpar = logpar->subselect( "Left" );
    if ( lfetlogpar )
	leftlog_->usePar( *lfetlogpar );

    ConstPtrMan<IOPar> rightlogpar = logpar->subselect( "Right" );
    if ( rightlogpar )
	rightlog_->usePar( *rightlogpar );

    ConstPtrMan<IOPar> logtubepar = logpar->subselect( "Tube" );
    if ( logtubepar )
	logtube_->usePar( *logtubepar );
}


void Well::DisplayProperties3D::fillPar( IOPar& iop ) const
{
    DisplayProperties::fillPar( iop );

    BufferString commonkey = IOPar::compKey( sKey3DDispProp(),
						   sKey::Log() );
    if ( leftlog_ )
    {
	IOPar leftpar;
	leftlog_->fillPar( leftpar );
	BufferString leftlogkey( IOPar::compKey(commonkey, "Left") );
	iop.mergeComp( leftpar, leftlogkey );
    }

    if ( rightlog_ )
    {
	IOPar rightpar;
	rightlog_->fillPar( rightpar );
	BufferString rightlogkey( IOPar::compKey(commonkey, "Right") );
	iop.mergeComp( rightpar, rightlogkey );
    }

    if ( logtube_ )
    {
	IOPar tubepar;
	logtube_->fillPar( tubepar );
	BufferString tubelogkey( IOPar::compKey(commonkey, "Tube") );
	iop.mergeComp( tubepar, tubelogkey );
    }
}


//LogPanelProps
Well::DisplayProperties2D::LogPanelProps::LogPanelProps( const char* nm )
    : NamedMonitorable( nm )
{
    //TODO do we need mLock4Write();?
     addLog();
}


Well::DisplayProperties2D::LogPanelProps::LogPanelProps(
						    const LogPanelProps& oth )
{
    logs_.setEmpty();
    for ( int lidx=0; lidx<oth.logs_.size(); lidx++ )
    {
	logs_ += new LogDispProps( *oth.logs_[lidx] );
	mAttachCB( oth.logs_[lidx]->objectChanged(),
		   Well::DisplayProperties2D::LogPanelProps::logChangeCB );
    }
    //mSendChgNotif ?
}


void Well::DisplayProperties2D::LogPanelProps::logChangeCB( CallBacker* cb )
{
    mLock4Write();
    mGetMonitoredChgDataWithCaller(cb,chgdata,caller);
    mDynamicCastGet(LogDispProps*,lp,caller);
    if ( !lp )
	return;

    bool isnmchg = chgdata.includes( Well::LogDispProps::cNameChg() );
    int logid = logs_.indexOf( lp );
    if ( isnmchg )
	mSendChgNotif( Well::DisplayProperties2D::LogPanelProps::cLogNameChg(),
		       logid );

    touch();
}


bool Well::DisplayProperties2D::LogPanelProps::addLog()
{
    mLock4Write();
    bool logadded = logs_.size() <= maximumNrOfLogs();
    if ( logadded )
    {
	LogDispProps* log = new LogDispProps;
	log->setLogName( "Density" );
	logs_ += log;
	const int logid = logs_.indexOf( log );
	mSendChgNotif( cLogAddToPanel(), logid );
	mAttachCB( log->objectChanged(),
		   Well::DisplayProperties2D::LogPanelProps::logChangeCB );
    }

    return logadded;
}


void Well::DisplayProperties2D::LogPanelProps::removeLog( int idx )
{
    if ( logs_.isEmpty() )
	return;

    mLock4Write();
    delete logs_.removeSingle( idx );
    mSendChgNotif( cLogRemoveFromPanel(), idx );
}


Well::LogDispProps* Well::DisplayProperties2D::LogPanelProps::getLog( int id )
{
    if ( logs_.isEmpty() )
	return 0;

    return logs_.get( id );
}


const Well::LogDispProps* Well::DisplayProperties2D::
					LogPanelProps::getLog( int id ) const
{
    if ( logs_.isEmpty() )
	return 0;

    return logs_.get( id );
}


void Well::DisplayProperties2D::LogPanelProps::fillPar( IOPar& iop ) const
{
    const int nrlogs = logs_.size();
    for ( int lidx=0; lidx<nrlogs; lidx++ )
    {
	iop.set( sKeyNrLogs, nrlogs );

	const BufferString logkey = IOPar::compKey( sKey::Log(), lidx );
	IOPar logpar;
	logs_.get( lidx )->fillPar( logpar );
	iop.mergeComp( logpar, logkey );
    }
}


void Well::DisplayProperties2D::LogPanelProps::usePar( const IOPar& iop )
{
    int nrlogs = 0;
    iop.get( sKeyNrLogs, nrlogs );
    logs_.setEmpty();
    for ( int lidx=0; lidx<nrlogs; lidx++ )
    {
	const BufferString logkey = IOPar::compKey( sKey::Log(), lidx );
	PtrMan<IOPar> logpar = iop.subselect( logkey );
	if ( !logpar )
	    continue;

	addLog();
	logs_.get( lidx )->usePar( iop );
	//TODO need to call old usepar format
	//like logs_->get( lidx )->oldusePar();
    }
}


//DisplayProperties2D
Well::DisplayProperties2D::DisplayProperties2D()
    : DisplayProperties( sKey2DDispProp() )
{
    addLogPanel();
}


Well::DisplayProperties2D::DisplayProperties2D( const DisplayProperties2D& oth )
    : DisplayProperties(oth)
{
    copyClassData( oth );
}


void Well::DisplayProperties2D::addLogPanel()
{
    mLock4Write();
    const bool addpanel = logpanels_.size() <= maximumNrOfLogPanels();
    if ( addpanel )
    {
	LogPanelProps* panel = new LogPanelProps;
	logpanels_ += panel;
	mSendChgNotif( cPanelAdded(), logpanels_.size()-1 );
    }
}


void Well::DisplayProperties2D::removeLogPanel( int panelid )
{
    const bool isvalid = logpanels_.validIdx( panelid );
    if ( !isvalid )
	return;

    mLock4Write();
    mSendChgNotif( cPanelRemove(), panelid );
    logpanels_.removeSingle( panelid );
}


int Well::DisplayProperties2D::nrPanels() const
{
    return logpanels_.size();
}


const Well::DisplayProperties2D::LogPanelProps* Well::DisplayProperties2D::
						getLogPanel( int panelid ) const
{
    const bool isvalid = logpanels_.validIdx( panelid );
    if ( !isvalid )
	return 0;

    return logpanels_.get( panelid );
}


Well::DisplayProperties2D::LogPanelProps* Well::DisplayProperties2D::
						getLogPanel( int panelid )
{
    const bool isvalid = logpanels_.validIdx( panelid );
    if ( !isvalid )
	return 0;

    return logpanels_.get( panelid );
}


mImplMonitorableAssignment(Well::DisplayProperties2D, Well::DisplayProperties);

void Well::DisplayProperties2D::copyClassData(
				const Well::DisplayProperties2D& oth )
{
    logpanels_.setEmpty();
    for ( int lpidx=0; lpidx<oth.logpanels_.size(); lpidx++ )
    {
	LogPanelProps* logpanel = new LogPanelProps( *oth.getLogPanel(lpidx) );
	logpanels_ += logpanel;
    }
}


Monitorable::ChangeType Well::DisplayProperties2D::compareClassData(
				const Well::DisplayProperties2D& oth ) const
{
    return cEntireObjectChange();
}


void Well::DisplayProperties2D::usePar( const IOPar& iop )
{
    PtrMan<IOPar> disp2ddisppar = iop.subselect( sKey2DDispProp() );
    if ( !disp2ddisppar )
	return;

    PtrMan<IOPar> logsubjnmpar = disp2ddisppar->subselect( sKey::Log() );
    if ( !logsubjnmpar )
	return;

    int nrpanels = 0;
    logsubjnmpar->get( sKeyNrPanels, nrpanels );
    if ( !nrpanels )
	return;

    logpanels_.setEmpty();
    for ( int pidx=0; pidx<nrpanels; pidx++ )
    {
	addLogPanel();
	BufferString panelkey( "Panel", pidx );
	PtrMan<IOPar> logpar = logsubjnmpar->subselect( panelkey );
	logpanels_.get( pidx )->usePar( *logpar );
    }
}


void Well::DisplayProperties2D::fillPar( IOPar& iop ) const
{
    DisplayProperties::fillPar( iop );

    IOPar logpars;
    logpars.set( sKeyNrPanels, logpanels_.size() );
    for ( int pidx=0; pidx<logpanels_.size(); pidx++ )
    {
	const LogPanelProps* panel = logpanels_.get( pidx );
	if ( !panel || panel->logs_.isEmpty() )
	    continue;

	IOPar panelpar;
	panel->fillPar( panelpar );
	const BufferString panelidxkey = IOPar::compKey( "Panel", pidx );
	logpars.mergeComp( panelpar, panelidxkey );
    }
}


//MarkerDispProps
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

/* TODO remove
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
*/


#define mGetLRIOpKey(ky) mGetIOPKey( gtLRKy(isleft,ky) )
/*
//TODO Need to support old format
void Well::LogDispProps::oldUsePar( const IOPar& iop, bool isleft )
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
    bool isflipped = ColTab::isFlipped( sequsemode_ );
    bool iscyclic = ColTab::isCyclic( sequsemode_ );
    iop.getYN( mGetLRIOpKey(sKeyColTabFlipped), isflipped );
    iop.getYN( mGetLRIOpKey(sKeyColTabCyclic), iscyclic );
    sequsemode_ = ColTab::getSeqUseMode( isflipped, iscyclic );

    float logwidth = 250.f;
    iop.get( mGetLRIOpKey(sKeyLogWidthXY), logwidth );
    if ( SI().xyInFeet() )
	logwidth *= mToFeetFactorF;
    logwidth_ = mNINT32( logwidth );
}
*/


void Well::LogDispProps::usePar( const IOPar& iop )
{
    mLock4Write();
    baseUsePar( iop, sKeyLogNmFont, "Log Name Size" );

    iop.get( sKeyLogName, logname_ );
    iop.get( sKeyRange, range_ );
    iop.get( sKeyFillName, fillname_ );
    iop.get( sKeyFillRange, fillrange_ );
    iop.getYN( sKeyFillLeftYN, isleftfill_ );
    iop.getYN( sKeyFillRightYN, isrightfill_ );

    iop.getYN( sKeyRevertRange, islogreverted_);
    iop.get( sKeyCliprate, cliprate_ );
    iop.getYN( sKeySingleCol, issinglecol_ );
    iop.getYN( sKeyDataRange, isdatarange_ );
    iop.get( sKeyRepeatLog, repeat_ );
    iop.get( sKeyOverlap, repeatovlap_ );
    iop.get( sKeySeisColor, seiscolor_ );
    iop.get( sKeySeqname, seqname_ );
    iop.getYN( sKeyScale, islogarithmic_ );
    iop.get( sKeyLogStyle,style_);
    bool isflipped = ColTab::isFlipped( sequsemode_ );
    bool iscyclic = ColTab::isCyclic( sequsemode_ );
    iop.getYN( sKeyColTabFlipped, isflipped );
    iop.getYN( sKeyColTabCyclic, iscyclic );
    sequsemode_ = ColTab::getSeqUseMode( isflipped, iscyclic );

    float logwidth = 250.f;
    iop.get( sKeyLogWidthXY, logwidth );
    if ( SI().xyInFeet() )
	logwidth *= mToFeetFactorF;

    logwidth_ = mNINT32( logwidth );
}


//void Well::LogDispProps::fillPar( IOPar& iop, bool isleft ) const
void Well::LogDispProps::fillPar( IOPar& iop ) const
{
    mLock4Read();
    baseFillPar( iop, sKeyLogNmFont );

    float logwidth = (float)logwidth_;
    if ( SI().xyInFeet() )
	logwidth *= mFromFeetFactorF;

    iop.set( sKeyLogName, logname_ );
    iop.set( sKeyRange, range_ );
    iop.set( sKeyFillName, fillname_ );
    iop.set( sKeyFillRange, fillrange_ );

    iop.setYN( sKeyFillLeftYN, isleftfill_ );
    iop.setYN( sKeyFillRightYN, isrightfill_ );

    iop.setYN( sKeyRevertRange,islogreverted_);
    iop.set( sKeyCliprate, cliprate_ );
    iop.setYN( sKeySingleCol, issinglecol_ );
    iop.setYN( sKeyDataRange, isdatarange_ );
    iop.set( sKeyRepeatLog, repeat_ );
    iop.set( sKeyOverlap, repeatovlap_ );
    iop.set( sKeySeisColor, seiscolor_ );
    iop.set( sKeySeqname, seqname_ );
    iop.setYN( sKeyScale, islogarithmic_ );
    iop.setYN( sKeyColTabFlipped, ColTab::isFlipped(sequsemode_));
    iop.setYN( sKeyColTabCyclic, ColTab::isCyclic(sequsemode_) );
    iop.set( sKeyLogStyle, style_ );
    iop.set( sKeyLogWidthXY, logwidth );
}
