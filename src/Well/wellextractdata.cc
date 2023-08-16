/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "wellextractdata.h"

#include "arrayndimpl.h"
#include "binidvalue.h"
#include "ctxtioobj.h"
#include "datacoldef.h"
#include "datapointset.h"
#include "envvars.h"
#include "filepath.h"
#include "iodir.h"
#include "iodirentry.h"
#include "ioman.h"
#include "ioobj.h"
#include "iopar.h"
#include "multiid.h"
#include "oddirs.h"
#include "posvecdataset.h"
#include "ptrman.h"
#include "sorting.h"
#include "strmprov.h"
#include "surveydisklocation.h"
#include "survinfo.h"
#include "welld2tmodel.h"
#include "welllog.h"
#include "welllogset.h"
#include "wellman.h"
#include "wellmarker.h"
#include "wellreader.h"
#include "welltrack.h"
#include "welltransl.h"

#include <math.h>

#define mLocalEps 1e-2f;

namespace Well
{
const char* ZRangeSelector::sKeyTopMrk()    { return "Top marker"; }
const char* ZRangeSelector::sKeyBotMrk()    { return "Bottom marker"; }
const char* ZRangeSelector::sKeyLimits()    { return "Extraction extension"; }
const char* ZRangeSelector::sKeyDataStart() { return "<Start of data>"; }
const char* ZRangeSelector::sKeyDataEnd()   { return "<End of data>"; }
const char* ZRangeSelector::sKeyZRange()    { return "Z range"; }
const char* ZRangeSelector::sKeyZSelection(){ return "Z Selection"; }
const char* ZRangeSelector::sKeySnapZRangeToSurvey()
					    { return "Snap Z Range to SI"; }
const char* ExtractParams::sKeyZExtractInTime() { return "Extract Z in time"; }
const char* ExtractParams::sKeySamplePol()  { return "Data sampling"; }
const char* TrackSampler::sKeyLogNm()	    { return "Log name"; }
const char* TrackSampler::sKeySelRadius()   { return "Selection radius"; }
const char* TrackSampler::sKeyFor2D()	    { return "For 2D"; }
const char* TrackSampler::sKeyDahCol()	    { return "Create MD column"; }
const char* LogDataExtracter::sKeyLogNm()   { return
					      TrackSampler::sKeyLogNm(); }

mDefineEnumUtils(ZRangeSelector,ZSelection,"Type of selection")
{ "Markers", "Depth range", "Time range", nullptr };

} // namespace Well

static const char* sKeyDAHColName()	{ return "<MD>"; }

Well::InfoCollector::InfoCollector( bool dologs, bool domarkers, bool dotracks )
    : Executor("Well information extraction")
    , domrkrs_(domarkers)
    , dologs_(dologs)
    , dotracks_(dotracks)
    , curidx_(0)
    , survloc_(*new SurveyDiskLocation)
{
    iodir_ = nullptr;
    direntries_ = nullptr;
    totalnr_ = 0;
    curmsg_ = totalnr_ ? tr("Gathering information") : tr("No wells");
}


Well::InfoCollector::~InfoCollector()
{
    deepErase( infos_ );
    deepErase( markers_ );
    deepErase( logs_ );
    delete direntries_;
    delete iodir_;
    delete &survloc_;
}


void Well::InfoCollector::getAllMarkerInfos( BufferStringSet& nms,
					     TypeSet<OD::Color>& colors ) const
{
    nms.setEmpty();
    if ( markers_.isEmpty() )
	return;

    MarkerSet wms( *markers_.get(0) );
    for ( auto idx=1; idx<markers_.size(); idx++ )
	wms.append( *markers_.get(idx) );

    wms.getNames( nms );
    wms.getColors( colors );
}


void Well::InfoCollector::getAllLogNames( BufferStringSet& nms ) const
{
    nms.setEmpty();
    for ( auto lognms : logs_ )
	for ( auto lognm : *lognms )
	    nms.addIfNew( *lognm );
}


void Well::InfoCollector::setSurvey( const SurveyDiskLocation& sdl )
{
    survloc_ = sdl;
}


SurveyDiskLocation& Well::InfoCollector::survey() const
{
    return survloc_;
}


int Well::InfoCollector::nextStep()
{
    SurveyChanger chgr( survey() );

    if ( !iodir_ )
    {
	const IOObjContext ctxt = mIOObjContext( Well );
	iodir_ = new IODir( ctxt.getSelKey() );
	direntries_ = new IODirEntryList( *iodir_, ctxt );
	totalnr_ = direntries_->size();
    }

    if ( curidx_ >= totalnr_ )
	return Finished();

    const IOObj* ioobj = (*direntries_)[curidx_]->ioobj_;
    const MultiID wmid( ioobj->key() );

    BufferStringSet lognms;
    RefMan<Data> wd;
    bool res;
    if ( survey().isCurrentSurvey() )
    {
	LoadReqs reqs( false );
	if ( dotracks_ )
	    reqs.include( Trck );
	if ( domrkrs_ )
	    reqs.include( Mrkrs );
	if ( dologs_ )
	    reqs.include( LogInfos );

	wd = MGR().get( wmid, reqs );
	res = wd;
	if ( res && dologs_ )
	    wd->logs().getNames( lognms, false );
	else if ( !res )
	    curmsg_ = toUiString( MGR().errMsg() );
    }
    else
    {
	wd = new Data;
	Reader rdr( *ioobj, *wd );
	res = true;
	if ( res && dotracks_ )
	    res = rdr.getTrack();
	if ( res && domrkrs_ )
	    res = rdr.getMarkers();
	if ( res && dologs_ )
	    rdr.getLogInfo( lognms );
	if ( !res )
	    curmsg_ = rdr.errMsg();
    }

    if ( !res )
	return ++curidx_ >= totalnr_ ? Finished() : MoreToDo();

    ids_ += wmid;
    infos_ += new Info( wd->info() );
    if ( dotracks_ )
    {
	const Track& trk = wd->track();
	if ( mIsUdf(trackstvdrg_.start) )
	    trackstvdrg_.setFrom( trk.zRange() );
	else
	{
	    Interval<float> tvdrg( trk.zRange() );
	    if ( !mIsUdf( tvdrg.start ) )
		trackstvdrg_.include( tvdrg );
	}
    }

    if ( domrkrs_ )
    {
	auto* newset = new MarkerSet;
	*newset = wd->markers();
	markers_ += newset;
    }

    if ( dologs_ )
    {
	auto* newlognms = new BufferStringSet;
	*newlognms = lognms;
	logs_ += newlognms;
    }

    return ++curidx_ >= totalnr_ ? Finished() : MoreToDo();
}


