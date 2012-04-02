/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : May 2004
-*/

static const char* rcsID = "$Id: wellextractdata.cc,v 1.72 2012-04-02 13:04:33 cvsbruno Exp $";

#include "wellextractdata.h"
#include "wellreader.h"
#include "wellmarker.h"
#include "welltrack.h"
#include "welld2tmodel.h"
#include "welllogset.h"
#include "welllog.h"
#include "welldata.h"
#include "welltransl.h"
#include "arrayndimpl.h"
#include "datapointset.h"
#include "posvecdataset.h"
#include "survinfo.h"
#include "iodirentry.h"
#include "ctxtioobj.h"
#include "datacoldef.h"
#include "filepath.h"
#include "strmprov.h"
#include "ioobj.h"
#include "ioman.h"
#include "iopar.h"
#include "ptrman.h"
#include "multiid.h"
#include "sorting.h"
#include "envvars.h"
#include "errh.h"

#include <iostream>
#include <math.h>

namespace Well
{
const char* ExtractParams::sKeyTopMrk()	    { return "Top marker"; }
const char* ExtractParams::sKeyBotMrk()	    { return "Bottom marker"; }
const char* ExtractParams::sKeyLimits()	    { return "Extraction extension"; }
const char* ExtractParams::sKeyDataStart()  { return "<Start of data>"; }
const char* ExtractParams::sKeyDataEnd()    { return "<End of data>"; }
const char* ExtractParams::sKeyZRange()	    { return "Z range"; }
const char* ExtractParams::sKeyZExtractInTime() { return "Extract Z in time"; }
const char* ExtractParams::sKeySamplePol()  { return "Data sampling"; }
const char* ExtractParams::sKeyZSelection() { return "Z Selection"; }
const char* TrackSampler::sKeyLogNm()	    { return "Log name"; }
const char* TrackSampler::sKeySelRadius()   { return "Selection radius"; }
const char* TrackSampler::sKeyFor2D()	    { return "For 2D"; }
const char* TrackSampler::sKeyDahCol()	    { return "Create MD column"; }
const char* LogDataExtracter::sKeyLogNm()   { return
    					      Well::TrackSampler::sKeyLogNm(); }

DefineEnumNames(ExtractParams,ZSelection,0,"Type of selection")
{ "Markers", "Depth range", "Times range", 0 };

}

static const char* sKeyDAHColName()	    { return "<MD>"; }


Well::InfoCollector::InfoCollector( bool dologs, bool domarkers )
    : Executor("Well information extraction")
    , domrkrs_(domarkers)
    , dologs_(dologs)
    , curidx_(0)
{
    PtrMan<CtxtIOObj> ctio = mMkCtxtIOObj(Well);
    IOM().to( ctio->ctxt.getSelKey() );
    direntries_ = new IODirEntryList( IOM().dirPtr(), ctio->ctxt );
    totalnr_ = direntries_->size();
    curmsg_ = totalnr_ ? "Gathering information" : "No wells";
}


Well::InfoCollector::~InfoCollector()
{
    deepErase( infos_ );
    deepErase( markers_ );
    deepErase( logs_ );
}


int Well::InfoCollector::nextStep()
{
    if ( curidx_ >= totalnr_ )
	return ErrorOccurred();

    IOObj* ioobj = (*direntries_)[curidx_]->ioobj;
    Well::Data wd;
    Well::Reader wr( ioobj->fullUserExpr(true), wd );
    if ( wr.getInfo() )
    {
	ids_ += new MultiID( ioobj->key() );
	infos_ += new Well::Info( wd.info() );
	if ( dologs_ )
	{
	    BufferStringSet* newlognms = new BufferStringSet;
	    wr.getLogInfo( *newlognms );
	    logs_ += newlognms;


	    if ( mIsUdf(logsdahrg_.start) )
		logsdahrg_ = wr.getAllLogsDahRange();
	    else 
	    {
		StepInterval<float> dahrg = wr.getAllLogsDahRange();
		if ( !mIsUdf( dahrg.start ) )
		    logsdahrg_.include( dahrg );
	    }
	}
	if ( domrkrs_ )
	{
	    Well::MarkerSet* newset = new Well::MarkerSet;
	    markers_ += newset;
	    if ( wr.getMarkers() )
		deepCopy( *newset, wd.markers() );
	}
    }

    return ++curidx_ >= totalnr_ ? Finished() : MoreToDo();
}


