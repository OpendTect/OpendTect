/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : May 2004
-*/


#include "wellextractdata.h"
#include "wellmanager.h"
#include "wellmarker.h"
#include "welltrack.h"
#include "wellinfo.h"
#include "welld2tmodel.h"
#include "welllogset.h"
#include "welltransl.h"
#include "arrayndimpl.h"
#include "datapointset.h"
#include "posvecdataset.h"
#include "survinfo.h"
#include "dbdir.h"
#include "ctxtioobj.h"
#include "datacoldef.h"
#include "dbdir.h"
#include "ioobj.h"
#include "iopar.h"
#include "ptrman.h"
#include "dbkey.h"
#include "sorting.h"
#include "envvars.h"
#include "keystrs.h"
#include "binidvalue.h"
#include "surveydisklocation.h"

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
const char* LogDataExtracter::sKeyLogNm()   { return TrackSampler::sKeyLogNm();}

mDefineEnumUtils(ZRangeSelector,ZSelection,"Type of selection")
{ "Markers", "Depth range", "Time range", 0 };

} // namespace Well

template<>
void EnumDefImpl<Well::ZRangeSelector::ZSelection>::init()
{
    uistrings_ += uiStrings::sMarker(mPlural);
    uistrings_ += uiStrings::sDepthRange();
    uistrings_ += uiStrings::sTimeRange();
}

static const char* sKeyDAHColName()	    { return "<MD>"; }


Well::InfoCollector::InfoCollector( bool dologs, bool domarkers, bool dotracks )
    : Executor("Well information extraction")
    , domrkrs_(domarkers)
    , dologs_(dologs)
    , dotracks_(dotracks)
    , curidx_(0)
    , curmsg_(tr("Gathering information"))
    , totalnr_(-1)
    , direntries_(0)
    , survloc_(*new SurveyDiskLocation)
{
}


Well::InfoCollector::~InfoCollector()
{
    delete direntries_;

    deepErase( infos_ );
    deepErase( markers_ );
    deepErase( lognames_ );
    deepErase( loguoms_ );

    delete &survloc_;
}


void Well::InfoCollector::setSurvey( const SurveyDiskLocation& sdl )
{
    survloc_ = sdl;
}


void Well::InfoCollector::getAllMarkerInfos( BufferStringSet& nms,
					     TypeSet<Color>& colors ) const
{
    nms.setEmpty();
    if ( markers_.isEmpty() )
	return;

    Well::MarkerSet wms( *markers_.get(0) );
    for ( auto idx=1; idx<markers_.size(); idx++ )
	wms.append( *markers_.get(idx) );

    wms.getNames( nms );
    wms.getColors( colors );
}


void Well::InfoCollector::getAllLogNames( BufferStringSet& nms ) const
{
    nms.setEmpty();
    for ( auto lognms : lognames_ )
	for ( auto lognm : *lognms )
	    nms.addIfNew( *lognm );
}