#define mErrRet(msg) { if ( errmsg ) *errmsg = msg; return false; }
bool Well::ZRangeSelector::isOK( uiString* errmsg ) const
{
    const bool usemrkr = zselection_ == Markers;
    if ( ( usemrkr && topmrkr_.isEqual(botmrkr_)
		&& mIsEqual(above_,below_,SI().zStep()) )
		    || ( !usemrkr && fixedzrg_.width() < SI().zStep()) )
	mErrRet( tr("Top distance is equal to bottom distance") );

    return true;
}



Well::ZRangeSelector::ZRangeSelector( const ZRangeSelector& p )
{
    topmrkr_ = p.topmrkr_;
    botmrkr_ = p.botmrkr_;
    above_ = p.above_;
    below_ = p.below_;
    zselection_ = p.zselection_;
    fixedzrg_ = p.fixedzrg_;
    snapzrgtosurvey_ = p.snapzrgtosurvey_;
}


Well::ZRangeSelector::~ZRangeSelector()
{}


void Well::ZRangeSelector::setEmpty()
{
    topmrkr_ = sKeyDataStart();
    botmrkr_ = sKeyDataEnd();
    above_ = below_ = 0;
    zselection_ = Markers;
    snapzrgtosurvey_ = false;
    fixedzrg_ = Interval<float>( mUdf(float), mUdf(float) );
}


void Well::ZRangeSelector::usePar( const IOPar& pars )
{
    pars.get( sKeyTopMrk(), topmrkr_ );
    pars.get( sKeyBotMrk(), botmrkr_ );
    pars.get( sKeyLimits(), above_, below_ );
    pars.get( sKeyZRange(), fixedzrg_ );
    pars.getYN( sKeySnapZRangeToSurvey(), snapzrgtosurvey_ );
    parseEnumZSelection( pars.find( sKeyZSelection() ), zselection_ );
}


void Well::ZRangeSelector::fillPar( IOPar& pars ) const
{
    pars.set( sKeyTopMrk(), topmrkr_ );
    pars.set( sKeyBotMrk(), botmrkr_ );
    pars.set( sKeyLimits(), above_, below_ );
    pars.set( sKeyZRange(), fixedzrg_ );
    pars.setYN( sKeySnapZRangeToSurvey(), snapzrgtosurvey_ );
    pars.set( sKeyZSelection(), getZSelectionString( zselection_ ) );
}


void Well::ZRangeSelector::setFixedRange( Interval<float> zrg, bool isintime )
{
    fixedzrg_ = zrg; zselection_ = isintime ? Times : Depths;
}


void Well::ZRangeSelector::setMarker( bool top, BufferString nm, float offset )
{
    if ( top )
	{ topmrkr_ = nm; above_ = offset; }
    else
	{ botmrkr_ = nm; below_ = offset; }
}


void Well::ZRangeSelector::snapZRangeToSurvey(Interval<float>& zrg,bool zistime,
					      const D2TModel* d2t,
					      const Track& track) const
{
    if ( !snapzrgtosurvey_ )
	return;

    const StepInterval<float> survrg = SI().zRange(false);
    if ( SI().zIsTime() && !zistime )
    {
	if ( !d2t ) return;
	zrg.start = survrg.snap( d2t->getTime( zrg.start, track ) );
	zrg.stop = survrg.snap( d2t->getTime( zrg.stop, track ) );
	zrg.start = d2t->getDah( zrg.start, track );
	zrg.stop = d2t->getDah( zrg.stop, track );
    }
    else
    {
	SI().snapZ( zrg.start, 1 );
	SI().snapZ( zrg.stop, -1 );
    }
}


#define mDah2TVD(dah,tvd)\
{\
    if ( !wd->track().dahRange().includes(dah,true) )\
	tvd = dah < wd->track().dahRange().start \
	    ? wd->track().zRange().start \
	    : wd->track().zRange().stop ; \
    else \
	tvd = mCast( float, track.getPos( dah ).z ); \
}