#define mErrRet(msg) { if ( errmsg ) *errmsg = msg; return false; }
bool Well::ExtractParams::isOK( BufferString* errmsg ) const
{
    const bool usemrkr = zselection_ == Markers;
    if ( ( usemrkr && topmrkr_.isEqual(botmrkr_) 
		&& mIsEqual(above_,below_,SI().zStep()) ) 
		    || ( !usemrkr && zrg_.width() < SI().zStep()) )
    {
	mErrRet( "Top distance is equal to bottom distance" );
    }
    if ( !mIsUdf( zrg_.step ) && zrg_.step < 0 )
	 mErrRet( "Please Enter a valid step value" );
    
    return true;
}



Well::ExtractParams::ExtractParams( const Well::ExtractParams& p )
{
    topmrkr_ = p.topmrkr_;
    botmrkr_ = p.botmrkr_;
    above_ = p.above_;
    below_ = p.below_;
    extractzintime_ = p.extractzintime_;
    zselection_ = p.zselection_;
    zrg_ = p.zrg_;
    samppol_ = p.samppol_;
}


void Well::ExtractParams::setEmpty()
{
    topmrkr_ = sKeyDataStart();
    botmrkr_ = sKeyDataEnd();
    above_ = below_ = 0;
    extractzintime_ = SI().zIsTime();
    zselection_ = Markers;
    zrg_ = StepInterval<float>( mUdf(float), mUdf(float), mUdf(float) );
    samppol_ = Stats::TakeNearest;
}


void Well::ExtractParams::usePar( const IOPar& pars )
{
    pars.get( sKeyTopMrk(), topmrkr_ );
    pars.get( sKeyBotMrk(), botmrkr_ );
    pars.get( sKeyLimits(), above_, below_ );
    pars.getYN( sKeyZExtractInTime(), extractzintime_ );
    pars.get( sKeyZRange(), zrg_ );
    parseEnumUpscaleType( pars.find( sKeySamplePol() ), samppol_ );
    parseEnumZSelection( pars.find( sKeyZSelection() ), zselection_ );
}


void Well::ExtractParams::fillPar( IOPar& pars ) const
{
    pars.set( sKeyTopMrk(), topmrkr_ );
    pars.set( sKeyBotMrk(), botmrkr_ );
    pars.set( sKeyLimits(), above_, below_ );
    pars.setYN( sKeyZExtractInTime(), extractzintime_ );
    pars.set( sKeyZRange(), zrg_ );
    pars.set( sKeySamplePol(), getUpscaleTypeString( samppol_ ) );
    pars.set( sKeyZSelection(), getZSelectionString( zselection_ ) );
}



#define mGetTrackRg(rg)\
    rg.start = wd.track().dah(0); rg.stop = wd.track().dah(wd.track().size()-1);
StepInterval<float> Well::ExtractParams::calcFrom( const IOObj& ioobj, 
					const BufferStringSet& lognms ) const
{
    if ( zselection_ != Markers )
	return zrg_; 

    StepInterval<float> dahrg( mUdf(float), mUdf(float), zrg_.step );

    Well::Data wd;
    Well::Reader wr( ioobj.fullUserExpr(true), wd );
    if ( !wr.getInfo() ) return dahrg;
    wr.getTrack(); 
    wr.getD2T(); 
    wr.getMarkers();

    if ( lognms.isEmpty() )
	{ mGetTrackRg( dahrg ); }

    int ilog = 0;

    for ( ; mIsUdf(dahrg.start) && ilog<lognms.size(); ilog++ )
	dahrg = wr.getLogDahRange( lognms.get(ilog) );

    for ( ; ilog<lognms.size(); ilog++ )
    {
	Interval<float> newdahrg = wr.getLogDahRange( lognms.get(ilog) );
	if ( mIsUdf(newdahrg.start) ) continue;
	dahrg.include( newdahrg );
    }

    getMarkerRange( wd, dahrg );

    return dahrg;
}