int Well::InfoCollector::nextStep()
{
    if ( !direntries_ )
    {
	direntries_ = new DBDirEntryList( mIOObjContext(Well), survloc_ );
	totalnr_ = direntries_->size();
	if ( totalnr_ < 1 )
	{
	    curmsg_ = tr("No wells");
	    return Finished();
	}
    }

    if ( curidx_ >= totalnr_ )
	return totalnr_ > 0 ? Finished() : ErrorOccurred();

    const DBKey& wky = direntries_->key( curidx_ );
    LoadReqs lreqs( Inf );
    if ( dotracks_ )
	lreqs.add( Trck ).add( D2T );
    if ( domrkrs_ )
	lreqs.add( Mrkrs );

    ConstRefMan<Data> wd = MGR().fetch( wky, lreqs );
    if ( wd )
    {
	ids_ += wky;
	infos_ += new Info( wd->info() );

	if ( dotracks_ )
	{
	    const Track& trk = wd->track();
	    if ( mIsUdf(trackstvdrg_.start) )
		trackstvdrg_.setFrom( trk.zRange() );
	    else
	    {
		Track::ZIntvType tvdrg( trk.zRange() );
		if ( !mIsUdf( tvdrg.start ) )
		    trackstvdrg_.include( tvdrg );
	    }
	}

	if ( domrkrs_ )
	{
	    MarkerSet* newset = new MarkerSet;
	    *newset = wd->markers();
	    markers_ += newset;
	}

	if ( dologs_ )
	{
	    ObjectSet<IOPar> iops;
	    MGR().getLogInfo( wky, iops );

	    auto* lognms = new BufferStringSet;
	    auto* loguomset = new ObjectSet<const UnitOfMeasure>;
	    loguomset->setNullAllowed( true );
	    for ( auto iop : iops )
	    {
		const char* unstr = iop->find( sKey::Unit() );
		const char* nmstr = iop->find( sKey::Name() );
		if ( nmstr && *nmstr )
		{
		    lognms->add( nmstr );
		    *loguomset += UnitOfMeasure::getGuessed( unstr );;
		}
	    }
	    loguoms_ += loguomset;
	    lognames_ += lognms;
	    deepErase( iops );
	}
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


void Well::ZRangeSelector::setEmpty()
{
    topmrkr_ = sKeyDataStart();
    botmrkr_ = sKeyDataEnd();
    above_ = below_ = 0;
    zselection_ = Markers;
    snapzrgtosurvey_ = false;
    fixedzrg_ = Interval<ZType>( mUdf(ZType), mUdf(ZType) );
}


void Well::ZRangeSelector::usePar( const IOPar& pars )
{
    pars.get( sKeyTopMrk(), topmrkr_ );
    pars.get( sKeyBotMrk(), botmrkr_ );
    pars.get( sKeyLimits(), above_, below_ );
    pars.get( sKeyZRange(), fixedzrg_ );
    pars.getYN( sKeySnapZRangeToSurvey(), snapzrgtosurvey_ );
    ZSelectionDef().parse( pars.find( sKeyZSelection() ), zselection_ );
}


void Well::ZRangeSelector::fillPar( IOPar& pars ) const
{
    pars.set( sKeyTopMrk(), topmrkr_ );
    pars.set( sKeyBotMrk(), botmrkr_ );
    pars.set( sKeyLimits(), above_, below_ );
    pars.set( sKeyZRange(), fixedzrg_ );
    pars.setYN( sKeySnapZRangeToSurvey(), snapzrgtosurvey_ );
    pars.set( sKeyZSelection(), toString( zselection_ ) );
}


Well::ZRangeSelector* Well::ZRangeSelector::clone() const
{
    return new Well::ZRangeSelector( *this );
}


void Well::ZRangeSelector::setFixedRange( Interval<ZType> zrg, bool isintime )
{
    fixedzrg_ = zrg; zselection_ = isintime ? Times : Depths;
}


void Well::ZRangeSelector::setMarker( bool top, BufferString nm, ZType offset )
{
    if ( top )
	{ topmrkr_ = nm; above_ = offset; }
    else
	{ botmrkr_ = nm; below_ = offset; }
}


void Well::ZRangeSelector::snapZRangeToSurvey(Interval<ZType>& zrg,bool zistime,
					      const D2TModel& d2t,
					      const Track& track) const
{
    if ( !snapzrgtosurvey_ )
	return;

    const StepInterval<ZType> survrg = SI().zRange();
    if ( SI().zIsTime() && !zistime )
    {
	if ( d2t.isEmpty() )
	    return;
	zrg.start = survrg.snap( d2t.getTime( zrg.start, track ) );
	zrg.stop = survrg.snap( d2t.getTime( zrg.stop, track ) );
	zrg.start = d2t.getDah( zrg.start, track );
	zrg.stop = d2t.getDah( zrg.stop, track );
    }
    else
    {
	SI().snapZ( zrg.start, OD::SnapUpward );
	SI().snapZ( zrg.stop, OD::SnapDownward );
    }
}


#define mDah2TVD(dah,tvd)\
{\
    if ( !wd.track().dahRange().includes(dah,true) )\
	tvd = dah < wd.track().dahRange().start \
	    ? wd.track().zRange().start \
	    : wd.track().zRange().stop ; \
    else \
	tvd = mCast( ZType, track.getPos( dah ).z_ ); \
}


Interval<float> Well::ZRangeSelector::calcFrom( const Data& wd,
			    const BufferStringSet& lognms, bool todah ) const
{
    if ( zselection_ == Times )
    {
	Interval<ZType> rg( fixedzrg_ );
	snapZRangeToSurvey( rg, true, wd.d2TModel(), wd.track() );
	return rg;
    }

    Interval<ZType> dahrg( mUdf(ZType), mUdf(ZType) );

    const Track& track = wd.track();
    MonitorLock mltrack( track );
    if ( track.isEmpty() )
	return dahrg;

    if ( zselection_ == Depths )
    {
	if ( todah )
	{
	    dahrg.start = track.getDahForTVD( fixedzrg_.start );
	    dahrg.stop = track.getDahForTVD( fixedzrg_.stop );
	    dahrg.limitTo( track.dahRange() );
	}
	else
	    dahrg = fixedzrg_;

	snapZRangeToSurvey( dahrg, false, 0, track );
	return dahrg;
    }

    int ilog = 0;

    if ( lognms.isEmpty() )
	dahrg = track.dahRange();

    for ( ; mIsUdf(dahrg.start) && ilog<lognms.size(); ilog++ )
    {
	const Log* log = wd.logs().getLogByName( lognms.get(ilog) );
	if ( !log || log->isEmpty() )
	    continue;

	dahrg = log->dahRange();
    }
    for ( ; ilog<lognms.size(); ilog++ )
    {
	const Log* log = wd.logs().getLogByName( lognms.get(ilog) );
	if ( !log || log->isEmpty() )
	    continue;

	const Interval<ZType> newdahrg( log->dahRange() );
	if ( !mIsUdf(newdahrg.start) )
	    dahrg.include( newdahrg );
    }

    getMarkerRange( wd, dahrg );
    if ( !todah )
    {
	mDah2TVD( dahrg.start, dahrg.start );
	mDah2TVD( dahrg.stop, dahrg.stop );
    }
    snapZRangeToSurvey( dahrg, false, 0, track );
    return dahrg;
}


void Well::ZRangeSelector::getMarkerRange( const Data& wd,
					Interval<ZType>& zrg ) const
{
    Interval<ZType> newzrg = zrg;

    getLimitPos(wd.markers(),true,newzrg.start,zrg);
    getLimitPos(wd.markers(),false,newzrg.stop,zrg);

    zrg = newzrg;
    if ( zrg.start > zrg.stop )
	std::swap( zrg.start, zrg.stop );
}


void Well::ZRangeSelector::getDahRange( const Well::Data& wd,
						   Interval<ZType>& zrg )
{
    if ( zselection_ == Markers )
    {
	getLimitPos(wd.markers(),true,zrg.start,zrg);
	getLimitPos(wd.markers(),false,zrg.stop,zrg);
    }
    else if ( isInTime() )
    {
	zrg.start = wd.d2TModel().getDah( fixedzrg_.start,  wd.track() );
	zrg.stop = wd.d2TModel().getDah( fixedzrg_.stop,  wd.track() );
    }
    else
	zrg = fixedzrg_;
}


void Well::ZRangeSelector::getLimitPos( const MarkerSet& markers,
				      bool isstart, ZType& val,
				      const Interval<ZType>& zrg ) const
{
    const BufferString& mrknm = isstart ? topmrkr_ : botmrkr_;
    if ( mrknm == ZRangeSelector::sKeyDataStart() )
	val = zrg.start;
    else if ( mrknm == ZRangeSelector::sKeyDataEnd() )
	val = zrg.stop;
    else
    {
	mSetUdf(val);
	val = markers.getDahFromMarkerName( mrknm );
    }
    if ( mIsUdf(val) )
	return;

    const ZType shft = isstart ? (mIsUdf(above_) ? above_ : -above_) : below_;
    if ( !mIsUdf(shft) )
	val += shft;
}



Well::ExtractParams::ExtractParams( const ExtractParams& ep )
{
    *this = ep;
    extractzintime_ = ep.extractzintime_;
    samppol_ = ep.samppol_;
}


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
	 mErrRet( uiStrings::phrEnter(tr("a valid step value")) );

    return ZRangeSelector::isOK( errmsg );
}


Well::ZRangeSelector* Well::ExtractParams::clone() const
{
    return new Well::ExtractParams( *this );
}


void Well::ExtractParams::usePar( const IOPar& pars )
{
    ZRangeSelector::usePar( pars );
    pars.getYN( sKeyZExtractInTime(), extractzintime_ );
    Stats::UpscaleTypeDef().parse( pars.find( sKeySamplePol() ), samppol_ );
}


void Well::ExtractParams::fillPar( IOPar& pars ) const
{
    ZRangeSelector::fillPar( pars );
    pars.setYN( sKeyZExtractInTime(), extractzintime_ );
    pars.set( sKeySamplePol(), Stats::toString( samppol_ ) );
}



Well::TrackSampler::TrackSampler( const DBKeySet& i,
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
	, msg_(tr("Scanning well tracks"))
{
}


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
	return Finished();

    if ( lognms_.isEmpty() )
    {
	msg_ = tr("No well logs specified");
	return ErrorOccurred();
    }

    DataPointSet* dps = new DataPointSet( for2d_, minidps_ );
    dpss_ += dps;
    if ( !mkdahcol_ )
	dahcolnr_ = -1;
    else
    {
	dps->dataSet().add( new DataColDef(sKeyDAHColName()) );
	dahcolnr_ = dps->nrCols() - 1;
    }

    uiRetVal uirv;
    ConstRefMan<Data> wd = MGR().fetch( ids_[curid_],
					LoadReqs(Trck,D2T,Mrkrs).add(Logs),
					uirv);
    if ( !wd )
	{ errmsgs_.add( uirv ); mRetNext() }

    zrg_ = params_.calcFrom( *wd, lognms_ );
    if ( zrg_.isUdf() )
	mRetNext()

    getData( *wd, *dps );
    mRetNext();
}


void Well::TrackSampler::getData( const Data& wd, DataPointSet& dps )
{
    const D2TModel& d2t = wd.d2TModel();
    const Track& track = wd.track();
    MonitorLock mltrack( track );
    MonitorLock mld2t( d2t );
    const bool haved2t = !d2t.isEmpty();
    const bool zrgistime = params_.zselection_ == ZRangeSelector::Times
			&& haved2t;
    const bool extractintime = params_.extractzintime_ && haved2t
			    && SI().zIsTime();

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
    dahrg.start = zrgistime ? d2t.getDah( zrg_.start, track ) : zrg_.start;
    dahrg.stop = zrgistime ? d2t.getDah( zrg_.stop, track ) : zrg_.stop;
    float zpos = dahrg.start;
    if ( extractintime )
	zpos = zrgistime ? zrg_.start : d2t.getTime( zpos, track );

    zpos -= zincr;

    int trackidx = 0; Coord3 precisepos;
    BinIDValue biv;
    BinIDValue prevbiv; mSetUdf(prevbiv.inl());

    dahrg.start -= mLocalEps;
    dahrg.stop  += mLocalEps;
    while ( true )
    {
	zpos += zincr;
	const float dah = extractintime ? d2t.getDah( zpos, track ) : zpos;
	if ( mIsUdf(dah) || !dahrg.includes(dah,true) )
	    return;
	else if ( !getPos(wd,dah,biv,trackidx,precisepos) )
	    continue;

	if ( biv != prevbiv )
	{
	    addPosns( dps, biv, precisepos, dah );
	    prevbiv = biv;
	}
    }
}


bool Well::TrackSampler::getPos( const Data& wd, float dah,
				 BinIDValue& biv, int& trackidx,
				 Coord3& pos ) const
{
    const int tracksz = wd.track().size();
    while ( trackidx < tracksz && dah > wd.track().dahByIdx(trackidx) )
	trackidx++;
    if ( trackidx < 1 || trackidx >= tracksz )
	return false;

    // Position is between trackidx and trackidx-1
    pos = wd.track().coordAfterIdx( dah, trackidx-1 );
    biv.set( SI().transform(pos.getXY()) );
    if ( SI().zIsTime() && !wd.d2TModel().isEmpty() )
    {
	pos.z_ = mCast( double, wd.d2TModel().getTime( dah, wd.track() ) );
	if ( mIsUdf(pos.z_) )
	    return false;
    }

    biv.set( (float)pos.z_ );
    return true;
}


void Well::TrackSampler::addPosns( DataPointSet& dps, const BinIDValue& biv,
				  const Coord3& precisepos, float dah ) const
{
    DataPointSet::DataRow dr;
    if ( dahcolnr_ >= 0 )
	dr.data_ += dah;
#define mAddRow(bv,pos) \
    dr.pos_.setZ( bv.val() ); dr.pos_.set( pos ); dps.addRow( dr )

    mAddRow( biv, precisepos );
    if ( mIsUdf(locradius_) || locradius_ < 1e-3 )
	return;

    const float sqrlocradius = locradius_ * locradius_;

#define mTryAddRow(stmt) \
{ \
    stmt; \
    crd = SI().transform( newbiv ); \
    if ( crd.sqDistTo(precisepos.getXY() ) <= sqrlocradius ) \
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


Well::LogDataExtracter::LogDataExtracter( const DBKeySet& i,
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


void Well::LogDataExtracter::usePar( const IOPar& pars )
{
    pars.get( sKeyLogNm(), lognm_ );
    Stats::UpscaleTypeDef().parse(
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

    if ( dpss_.size() <= curid_ )
	mRetNext()
    DataPointSet& dps = *dpss_[curid_];
    if ( dps.isEmpty() )
	mRetNext()

    uiRetVal uirv;
    ConstRefMan<Data> wd = MGR().fetch( ids_[curid_], LoadReqs(), uirv );
    if ( !wd )
	{ errmsgs_.add( uirv ); mRetNext() }

    const Track* track2use = 0;
    Track* timetrack = 0;
    if ( !zistime_ )
	track2use = &wd->track();
    else
    {
	timetrack = new Track;
	timetrack->toTime( *wd );
	track2use = timetrack;
    }

    getData( dps, *wd, *track2use );
    delete timetrack;

    mRetNext();
}


#define mDefWinSz SI().zIsTime() ? wl.dahStep(true)*20 : SI().zStep()


void Well::LogDataExtracter::getData( DataPointSet& dps,
				      const Well::Data& wd,
				      const Well::Track& track )
{
    DataPointSet::ColID dpscolidx = dps.indexOf( lognm_ );
    if ( dpscolidx < 0 )
    {
	dps.dataSet().add( new DataColDef(lognm_) );
	dpscolidx = dps.nrCols() - 1;
	if ( dpscolidx < 0 ) return;
    }

    const Well::Log* wlptr = wd.logs().getLogByName( lognm_ );
    if ( !wlptr)
	return;
    const Well::Log& wl = *wlptr;

    MonitorLock mltrack( track );
    MonitorLock mllog( wl );

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
    float z1 = track.valueByIdx( trackidx );

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
	if ( track.valueByIdx(trackidx) > dpsz )
	    break;
    }
    if ( trackidx >= track.size() ) // Duh. Entire track below data.
	return;

    float prevwinsz = mDefWinSz;
    float prevdah = mUdf(float);
    for ( ; dpsrowidx<dps.size(); dpsrowidx++ )
    {
	dpsz = dps.z(dpsrowidx);
	float z2 = track.valueByIdx( trackidx );
	while ( dpsz > z2 )
	{
	    trackidx++;
	    if ( trackidx >= track.size() )
		return;
	    z2 = track.valueByIdx( trackidx );
	}
	if ( trackidx == 0 ) // Huh?
	    continue;

	z1 = track.valueByIdx( trackidx - 1 );
	if ( z1 > dpsz )
	{
	    // This is not uncommon. A new binid with higher posns.
	    trackidx = 0;
	    if ( dpsrowidx > 0 ) dpsrowidx--;
	    mSetUdf(prevdah);
	    continue;
	}

	float dah = ( (z2-dpsz) * track.dahByIdx(trackidx-1)
		    + (dpsz-z1) * track.dahByIdx(trackidx) )
		    / (z2 - z1);

	float winsz = mIsUdf(prevdah) ? prevwinsz : dah - prevdah;
	addValAtDah( dah, wl, winsz, dps, dpscolidx, dpsrowidx );
	prevwinsz = winsz;
	prevdah = dah;
    }
}


void Well::LogDataExtracter::getGenTrackData( DataPointSet& dps,
					      const Well::Track& track,
					      const Well::Log& wl,
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


void Well::LogDataExtracter::addValAtDah( float dah, const Well::Log& wl,
					  float winsz, DataPointSet& dps,
					  int dpscolidx, int dpsrowidx ) const
{
    float val = samppol_ == Stats::TakeNearest	?
			wl.valueAt( dah ) : calcVal(wl,dah,winsz,samppol_);
    dps.getValues(dpsrowidx)[dpscolidx] = val;
}


float Well::LogDataExtracter::calcVal( const Well::Log& wl, float dah,
				   float winsz, Stats::UpscaleType samppol,
				   float maxholesz )
{
    if ( samppol == Stats::TakeNearest )
	return wl.valueAt( dah, true );

    const bool logisvel = wl.propType() == PropertyRef::Vel;

    Interval<float> rg( dah-winsz, dah+winsz ); rg.sort();
    TypeSet<float> vals;
    int startidx = wl.indexOf( rg.start );
    if ( startidx < 0 )
	startidx = 0;
    for ( int idx=startidx; idx<wl.size(); idx++ )
    {
	const float curdah = wl.dahByIdx( idx );
	if ( rg.includes(curdah,false) )
	{
	    const float val = wl.valueByIdx( idx );
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

    const bool iscode = wl.valsAreCodes();
    const int sz = vals.size();
    if ( sz < 1 )
    {
	if ( iscode )
	    return wl.valueAt( dah, true );
	if ( mIsUdf(maxholesz) )
	    return mUdf(float);

	const float dahdiffbefore = Math::Abs( dah - wl.dahByIdx(startidx) );
	int stopidx = startidx+1;
	if ( stopidx >= wl.size() )
	    stopidx = wl.size() > 1 ? wl.size()-1 : 0;
	else if ( stopidx < 0 )
	    stopidx = 0;
	const float dahdiffafter = Math::Abs( wl.dahByIdx(stopidx) - dah );
	float val = mUdf(float);
	if ( dahdiffbefore < maxholesz && dahdiffbefore < dahdiffafter )
	    val =  wl.valueByIdx( startidx );
	if ( dahdiffafter < maxholesz && dahdiffafter < dahdiffbefore )
	    val =  wl.valueByIdx( stopidx );
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



Well::SimpleTrackSampler::SimpleTrackSampler( const Well::Track& t,
					    const Well::D2TModel& d2t,
					    bool doextrapolate,
					    bool stayinsidesurvey )
    : Executor("Extracting Well track positions")
    , track_(t)
    , d2t_(d2t.isEmpty()?0:&d2t)
    , isinsidesurvey_(stayinsidesurvey)
    , extrintv_(t.dahRange())
    , extrapolate_(doextrapolate)
    , nrdone_(0)
    , monlock_(t)
{
    if ( track_.isEmpty() )
	return;

    tracklimits_.start = track_.firstDah();
    tracklimits_.stop = track_.lastDah();
    if ( d2t_ )
    {
	tracklimits_.start = d2t_->getTime( tracklimits_.start, track_ );
	tracklimits_.stop = d2t_->getTime( tracklimits_.stop, track_ );
	extrintv_.start = d2t_->getTime( extrintv_.start, track_ );
	extrintv_.stop = d2t_->getTime( extrintv_.stop, track_ );
	extrintv_.step = SI().zStep();
    }
}


int Well::SimpleTrackSampler::nextStep()
{
    float zval = extrintv_.atIndex( nrdone_ );
    float dah = d2t_ ? d2t_->getDah( zval, track_ ) : zval;

    if ( zval > extrintv_.stop )
	return Executor::Finished();

    Coord3 pos = track_.getPos( dah );
    pos.z_ = zval;

    BinID bid = SI().transform( pos.getXY() );
    const bool withintrack = tracklimits_.includes(zval,true);
    if ( withintrack || extrapolate_ )
    {
	if ( extrapolate_ && !withintrack )
	{
	    pos = bidset_.isEmpty() ? track_.posByIdx(0)
				    : track_.posByIdx(track_.size()-1);
	    pos.z_ = zval;
	    bid = SI().transform( pos.getXY() );
	}
	if ( ( isinsidesurvey_ && !SI().includes( bid, zval ) )
		|| !SI().isReasonable( bid ))
	    { nrdone_++; return Executor::MoreToDo(); }

	bidset_ += bid;
	coords_ += pos.getXY();
    }

    nrdone_++;
    return Executor::MoreToDo();
}



Well::LogSampler::LogSampler( const Well::Data& wd,
			    const Well::ExtractParams& pars,
			    const BufferStringSet& lognms )
    : track_( wd.track() )
{
    init( &wd.d2TModel(), pars.calcFrom(wd,lognms,false), pars.isInTime(),
	    pars.zstep_, pars.extractzintime_, pars.samppol_ );
    for ( int idx=0; idx<lognms.size(); idx++ )
    {
	const Well::Log* log = wd.logs().getLogByName( lognms.get(idx) );
	if ( log )
	    logset_ += log;
    }
}


Well::LogSampler::LogSampler( const Well::Data& wd,
			    const Interval<float>& zrg, bool zrgisintime,
			    float zstep, bool extrintime,
			    Stats::UpscaleType samppol,
			    const BufferStringSet& lognms )
    : track_( wd.track() )
{
    init( &wd.d2TModel(), zrg, zrgisintime, zstep, extrintime, samppol);
    for ( int idx=0; idx<lognms.size(); idx++ )
    {
	const Well::Log* log = wd.logs().getLogByName( lognms.get(idx) );
	if ( log )
	    logset_ += log;
    }
}


Well::LogSampler::LogSampler( const Well::D2TModel* d2t,
				const Well::Track* track,
				const Interval<float>& zrg, bool zrgisintime,
				float zstep, bool extrintime,
				Stats::UpscaleType samppol,
				const ObjectSet<const Well::Log>& logs )
    : track_( *track )
{
    init( d2t, zrg, zrgisintime, zstep, extrintime, samppol );
    logset_ = logs;
}


void Well::LogSampler::init( const Well::D2TModel* d2t,
			const Interval<float>& zrg, bool zrgisintime,
			float zstep, bool extrintime,
			Stats::UpscaleType samppol )
{
    d2t_ = d2t;
    data_ = 0;
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


od_int64 Well::LogSampler::nrIterations() const
{ return logset_.size(); }


#define mGetDah(dah,zvalue,zintime) \
	dah = zintime \
	    ? d2t_->getDah( zvalue, track_ ) \
	    : track_.getDahForTVD( zvalue );


#define mGetZ(zvalue,dah,zintime) \
	zvalue = zintime \
	       ? mCast( float, track_.getPos( dah ).z_ ) \
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
    const Well::Log* log = logset_.validIdx( logidx ) ? logset_[logidx] : 0;
    if ( !log || log->isEmpty() ) return false;

    const int winszidx = data_->getSize(0)-1;
    const bool nearestinterp = samppol_ == Stats::TakeNearest;
    for ( int idz=0; idz<data_->getSize(1); idz++ )
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
    return data_ && ( idz < data_->getSize(1) ) ?
				  data_->get( 0, idz ) : mUdf( float );
}


float Well::LogSampler::getThickness( int idz ) const
{
    const int winszidx = data_->getSize(0) - 1;
    return data_ && ( idz < data_->getSize(1) ) ?
				data_->get( winszidx, idz ) : mUdf( float );
}


float Well::LogSampler::getLogVal( int logidx, int idz ) const
{
    const int xsz = data_->getSize(0);
    const int zsz = data_->getSize(1);

    return data_ && logidx+1 < xsz && idz < zsz ?
	data_->get( logidx + 1, idz ) : mUdf( float );
}


float Well::LogSampler::getLogVal( const char* lnm, int idz ) const
{
    bool found = false;
    int logidx = 0;
    for ( logidx=0; logidx<logset_.size(); logidx++ )
    {
	if ( logset_[logidx]->hasName(lnm) )
	    { found = true; break; }
    }
    return found ? getLogVal( logidx, idz ) :  mUdf(float);
}


int Well::LogSampler::nrZSamples() const
{ return data_ ? data_->getSize(1) : 0; }