Interval<float> Well::ZRangeSelector::calcFrom( const Data& wdin,
			    const BufferStringSet& lognms, bool todah ) const
{
    ConstRefMan<Data> wd( &wdin );
    if ( zselection_ == Times )
    {
	Interval<float> rg( fixedzrg_ );
	snapZRangeToSurvey( rg, true, wd->d2TModel(), wd->track() );
	return rg;
    }

    Interval<float> dahrg( mUdf(float), mUdf(float) );

    const Track& track = wd->track();
    if ( track.isEmpty() )
	return dahrg;

    if (  zselection_ == Depths )
    {
	if ( todah )
	{
	    dahrg.start = track.getDahForTVD( fixedzrg_.start );
	    dahrg.stop = track.getDahForTVD( fixedzrg_.stop );
	    dahrg.limitTo( track.dahRange() );
	}
	else
	    dahrg = fixedzrg_;

	snapZRangeToSurvey( dahrg, false, 0, wd->track() );
	return dahrg;
    }

    int ilog = 0;

    if ( lognms.isEmpty() )
	{ dahrg = wd->track().dahRange(); }

    for ( ; mIsUdf(dahrg.start) && ilog<lognms.size(); ilog++ )
    {
	const Log* log = wd->logs().getLog( lognms.get(ilog).buf() );
	if ( !log || log->isEmpty() )  continue;

	dahrg = log->dahRange();
    }
    for ( ; ilog<lognms.size(); ilog++ )
    {
	const Log* log = wd->logs().getLog( lognms.get(ilog).buf() );
	if ( !log || log->isEmpty() )  continue;

	Interval<float> newdahrg = log->dahRange();
	if ( mIsUdf(newdahrg.start) ) continue;
	dahrg.include( newdahrg );
    }

    getMarkerRange( *wd, dahrg );
    if ( !todah )
    {
	mDah2TVD( dahrg.start, dahrg.start );
	mDah2TVD( dahrg.stop, dahrg.stop );
    }
    snapZRangeToSurvey( dahrg, false, 0, wd->track() );
    return dahrg;
}


Interval<float> Well::ZRangeSelector::calcFrom( const Data& wdin,
			    const LogSet& logset, bool todah ) const
{
    ConstRefMan<Data> wd( &wdin );
    if ( zselection_ == Times )
    {
	Interval<float> rg( fixedzrg_ );
	snapZRangeToSurvey( rg, true, wd->d2TModel(), wd->track() );
	return rg;
    }

    Interval<float> dahrg( mUdf(float), mUdf(float) );

    const Track& track = wd->track();
    if ( track.isEmpty() )
	return dahrg;

    if (  zselection_ == Depths )
    {
	if ( todah )
	{
	    dahrg.start = track.getDahForTVD( fixedzrg_.start );
	    dahrg.stop = track.getDahForTVD( fixedzrg_.stop );
	    dahrg.limitTo( track.dahRange() );
	}
	else
	    dahrg = fixedzrg_;

	snapZRangeToSurvey( dahrg, false, 0, wd->track() );
	return dahrg;
    }

    dahrg.include( logset.dahInterval() );

    getMarkerRange( *wd, dahrg );
    if ( !todah )
    {
	mDah2TVD( dahrg.start, dahrg.start );
	mDah2TVD( dahrg.stop, dahrg.stop );
    }
    snapZRangeToSurvey( dahrg, false, 0, wd->track() );
    return dahrg;
}


void Well::ZRangeSelector::getMarkerRange( const Data& wdin,
					Interval<float>& zrg ) const
{
    ConstRefMan<Data> wd( &wdin );
    Interval<float> newzrg = zrg;

    getLimitPos(wd->markers(),true,newzrg.start,zrg);
    getLimitPos(wd->markers(),false,newzrg.stop,zrg);

    zrg = newzrg;
    if ( zrg.start > zrg.stop ) Swap( zrg.start, zrg.stop );
}


void Well::ZRangeSelector::getLimitPos( const MarkerSet& markers,
				      bool isstart, float& val,
				      const Interval<float>& zrg ) const
{
    const BufferString& mrknm = isstart ? topmrkr_ : botmrkr_;
    if ( mrknm == ZRangeSelector::sKeyDataStart() )
	val = zrg.start;
    else if ( mrknm == ZRangeSelector::sKeyDataEnd() )
	val = zrg.stop;
    else
    {
	mSetUdf(val);
	for ( int idx=0; idx<markers.size(); idx++ )
	{
	    if ( markers[idx]->name() == mrknm )
	    {
		val = markers[idx]->dah();
		break;
	    }
	}
    }

    float shft = isstart ? (mIsUdf(above_) ? above_ : -above_) : below_;
    if ( mIsUdf(val) )
	return;

    if ( !mIsUdf(shft) )
	val += shft;
}



Well::ExtractParams::ExtractParams( const ExtractParams& ep )
{
    *this = ep;
    extractzintime_ = ep.extractzintime_;
    samppol_ = ep.samppol_;
}


Well::ExtractParams::~ExtractParams()
{}


void Well::ExtractParams::setEmpty()
{
    ZRangeSelector::setEmpty();
    zstep_ = 1;
    extractzintime_ = false;
    samppol_ = Stats::UseAvg;
}


bool Well::ExtractParams::isOK( uiString* errmsg ) const
{
    if ( !mIsUdf( zstep_ ) && zstep_ < 0 )
	 mErrRet( uiStrings::phrEnter(tr("a valid step value")) )

    return ZRangeSelector::isOK( errmsg );
}


void Well::ExtractParams::usePar( const IOPar& pars )
{
    ZRangeSelector::usePar( pars );
    pars.getYN( sKeyZExtractInTime(), extractzintime_ );
    parseEnumUpscaleType( pars.find( sKeySamplePol() ), samppol_ );
}


void Well::ExtractParams::fillPar( IOPar& pars ) const
{
    ZRangeSelector::fillPar( pars );
    pars.setYN( sKeyZExtractInTime(), extractzintime_ );
    pars.set( sKeySamplePol(), getUpscaleTypeString( samppol_ ) );
}