StepInterval<float> Well::ExtractParams::calcFrom( const Well::Data& wd,
					const BufferStringSet& lognms ) const
{
    if ( zselection_ != Markers )
	return zrg_; 

    StepInterval<float> dahrg( mUdf(float), mUdf(float), zrg_.step );
    int ilog = 0;

    if ( lognms.isEmpty() )
	{ mGetTrackRg( dahrg ); }

    for ( ; mIsUdf(dahrg.start) && ilog<lognms.size(); ilog++ )
    {
	const Well::Log* log = wd.logs().getLog( lognms.get( ilog ) );
	if ( !log || log->isEmpty() )  continue;

	dahrg.set( log->dah(0), log->dah(log->size()-1), dahrg.step );
    }
    for ( ; ilog<lognms.size(); ilog++ )
    {
	const Well::Log* log = wd.logs().getLog( lognms.get( ilog ) );
	if ( !log || log->isEmpty() )  continue;

	Interval<float> newdahrg(log->dah(0),log->dah(log->size()-1));
	if ( mIsUdf(newdahrg.start) ) continue;
	dahrg.include( newdahrg );
    }

    getMarkerRange( wd, dahrg );

    return dahrg;
}


void Well::ExtractParams::getMarkerRange( const Well::Data& wd,
					Interval<float>& zrg ) const
{
    Interval<float> newzrg = zrg;

    getLimitPos(wd.markers(),wd.d2TModel(),true,newzrg.start,zrg);
    getLimitPos(wd.markers(),wd.d2TModel(),false,newzrg.stop,zrg);

    zrg = newzrg;
    if ( zrg.start > zrg.stop ) Swap( zrg.start, zrg.stop );
}