Well::TrackSampler::TrackSampler( const TypeSet<MultiID>& i,
				  ObjectSet<DataPointSet>& d,
				  bool ztm )
	: Executor("Well data extraction")
	, locradius_(0)
	, ids_(i)
	, dpss_(d)
	, curid_(0)
	, zistime_(ztm)
	, for2d_(false)
	, minidps_(false)
	, mkdahcol_(false)
	, dahcolnr_(-1)
{
}


Well::TrackSampler::~TrackSampler()
{}


void Well::TrackSampler::usePar( const IOPar& pars )
{
    params_.usePar( pars );
    pars.get( sKeyLogNm(), lognms_ );
    pars.get( sKeySelRadius(), locradius_ );
    pars.getYN( sKeyDahCol(), mkdahcol_ );
    pars.getYN( sKeyFor2D(), for2d_ );
}


static int closeDPSS( ObjectSet<DataPointSet>& dpss )
{
    for ( int idx=0; idx<dpss.size(); idx++ )
	dpss[idx]->dataChanged();
    return Executor::Finished();
}

#define mRetNext() { \
    curid_++; \
    return curid_ >= ids_.size() ? closeDPSS(dpss_) : MoreToDo(); }

int Well::TrackSampler::nextStep()
{
    if ( curid_ >= ids_.size() )
	return 0;

    if ( lognms_.isEmpty() )
    {
	errmsg_ = tr("No well logs specified");
	return ErrorOccurred();
    }

    auto* dps = new DataPointSet( for2d_, minidps_ );
    dpss_ += dps;
    if ( !mkdahcol_ )
	dahcolnr_ = -1;
    else
    {
	dps->dataSet().add( new DataColDef(sKeyDAHColName(), nullptr,
			       UnitOfMeasure::surveyDefDepthStorageUnit()) );
	dahcolnr_ = dps->nrCols() - 1;
    }
    LoadReqs lreqs( Trck, D2T, Mrkrs );
    lreqs.add( LogInfos );
    RefMan<Data> wd = MGR().get( ids_.get(curid_), lreqs );
    if ( !wd )
    {
	errmsg_ = mToUiStringTodo(MGR().errMsg());
	mRetNext()
    }

    zrg_ = params_.calcFrom( *wd, lognms_ );
    if ( zrg_.isUdf() )
	mRetNext()

    getData( *wd, *dps );
    mRetNext();
}


void Well::TrackSampler::getData( const Data& wdin, DataPointSet& dps )
{
    ConstRefMan<Data> wd( &wdin );
    const D2TModel* d2t = wd->d2TModel();
    const Track& track = wd->track();
    const bool zrgistime = params_.zselection_ == ZRangeSelector::Times && d2t;
    const bool extractintime = params_.extractzintime_ && d2t && SI().zIsTime();

    float zincr = params_.zstep_;
    if ( mIsUdf( zincr ) )
    {
	zincr = SI().zStep();
	if ( !extractintime )
	{
	    zincr *= .5;
	    if ( SI().zIsTime() )
		zincr *= 2000; //As dx = v * dt, Using v = 2000 m/s
	}
    }

    Interval<float> dahrg;
    dahrg.start = zrgistime ? d2t->getDah( zrg_.start, track ) : zrg_.start;
    dahrg.stop = zrgistime ? d2t->getDah( zrg_.stop, track ) : zrg_.stop;
    float zpos = dahrg.start;
    if ( extractintime )
	zpos = zrgistime ? zrg_.start : d2t->getTime( zpos, track );

    zpos -= zincr;

    int trackidx = 0; Coord3 precisepos;
    BinIDValue biv;
    BinIDValue prevbiv; mSetUdf(prevbiv.inl());

    dahrg.start -= mLocalEps;
    dahrg.stop  += mLocalEps;
    while ( true )
    {
	zpos += zincr;
	const float dah = extractintime ? d2t->getDah( zpos, track ) :zpos;
	if ( mIsUdf(dah) || !dahrg.includes(dah,true) )
	    return;
	else if ( !getPos(*wd,dah,biv,trackidx,precisepos) )
	    continue;

	if ( biv != prevbiv )
	{
	    addPosns( dps, biv, precisepos, dah );
	    prevbiv = biv;
	}
    }
}


bool Well::TrackSampler::getPos( const Data& wdin, float dah,
				 BinIDValue& biv, int& trackidx,
				 Coord3& pos ) const
{
    ConstRefMan<Data> wd( &wdin );
    const int tracksz = wd->track().size();
    while ( trackidx < tracksz && dah > wd->track().dah(trackidx) )
	trackidx++;
    if ( trackidx < 1 || trackidx >= tracksz )
	return false;

    // Position is between trackidx and trackidx-1
    pos = wd->track().coordAfterIdx( dah, trackidx-1 );
    biv.set( SI().transform(pos) );
    if ( SI().zIsTime() && wd->d2TModel() )
    {
	pos.z = mCast( double, wd->d2TModel()->getTime( dah, wd->track() ) );
	if ( mIsUdf(pos.z) )
	    return false;
    }

    biv.set( (float)pos.z );
    return true;
}


void Well::TrackSampler::addPosns( DataPointSet& dps, const BinIDValue& biv,
				  const Coord3& precisepos, float dah ) const
{
    DataPointSet::DataRow dr;
    if ( dahcolnr_ >= 0 )
	dr.data_ += dah;
#define mAddRow(bv,pos) \
    dr.pos_.z_ = bv.val(); dr.pos_.set( pos ); dps.addRow( dr )

    mAddRow( biv, precisepos );
    if ( mIsUdf(locradius_) || locradius_ < 1e-3 )
	return;

    const float sqrlocradius = locradius_ * locradius_;

#define mTryAddRow(stmt) \
{ \
    stmt; \
    crd = SI().transform( newbiv ); \
    if ( crd.sqDistTo(precisepos) <= sqrlocradius ) \
	{ mAddRow(biv,crd); nradded++; } \
}

    BinIDValue newbiv( biv ); Coord crd;

    for ( int idist=1; ; idist++ )
    {
	int nradded = 0;

	newbiv.crl() = biv.crl() - idist;
	for ( int iinl=-idist; iinl<=idist; iinl++ )
	    mTryAddRow(newbiv.inl() = biv.inl() + iinl)
	newbiv.crl() = biv.crl() + idist;
	for ( int iinl=-idist; iinl<=idist; iinl++ )
	    mTryAddRow(newbiv.inl() = biv.inl() + iinl)
	newbiv.inl() = biv.inl() + idist;
	for ( int icrl=1-idist; icrl<idist; icrl++ )
	    mTryAddRow(newbiv.crl() = biv.crl() + icrl)
	newbiv.inl() = biv.inl() - idist;
	for ( int icrl=1-idist; icrl<idist; icrl++ )
	    mTryAddRow(newbiv.crl() = biv.crl() + icrl)

	if ( nradded == 0 ) break;
    }
}


Well::LogDataExtracter::LogDataExtracter( const TypeSet<MultiID>& i,
					  ObjectSet<DataPointSet>& d,
					  bool ztm )
	: Executor("Well log data extraction")
	, ids_(i)
	, dpss_(d)
	, samppol_(Stats::UseAvg)
	, curid_(0)
	, zistime_(ztm)
{
}


Well::LogDataExtracter::~LogDataExtracter()
{}


void Well::LogDataExtracter::usePar( const IOPar& pars )
{
    pars.get( sKeyLogNm(), lognm_ );
    parseEnumUpscaleType(
	    pars.find( ExtractParams::sKeySamplePol() ), samppol_ );
}


#undef mRetNext
#define mRetNext() { \
    curid_++; \
    return curid_ >= ids_.size() ? Finished() : MoreToDo(); }

int Well::LogDataExtracter::nextStep()
{
    if ( curid_ >= ids_.size() )
	return 0;
    if ( msg_.isEmpty() )
    {
	msg_ = tr("Extracting '%1'").arg(lognm_);
	return MoreToDo();
    }

    if ( dpss_.size() <= curid_ ) mRetNext()
    DataPointSet& dps = *dpss_[curid_];
    if ( dps.isEmpty() ) mRetNext()

    RefMan<Data> wd = MGR().get( ids_.get(curid_), LoadReqs(Trck, LogInfos) );
    Track* track = nullptr;
    if ( !wd )
    {
	msg_ = mToUiStringTodo(MGR().errMsg());
	mRetNext()
    }

    if ( zistime_ )
    {
	track = new Track;
	track->toTime( *wd );
    }
    else
	track = &wd->track();

    getData( dps, *wd, *track );
    if( zistime_ ) delete track;

    mRetNext();
}


#define mDefWinSz SI().zIsTime() ? wl.dahStep(true)*20 : SI().zStep()


void Well::LogDataExtracter::getData( DataPointSet& dps,
				      const Data& wdin,
				      const Track& track )
{
    ConstRefMan<Data> wd( &wdin );
    const Log* wlog = wd->getLog( lognm_ );
    if ( !wlog )
	return;
    const Log& wl = *wlog;
    DataPointSet::ColID dpscolidx = dps.indexOf( lognm_ );
    if ( dpscolidx < 0 )
    {
	dps.dataSet().add( new DataColDef(lognm_, nullptr,
							  wl.unitOfMeasure()) );
	dpscolidx = dps.nrCols() - 1;
	if ( dpscolidx < 0 ) return;
    }

    const int dahcolidx = dps.indexOf( sKeyDAHColName() );
    bool usegenalgo = dahcolidx >= 0 || !track.alwaysDownward();
    int opt = GetEnvVarIVal("DTECT_LOG_EXTR_ALGO",0);
    if ( opt == 1 ) usegenalgo = false;
    if ( opt == 2 ) usegenalgo = true;

    if ( usegenalgo )
    {
	// Very fast if dahcolidx >= 0, otherwise much slower, less precise
	getGenTrackData( dps, track, wl, dpscolidx, dahcolidx );
	return;
    }

    // The idea here is to calculate the dah from the Z only.
    // Should be OK for all wells without horizontal sections

    int trackidx = 0;
    float z1 = track.value( trackidx );

    int dpsrowidx = 0; float dpsz = 0;
    for ( ; dpsrowidx<dps.size(); dpsrowidx++ )
    {
	dpsz = dps.z(dpsrowidx);
	if ( dpsz >= z1 )
	    break;
    }
    if ( dpsrowidx >= dps.size() ) // Duh. All data below track.
	return;

    for ( trackidx=0; trackidx<track.size(); trackidx++ )
    {
	if ( track.value(trackidx) > dpsz )
	    break;
    }
    if ( trackidx >= track.size() ) // Duh. Entire track below data.
	return;

    float prevwinsz = mDefWinSz;
    float prevdah = mUdf(float);
    for ( ; dpsrowidx<dps.size(); dpsrowidx++ )
    {
	dpsz = dps.z(dpsrowidx);
	float z2 = track.value( trackidx );
	while ( dpsz > z2 )
	{
	    trackidx++;
	    if ( trackidx >= track.size() )
		return;
	    z2 = track.value( trackidx );
	}
	if ( trackidx == 0 ) // Huh?
	    continue;

	z1 = track.value( trackidx - 1 );
	if ( z1 > dpsz )
	{
	    // This is not uncommon. A new binid with higher posns.
	    trackidx = 0;
	    if ( dpsrowidx > 0 ) dpsrowidx--;
	    mSetUdf(prevdah);
	    continue;
	}

	float dah = ( (z2-dpsz) * track.dah(trackidx-1)
		    + (dpsz-z1) * track.dah(trackidx) )
		    / (z2 - z1);

	float winsz = mIsUdf(prevdah) ? prevwinsz : dah - prevdah;
	addValAtDah( dah, wl, winsz, dps, dpscolidx, dpsrowidx );
	prevwinsz = winsz;
	prevdah = dah;
    }
}