void Well::ExtractParams::getLimitPos( const MarkerSet& markers,
				      const Well::D2TModel* d2t,
				      bool isstart, float& val,
				      const Interval<float>& zrg ) const
{
    const BufferString& mrknm = isstart ? topmrkr_ : botmrkr_;
    if ( mrknm == Well::ExtractParams::sKeyDataStart() )
	val = zrg.start;
    else if ( mrknm == Well::ExtractParams::sKeyDataEnd() )
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





Well::TrackSampler::TrackSampler( const BufferStringSet& i,
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
    delete ioobj; \
    curid_++; \
    return curid_ >= ids_.size() ? closeDPSS(dpss_) : MoreToDo(); }

int Well::TrackSampler::nextStep()
{
    if ( curid_ >= ids_.size() )
	return 0;
    if ( lognms_.isEmpty() )
	{ "No well logs specified"; return Executor::ErrorOccurred(); }

    DataPointSet* dps = new DataPointSet( for2d_, minidps_ );
    dpss_ += dps;
    if ( !mkdahcol_ )
	dahcolnr_ = -1;
    else
    {
	dps->dataSet().add( new DataColDef(sKeyDAHColName()) );
	dahcolnr_ = dps->nrCols() - 1;
    }

    IOObj* ioobj = IOM().get( MultiID(ids_.get(curid_)) );
    if ( !ioobj ) mRetNext()
    Well::Data wd;
    Well::Reader wr( ioobj->fullUserExpr(true), wd );
    if ( !wr.getInfo() ) mRetNext()
    if ( zistime_ && !wr.getD2T() )
	mRetNext()

    welldahrg_.start = mUdf(float);
    welldahrg_ = params_.calcFrom( *ioobj, lognms_ );

    if ( mIsUdf(welldahrg_.start) || mIsUdf(welldahrg_.stop ) ) 
	mRetNext()

    getData( wd, *dps );
    mRetNext();
}


void Well::TrackSampler::getData( const Well::Data& wd, DataPointSet& dps )
{
    float dahincr = SI().zStep() * .5;
    if ( SI().zIsTime() )
	dahincr = 2000 * dahincr; //As dx = v * dt, Using v = 2000 m/s

    BinIDValue biv; float dah = welldahrg_.start; 
    float time = mUdf(float);
    const Well::D2TModel* d2t = wd.d2TModel();
    const bool extractintime = params_.extractzintime_ && d2t && SI().zIsTime();
    float timeincr = SI().zStep();
    if ( extractintime )
    {
	time = d2t->getTime( dah ) - timeincr; 
	if ( !mIsUdf(params_.zrg_.step) )	
	    timeincr = params_.zrg_.step; 
    }
    else
	dah -= dahincr;
    int trackidx = 0; Coord3 precisepos;
    BinIDValue prevbiv; mSetUdf(prevbiv.binid.inl);

    while ( true )
    {
	if ( extractintime )
	{
	    time += timeincr;
	    dah = d2t->getDah( time ); 
	}
	else
	    dah += dahincr;

	if ( mIsUdf(dah) || !welldahrg_.includes( dah, true ) )
	    return;
	else if ( !getSnapPos(wd,dah,biv,trackidx,precisepos) )
	    continue;

	if ( biv.binid != prevbiv.binid
	  || !mIsEqual(biv.value,prevbiv.value,mDefEps) )
	{
	    addPosns( dps, biv, precisepos, dah );
	    prevbiv = biv;
	}
    }
}


bool Well::TrackSampler::getSnapPos( const Well::Data& wd, float dah,
				     BinIDValue& biv, int& trackidx,
				     Coord3& pos ) const
{
    const int tracksz = wd.track().size();
    while ( trackidx < tracksz && dah > wd.track().dah(trackidx) )
	trackidx++;
    if ( trackidx < 1 || trackidx >= tracksz )
	return false;

    // Position is between trackidx and trackidx-1
    pos = wd.track().coordAfterIdx( dah, trackidx-1 );
    biv.binid = SI().transform( pos );
    if ( SI().zIsTime() && wd.d2TModel() )
    {
	pos.z = wd.d2TModel()->getTime( dah );
	if ( mIsUdf(pos.z) )
	    return false;
    }
    const int nearidx = SI().zRange(false).nearestIndex( pos.z );
    biv.value = SI().zRange(false).atIndex( nearidx );
    return true;
}


void Well::TrackSampler::addPosns( DataPointSet& dps, const BinIDValue& biv,
				  const Coord3& precisepos, float dah ) const
{
    DataPointSet::DataRow dr;
    if ( dahcolnr_ >= 0 )
	dr.data_ += dah;
#define mAddRow(bv,pos) \
    dr.pos_.z_ = bv.value; dr.pos_.set( pos ); dps.addRow( dr )

    mAddRow( biv, precisepos );
    if ( mIsUdf(locradius_) || locradius_ < 1e-3 )
	return;

    const float sqrlocradius = locradius_ * locradius_;

#define mTryAddRow(stmt) \
{ \
    stmt; \
    crd = SI().transform( newbiv.binid ); \
    if ( crd.sqDistTo(precisepos) <= sqrlocradius ) \
	{ mAddRow(biv,crd); nradded++; } \
}

    BinIDValue newbiv( biv ); Coord crd;

    for ( int idist=1; ; idist++ )
    {
	int nradded = 0;

	newbiv.binid.crl = biv.binid.crl - idist;
	for ( int iinl=-idist; iinl<=idist; iinl++ )
	    mTryAddRow(newbiv.binid.inl = biv.binid.inl + iinl)
	newbiv.binid.crl = biv.binid.crl + idist;
	for ( int iinl=-idist; iinl<=idist; iinl++ )
	    mTryAddRow(newbiv.binid.inl = biv.binid.inl + iinl)
	newbiv.binid.inl = biv.binid.inl + idist;
	for ( int icrl=1-idist; icrl<idist; icrl++ )
	    mTryAddRow(newbiv.binid.crl = biv.binid.crl + icrl)
	newbiv.binid.inl = biv.binid.inl - idist;
	for ( int icrl=1-idist; icrl<idist; icrl++ )
	    mTryAddRow(newbiv.binid.crl = biv.binid.crl + icrl)

	if ( nradded == 0 ) break;
    }
}


Well::LogDataExtracter::LogDataExtracter( const BufferStringSet& i,
					  ObjectSet<DataPointSet>& d,
       					  bool ztm )
	: Executor("Well log data extraction")
	, ids_(i)
	, dpss_(d)
    	, samppol_(Stats::UseMed)
	, curid_(0)
	, zistime_(ztm)
{
}


void Well::LogDataExtracter::usePar( const IOPar& pars )
{
    pars.get( sKeyLogNm(), lognm_ );
    parseEnumUpscaleType( 
	    pars.find( Well::ExtractParams::sKeySamplePol() ), samppol_ );
}


#undef mRetNext
#define mRetNext() { \
    delete ioobj; \
    curid_++; \
    return curid_ >= ids_.size() ? Finished() : MoreToDo(); }