void Well::LogDataExtracter::getGenTrackData( DataPointSet& dps,
					      const Track& track,
					      const Log& wl,
					      int dpscolidx, int dahcolidx )
{
    if ( dps.isEmpty() || track.isEmpty() )
	return;

    float prevwinsz = mDefWinSz;
    float prevdah = mUdf(float);
    DataPointSet::RowID dpsrowidx = 0;
    for ( ; dpsrowidx<dps.size(); dpsrowidx++ )
    {
	Coord3 coord( dps.coord(dpsrowidx), dps.z(dpsrowidx) );
	float dah = dahcolidx >= 0 ? dps.value(dahcolidx,dpsrowidx)
				   : track.nearestDah( coord );
	if ( mIsUdf(dah) )
	    continue;

	float winsz = mIsUdf(prevdah) ? prevwinsz : dah - prevdah;
	if ( winsz <= 0 ) winsz = mDefWinSz;
	addValAtDah( dah, wl, winsz, dps, dpscolidx, dpsrowidx );
	prevwinsz = winsz;
	prevdah = dah;
    }
}


void Well::LogDataExtracter::addValAtDah( float dah, const Log& wl,
					  float winsz, DataPointSet& dps,
					  int dpscolidx, int dpsrowidx ) const
{
    float val = samppol_ == Stats::TakeNearest	?
			wl.getValue( dah ) : calcVal(wl,dah,winsz,samppol_);
    dps.getValues(dpsrowidx)[dpscolidx] = val;
}


float Well::LogDataExtracter::calcVal( const Log& wl, float dah,
				   float winsz, Stats::UpscaleType samppol,
				   float maxholesz, bool logisvel )
{
    if ( samppol == Stats::TakeNearest )
	return wl.getValue( dah, true );

    Interval<float> rg( dah-winsz, dah+winsz ); rg.sort();
    TypeSet<float> vals;
    int startidx = wl.indexOf( rg.start );
    if ( startidx < 0 )
	startidx = 0;
    for ( int idx=startidx; idx<wl.size(); idx++ )
    {
	const float curdah = wl.dah( idx );
	if ( rg.includes(curdah,false) )
	{
	    const float val = wl.value(idx);
	    if ( !mIsUdf(val) )
	    {
		if ( logisvel && val > 1e-5f )
		    vals += 1.f / val;
		else
		    vals += val;
	    }
	}
	else if ( curdah > rg.stop )
	    break;
    }

    const bool iscode = wl.isCode();
    const int sz = vals.size();
    if ( sz < 1 )
    {
	if ( iscode )
	    return wl.getValue( dah, true );
	if ( mIsUdf(maxholesz) )
	    return mUdf(float);

	const float dahdiffbefore = Math::Abs( dah - wl.dah(startidx) );
	int stopidx = startidx+1;
	if ( stopidx >= wl.size() )
	    stopidx = wl.size() > 1 ? wl.size()-1 : 0;
	else if ( stopidx < 0 )
	    stopidx = 0;
	const float dahdiffafter = Math::Abs( wl.dah(stopidx) - dah );
	float val = mUdf(float);
	if ( dahdiffbefore < maxholesz && dahdiffbefore < dahdiffafter )
	    val = wl.value( startidx );
	if ( dahdiffafter < maxholesz && dahdiffafter < dahdiffbefore )
	    val = wl.value( stopidx );
	return !mIsUdf(val) && logisvel && val > 1e-5f ? val = 1.f / val : val;
    }
    if ( sz == 1 )
	return logisvel ? 1.f / vals[0] : vals[0];
    if ( sz == 2 )
	return samppol == Stats::UseAvg && !iscode
		? ( logisvel ? 2.f/(vals[0]+vals[1]) : (vals[0]+vals[1])*0.5f )
		: logisvel ? 1.f / vals[0] : vals[0];

    if ( samppol == Stats::UseMostFreq || iscode )
    {
	TypeSet<float> valsseen;
	TypeSet<int> valsseencount;
	valsseen += vals[0]; valsseencount += 0;
	for ( int idx=1; idx<sz; idx++ )
	{
	    float val = vals[idx];
	    for ( int ivs=0; ivs<valsseen.size(); ivs++ )
	    {
		float vs = valsseen[ivs];
		if ( mIsEqual(vs,val,mDefEps) )
		    { valsseencount[ivs]++; break; }
		if ( ivs == valsseen.size()-1 )
		    { valsseen += val; valsseencount += 0; }
	    }
	}

	int maxvsidx = 0;
	for ( int idx=1; idx<valsseencount.size(); idx++ )
	{
	    if ( valsseencount[idx] > valsseencount[maxvsidx] )
		maxvsidx = idx;
	}
	return logisvel ? 1.f / valsseen[ maxvsidx ] : valsseen[ maxvsidx ];
    }
    else if ( samppol == Stats::UseMed )
    {
	sort_array( vals.arr(), sz );
	return logisvel ? 1.f / vals[sz/2] : vals[sz/2];
    }
    else if ( samppol == Stats::UseAvg )
    {
	float val = 0;
	for ( int idx=0; idx<sz; idx++ )
	    val += vals[idx];
	return logisvel ? (float)sz / val : val / (float)sz;
    }
    else if ( samppol == Stats::UseRMS )
    {
	float val = 0;
	for ( int idx=0; idx<sz; idx++ )
	    val += vals[idx] * vals[idx];
	return logisvel ? Math::Sqrt( (float)sz / val )
			: Math::Sqrt( val / (float)sz );
    }

    //pErrMsg( "UpscaleType not supported" );
    return vals[0];
}