int Well::LogDataExtracter::nextStep()
{
    if ( curid_ >= ids_.size() )
	return 0;
    if ( msg_.isEmpty() )
	{ msg_ = "Extracting '"; msg_ += lognm_; msg_ += "'"; return MoreToDo(); }

    IOObj* ioobj = 0;
    if ( dpss_.size() <= curid_ ) mRetNext()
    DataPointSet& dps = *dpss_[curid_];
    if ( dps.isEmpty() ) mRetNext()

    ioobj = IOM().get( MultiID(ids_.get(curid_)) );
    if ( !ioobj ) mRetNext()
    Well::Data wd;
    Well::Reader wr( ioobj->fullUserExpr(true), wd );
    if ( !wr.getInfo() ) mRetNext()

    PtrMan<Well::Track> timetrack = 0;
    if ( zistime_ )
    {
	if ( !wr.getD2T() ) mRetNext()
	timetrack = new Well::Track( wd.track() );
	timetrack->toTime( *wd.d2TModel() );
    }
    const Well::Track& track = zistime_ ? *timetrack : wd.track();
    if ( track.size() < 2 ) mRetNext()
    if ( !wr.getLogs() ) mRetNext()
    
    getData( dps, wd, track );
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

    int wlidx = wd.logs().indexOf( lognm_ );
    if ( wlidx < 0 )
	return;
    const Well::Log& wl = wd.logs().getLog( wlidx );

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
    float z1 = track.pos(trackidx).z;

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
	if ( track.pos(trackidx).z > dpsz )
	    break;
    }
    if ( trackidx >= track.size() ) // Duh. Entire track below data.
	return;

    float prevwinsz = mDefWinSz;
    float prevdah = mUdf(float);
    for ( ; dpsrowidx<dps.size(); dpsrowidx++ )
    {
	dpsz = dps.z(dpsrowidx);
	float z2 = track.pos( trackidx ).z;
	while ( dpsz > z2 )
	{
	    trackidx++;
	    if ( trackidx >= track.size() )
		return;
	    z2 = track.pos( trackidx ).z;
	}
	if ( trackidx == 0 ) // Huh?
	    continue;

	z1 = track.pos( trackidx - 1 ).z;
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
					      const Well::Track& track,
					      const Well::Log& wl,
					      int dpscolidx, int dahcolidx )
{
    if ( dps.isEmpty() || track.isEmpty() )
	return;

    const float dahstep = wl.dahStep(true);
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
			wl.getValue( dah ) : calcVal(wl,dah,winsz,samppol_);
    dps.getValues(dpsrowidx)[dpscolidx] = val;
}


float Well::LogDataExtracter::calcVal( const Well::Log& wl, float dah,
				   float winsz, Stats::UpscaleType samppol ) 
{
    Interval<float> rg( dah-winsz, dah+winsz ); rg.sort();
    TypeSet<float> vals;
    for ( int idx=0; idx<wl.size(); idx++ )
    {
	float newdah = wl.dah( idx );
	if ( rg.includes(newdah,false) )
	{
	    float val = wl.value(idx);
	    if ( !mIsUdf(val) )
		vals += wl.value(idx);
	}
	else if ( newdah > rg.stop )
	    break;
    }
    if ( vals.size() < 1 ) return mUdf(float);
    if ( vals.size() == 1 ) return vals[0];
    if ( vals.size() == 2 ) return samppol == Stats::UseAvg
				? (vals[0]+vals[1])/2 : vals[0];
    const int sz = vals.size();
    if ( samppol == Stats::UseMed )
    {
	sort_array( vals.arr(), sz );
	return vals[sz/2];
    }
    else if ( samppol == Stats::UseAvg )
    {
	float val = 0;
	for ( int idx=0; idx<sz; idx++ )
	    val += vals[idx];
	return val / sz;
    }
    else if ( samppol == Stats::UseRMS )
    {
	float val = 0;
	for ( int idx=0; idx<sz; idx++ )
	    val += vals[idx] * vals[idx];
	return sqrt( val / sz );
    }
    else if ( samppol == Stats::UseMostFreq )
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
	return valsseen[ maxvsidx ];
    }

    //pErrMsg( "UpscaleType not supported" );
    return vals[0];
}



Well::SimpleTrackSampler::SimpleTrackSampler( const Well::Track& t, 
					    const Well::D2TModel* d2t,
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

    if ( mIsUdf( extrintv_.step ) )
	extrintv_.step = SI().zStep();

    float zstop = track_.dah( track_.size()-1 );
    float zstart = track_.dah( 0 );
    if ( d2t )
    {
	zstart = d2t->getTime( zstart );
	zstop = d2t->getTime( zstop );
	extrintv_.start = d2t->getTime( extrintv_.start );
	extrintv_.stop = d2t->getTime( extrintv_.stop );
    }
    tracklimits_.start = zstart;
    tracklimits_.stop = zstop; 
}


int Well::SimpleTrackSampler::nextStep()
{
    float zval = extrintv_.atIndex( nrdone_ );
    float dah = d2t_ ? d2t_->getDah(zval) : zval;

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
	if ( isinsidesurvey_ && !SI().includes( bid, zval, false ) ) 
	    { nrdone_++; return Executor::MoreToDo(); }

	bidset_ += bid; coords_ += pos;
    }

    nrdone_ ++;
    return Executor::MoreToDo();
}