Well::SimpleTrackSampler::SimpleTrackSampler( const Track& t,
					    const D2TModel* d2t,
					    bool doextrapolate,
					    bool stayinsidesurvey )
    : Executor("Extracting Well track positions")
    , track_(t)
    , d2t_(d2t)
    , isinsidesurvey_(stayinsidesurvey)
    , extrintv_(t.dahRange())
    , extrapolate_(doextrapolate)
    , nrdone_(0)
{
    if ( track_.isEmpty() )
	return;

    float zstop = track_.dah( track_.size()-1 );
    float zstart = track_.dah( 0 );
    if ( d2t )
    {
	zstart = d2t->getTime( zstart, track_ );
	zstop = d2t->getTime( zstop, track_ );
	extrintv_.start = d2t->getTime( extrintv_.start, track_ );
	extrintv_.stop = d2t->getTime( extrintv_.stop, track_ );
	extrintv_.step = SI().zStep();
    }
    tracklimits_.start = zstart;
    tracklimits_.stop = zstop;
}


Well::SimpleTrackSampler::~SimpleTrackSampler()
{}


int Well::SimpleTrackSampler::nextStep()
{
    float zval = extrintv_.atIndex( nrdone_ );
    float dah = d2t_ ? d2t_->getDah( zval, track_ ) : track_.getDahForTVD(zval);

    if ( zval > extrintv_.stop )
	return Executor::Finished();

    Coord3 pos = track_.getPos( dah );
    pos.z = zval;

    BinID bid = SI().transform( pos );
    const bool withintrack = tracklimits_.includes(zval,true);
    if ( withintrack || extrapolate_ )
    {
	if ( extrapolate_ && !withintrack )
	{
	    pos = bidset_.isEmpty() ? track_.pos(0)
				    : track_.pos(track_.size()-1);
	    pos.z = zval;
	    bid = SI().transform( pos );
	}
	if ( ( isinsidesurvey_ && !SI().includes( bid, zval, false ) )
		|| !SI().isReasonable( bid ))
	    { nrdone_++; return Executor::MoreToDo(); }

	bidset_ += bid; coords_ += pos;
    }

    nrdone_ ++;
    return Executor::MoreToDo();
}



Well::LogSampler::LogSampler( const Data& wd,const ExtractParams& pars,
			      const BufferStringSet& lognms )
    : ParallelTask("Resampling logs")
    , wd_(&wd)
    , track_( wd_->track() )
{
    init( wd_->d2TModel(), pars.calcFrom(*wd_,lognms,false), pars.isInTime(),
			  pars.zstep_, pars.extractzintime_, pars.samppol_ );
    for ( int idx=0; idx<lognms.size(); idx++ )
    {
	const Log* log = wd_->getLog( lognms.get( idx ) );
	if ( log ) logset_ += log;
    }
}


Well::LogSampler::LogSampler( const Data& wd, const Interval<float>& zrg,
			      bool zrgisintime,float zstep, bool extrintime,
			      Stats::UpscaleType samppol,
			      const BufferStringSet& lognms )
    : ParallelTask("Resampling logs")
    , wd_(&wd)
    , track_( wd_->track() )
{
    init( wd_->d2TModel(), zrg, zrgisintime, zstep, extrintime, samppol);
    for ( int idx=0; idx<lognms.size(); idx++ )
    {
	const Log* log = wd_->getLog( lognms.get( idx ) );
	if ( log ) logset_ += log;
    }
}


Well::LogSampler::LogSampler( const D2TModel* d2t, const Track* track,
			      const Interval<float>& zrg, bool zrgisintime,
			      float zstep,
			      bool extrintime,Stats::UpscaleType samppol,
			      const ObjectSet<const Log>& logs )
    : ParallelTask("Resampling logs")
    , track_( *track )
{
    init( d2t, zrg, zrgisintime, zstep, extrintime, samppol );
    logset_ = logs;
}


Well::LogSampler::LogSampler( const Data& wd,
			      const ExtractParams& pars,
			      const LogSet& logset,
			      const BufferStringSet& lognms )
    : ParallelTask("Resampling logs")
    , wd_(&wd)
    , track_( wd_->track() )
{
    init( wd_->d2TModel(), pars.calcFrom(*wd_,logset,false), pars.isInTime(),
	    pars.zstep_, pars.extractzintime_, pars.samppol_ );
    for ( auto* lognm : lognms )
    {
	const Log* log = logset.getLog( lognm->buf() );
	if ( log ) logset_ += log;
    }
}


void Well::LogSampler::init( const D2TModel* d2t,
			const Interval<float>& zrg, bool zrgisintime,
			float zstep, bool extrintime,
			Stats::UpscaleType samppol )
{
    d2t_ = d2t;
    data_ = nullptr;
    samppol_ = samppol;
    zrg_ = zrg;
    zrgisintime_ = zrgisintime;
    extrintime_ = extrintime;
    zstep_ = zstep;
}


Well::LogSampler::~LogSampler()
{
    delete data_;
}


uiString Well::LogSampler::uiNrDoneText() const
{
    return tr("Logs done");
}


od_int64 Well::LogSampler::nrIterations() const
{ return logset_.size(); }


#define mGetDah(dah,zvalue,zintime) \
	dah = zintime \
	    ? d2t_->getDah( zvalue, track_ ) \
	    : track_.getDahForTVD( zvalue );


#define mGetZ(zvalue,dah,zintime) \
	zvalue = zintime \
	       ? mCast( float, track_.getPos( dah ).z ) \
	       : d2t_->getTime( dah, track_ );

#undef mErrRet
#define mErrRet(msg) { errmsg_ = msg; return false; }

bool Well::LogSampler::doPrepare( int thread )
{
    if ( !nrIterations() )
	mErrRet( tr("No log found"))

    if ( zrg_.isUdf() )
	mErrRet( tr("No valid range specified") )

    if ( mIsUdf(zstep_) )
	mErrRet( tr("No valid step specified") )

    if ( ( extrintime_ || zrgisintime_ ) && !d2t_ )
	mErrRet( tr("No valid depth/time model found") )

    Interval<float> dahrg;
    mGetDah( dahrg.start, zrg_.start, zrgisintime_ )
    mGetDah( dahrg.stop, zrg_.stop, zrgisintime_ )
    if ( dahrg.isUdf() )
    {
	mErrRet( tr("Wrong extraction boundaries") )
    }

    dahrg.limitTo( track_.dahRange() );
    if ( extrintime_ != zrgisintime_ )
    {
	mGetZ( zrg_.start, dahrg.start, zrgisintime_ )
	mGetZ( zrg_.stop, dahrg.stop, zrgisintime_ )
    } // zrg_ now matches the extraction domain
    zrgisintime_ = extrintime_;

    const int nrsamples = mNINT32( zrg_.width(true) / zstep_ );
    zrg_.stop = zrg_.start + nrsamples * zstep_;
    const StepInterval<float> zrgreg( zrg_.start, zrg_.stop, zstep_ );
    TypeSet<float> dahs;
    TypeSet<float> winsz;
    for ( int idx=0; idx<zrgreg.nrSteps()+1; idx++ )
    {
	const float zmid = zrgreg.atIndex(idx);
	const float ztop = zmid - zstep_/2.f;
	const float zbase = zmid + zstep_/2.f;
	Interval<float> dahwin;
	mGetDah( dahwin.start, ztop, extrintime_ )
	mGetDah( dahwin.stop, zbase, extrintime_ )
	if ( !dahwin.isUdf() )
	{
	    dahs += dahwin.center();
	    winsz += dahwin.width();
	}
    }

    data_ = new Array2DImpl<float>( mCast(int,nrIterations()+2),
				    dahs.size() );
    const int winszidx = mCast(int,nrIterations())+1;
    for ( int idz=0; idz<dahs.size(); idz++ )
    {
	data_->set( 0, idz, dahs[idz] );
	data_->set( winszidx, idz, winsz[idz] );
    }

    return true;
}


bool Well::LogSampler::doWork( od_int64 start, od_int64 stop, int nrthreads )
{
    for ( int idx=mCast(int,start); idx<=stop; idx++ )
    {
	if ( !shouldContinue() )
	    return false;

	if ( !doLog( idx ) )
	    { errmsg_ = tr("One or several logs could not be extracted"); }

	addToNrDone( 1 );
    }
    return true;
}


bool Well::LogSampler::doLog( int logidx )
{
    const Log* log = logset_.validIdx( logidx ) ? logset_[logidx] : nullptr;
    if ( !log || log->isEmpty() || !data_ )
	return false;

    const int winszidx = data_->info().getSize(0)-1;
    const bool nearestinterp = samppol_ == Stats::TakeNearest;
    for ( int idz=0; idz<data_->info().getSize(1); idz++ )
    {
	const float dah = data_->get( 0, idz );
	float lval = mUdf(float);

	const float winsz = nearestinterp ? 0.f : data_->get( winszidx, idz );
	lval = LogDataExtracter::calcVal(*log,dah,winsz,samppol_,maxholesz_);
	data_->set( logidx+1, idz, lval );
    }

    return true;
}


float Well::LogSampler::getDah( int idz ) const
{
    return data_ && data_->info().validDimPos(1,idz)
			? data_->get( 0, idz ) : mUdf(float);
}


float Well::LogSampler::getThickness( int idz ) const
{
    if ( !data_ )
	return mUdf(float);

    const int winszidx = data_->info().getSize(0) - 1;
    return data_->info().validDimPos(1,idz)
			? data_->get( winszidx ,idz ) : mUdf(float);
}


float Well::LogSampler::getLogVal( int logidx, int idz ) const
{
    if ( !data_ )
	return mUdf(float);

    const int xsz = data_->info().getSize(0);
    const int zsz = data_->info().getSize(1);

    return data_ && logidx+1 < xsz && idz < zsz ?
	data_->get( logidx + 1, idz ) : mUdf( float );
}


float Well::LogSampler::getLogVal( const char* lnm, int idz ) const
{
    bool found = false;
    int logidx = 0;
    for ( logidx=0; logidx<logset_.size(); logidx++ )
    {
	if ( logset_[logidx]->name() == lnm )
	    { found = true; break; }
    }
    return found ? getLogVal( logidx, idz ) :  mUdf(float);
}


const char* Well::LogSampler::uomLabel( int logidx ) const
{
    if ( !logset_.validIdx(logidx) || !logset_[logidx] )
	return nullptr;

    const UnitOfMeasure* uom = logset_[logidx]->unitOfMeasure();
    return uom ? uom->symbol() : logset_[logidx]->unitMeasLabel();
}


int Well::LogSampler::nrZSamples() const
{
    return data_ ? data_->info().getSize(1) : 0;
}