#define mGetRange()\
{\
    ExtractRangeComputer erc; erc.set( pars_, &lognms_ );\
    zrg_ = erc.dahRangeFrom( wd_ );\
}

Well::LogSampler::LogSampler( const Well::Data& wd, 
			    const Well::ExtractParams& pars, 
			    const BufferStringSet& lognms )
    : wd_(wd)
    , data_(0)
    , samppol_(pars.samppol_)
    , zrg_(pars.calcFrom(wd,lognms))
    , extrintime_(pars.extractzintime_)
    , lognms_(lognms)
{
}


Well::LogSampler::LogSampler( const Well::Data& wd, 
			    const StepInterval<float>& zrg, bool extrintime,
			    Stats::UpscaleType samppol,
			    const BufferStringSet& lognms )
    : wd_(wd)
    , data_(0)
    , samppol_(samppol)
    , lognms_(lognms)
    , zrg_(zrg) 
    , extrintime_(extrintime)
{
}


Well::LogSampler::~LogSampler()
{
    delete data_;
}


od_int64 Well::LogSampler::nrIterations() const
{ return lognms_.size(); }


bool Well::LogSampler::doPrepare( int thread )
{
    if ( !nrIterations() )
	{ errmsg_ = "No log found"; return false; } 

    if ( mIsUdf( zrg_.step ) )
    {
	zrg_.step = SI().zStep()*0.5;
	if ( SI().zIsTime() )
	    zrg_.step = 2000 * zrg_.step;
    }
    if ( mIsUdf(zrg_.start) || mIsUdf(zrg_.stop) || !zrg_.nrSteps() )
	{errmsg_ ="No valid range specified"; return false;}


    data_ = new Array2DImpl<float>( nrIterations()+1, zrg_.nrSteps()+1 );

    const Well::D2TModel* d2t = wd_.d2TModel();
    for ( int idz=0; idz<zrg_.nrSteps()+1; idz++ ) 
    {
	float z = zrg_.atIndex( idz );
	z = d2t && extrintime_ ? d2t->getDah(z) : wd_.track().getDahForTVD(z);
	data_->set( 0, idz, z );
    }

    return true;
}


bool Well::LogSampler::doWork( od_int64 start, od_int64 stop, int nrthreads )
{
    for ( int idx=start; idx<=stop; idx++ )
    {
	if ( !shouldContinue() )
	    return false;

	if ( !doLog( idx ) )
	    { errmsg_ = "One or several logs could not be written"; }

	addToNrDone( 1 );
    }
    return true;
}


#define mWinSz extrintime_ ? log->dahStep(true)*20 : SI().zStep()
bool Well::LogSampler::doLog( int logidx )
{
    const Well::Log* log = wd_.logs().getLog( lognms_.get( logidx ) );
    if ( !log || log->isEmpty() ) return false;

    for ( int idz=0; idz<zrg_.nrSteps()+1; idz++ ) 
    {
	const float dah = data_->get( 0, idz );
	const float winsz = mWinSz;
	const float lval = samppol_ == Stats::TakeNearest ? 
	    	  log->getValue(dah,true) 
		: LogDataExtracter::calcVal(*log,dah,winsz,samppol_);
	data_->set( logidx+1, idz, lval );
    }

    return true;
}


float Well::LogSampler::getDah( int idz ) const
{
    return data_ ? data_->get( 0, idz ) : mUdf( float );
}


float Well::LogSampler::getDah( float zpos ) const
{
    const int idz = zrg_.getIndex( zpos );
    return getDah( idz );
}


float Well::LogSampler::getLogVal( int logidx, int idz ) const
{
    const int xsz = data_->info().getSize(0);
    const int zsz = data_->info().getSize(1);

    return data_ && logidx+1 < xsz && idz < zsz ? 
	data_->get( logidx + 1, idz ) : mUdf( float );
}


float Well::LogSampler::getLogVal( const char* lnm, int idz ) const
{
    const int logidx = lognms_.indexOf( lnm );
    return logidx < 0 ? mUdf(float) : getLogVal( logidx, idz );
}


float Well::LogSampler::getLogVal( int logidx, float zpos ) const
{
    const int idz = zrg_.getIndex( zpos );
    return getLogVal( logidx, idz );
}
