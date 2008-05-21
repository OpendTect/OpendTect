/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : May 2004
-*/

static const char* rcsID = "$Id: wellextractdata.cc,v 1.43 2008-05-21 14:14:22 cvsbert Exp $";

#include "wellextractdata.h"
#include "wellreader.h"
#include "wellmarker.h"
#include "welltrack.h"
#include "welld2tmodel.h"
#include "welllogset.h"
#include "welllog.h"
#include "welldata.h"
#include "welltransl.h"
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


const char* Well::TrackSampler::sKeyTopMrk = "Top marker";
const char* Well::TrackSampler::sKeyBotMrk = "Bottom marker";
const char* Well::TrackSampler::sKeyLimits = "Extraction extension";
const char* Well::TrackSampler::sKeySelRadius = "Selection radius";
const char* Well::TrackSampler::sKeyDataStart = "<Start of data>";
const char* Well::TrackSampler::sKeyDataEnd = "<End of data>";
const char* Well::TrackSampler::sKeyLogNm = "Log name";
const char* Well::TrackSampler::sKeyFor2D = "For 2D";
const char* Well::LogDataExtracter::sKeyLogNm = Well::TrackSampler::sKeyLogNm;

DefineEnumNames(Well::LogDataExtracter,SamplePol,2,
		Well::LogDataExtracter::sKeySamplePol)
	{ "Average", "Median", "Most frequent", "Nearest sample", 0 };
const char* Well::LogDataExtracter::sKeySamplePol = "Data sampling";


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
	return ErrorOccurred;

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
	}
	if ( domrkrs_ )
	{
	    ObjectSet<Well::Marker>* newset = new ObjectSet<Well::Marker>;
	    markers_ += newset;
	    if ( wr.getMarkers() )
		deepCopy( *newset, wd.markers() );
	}
    }

    return ++curidx_ >= totalnr_ ? Finished : MoreToDo;
}


Well::TrackSampler::TrackSampler( const BufferStringSet& i,
				  ObjectSet<DataPointSet>& d )
	: Executor("Well data extraction")
	, topmrkr(sKeyDataStart)
	, botmrkr(sKeyDataEnd)
	, above(0)
    	, below(0)
    	, locradius(0)
    	, ids(i)
    	, dpss(d)
    	, curid(0)
    	, timesurv(SI().zIsTime())
    	, for2d(false)
    	, minidps(false)
{
}


void Well::TrackSampler::usePar( const IOPar& pars )
{
    pars.get( sKeyTopMrk, topmrkr );
    pars.get( sKeyBotMrk, botmrkr );
    pars.get( sKeyLogNm, lognms );
    pars.get( sKeyLimits, above, below );
    pars.get( sKeySelRadius, locradius );
    pars.getYN( sKeyFor2D, for2d );
}


static int closeDPSS( ObjectSet<DataPointSet>& dpss )
{
    for ( int idx=0; idx<dpss.size(); idx++ )
	dpss[idx]->dataChanged();
    return Executor::Finished;
}

#define mRetNext() { \
    delete ioobj; \
    curid++; \
    return curid >= ids.size() ? closeDPSS(dpss) : MoreToDo; }

int Well::TrackSampler::nextStep()
{
    if ( curid >= ids.size() )
	return 0;
    if ( lognms.isEmpty() )
	{ "No well logs specified"; return Executor::ErrorOccurred; }

    DataPointSet* dps = new DataPointSet( for2d, minidps );
    dpss += dps;

    IOObj* ioobj = IOM().get( MultiID(ids.get(curid)) );
    if ( !ioobj ) mRetNext()
    Well::Data wd;
    Well::Reader wr( ioobj->fullUserExpr(true), wd );
    if ( !wr.getInfo() ) mRetNext()
    if ( timesurv && !wr.getD2T() )
	mRetNext()

    fulldahrg.start = mUdf(float);
    int ilog = 0;
    for ( ; mIsUdf(fulldahrg.start) && ilog<lognms.size(); ilog++ )
	fulldahrg = wr.getLogDahRange( lognms.get(ilog) );
    for ( ; ilog<lognms.size(); ilog++ )
    {
	Interval<float> newdahrg = wr.getLogDahRange( lognms.get(ilog) );
	if ( mIsUdf(newdahrg.start) ) continue;
	fulldahrg.include( newdahrg );
    }
    if ( mIsUdf(fulldahrg.start) ) mRetNext()

    wr.getMarkers();

    getData( wd, *dps );
    mRetNext();
}


void Well::TrackSampler::getData( const Well::Data& wd, DataPointSet& dps )
{
    Interval<float> dahrg;
    getLimitPos(wd.markers(),true,dahrg.start);
    	if ( mIsUdf(dahrg.start) ) return;
    getLimitPos(wd.markers(),false,dahrg.stop);
    	if ( mIsUdf(dahrg.stop) ) return;
    if ( dahrg.start > dahrg.stop ) Swap( dahrg.start, dahrg.stop );

    float dahincr = SI().zStep() * .5;
    if ( SI().zIsTime() )
	dahincr = 1000 * dahincr; // As dx = v * dt , Using v = 1000 m/s

    BinIDValue biv; float dah = dahrg.start - dahincr;
    int trackidx = 0; Coord3 precisepos;
    BinIDValue prevbiv; mSetUdf(prevbiv.binid.inl);

    while ( true )
    {
	dah += dahincr;
	if ( dah > dahrg.stop )
	    return;
	else if ( !getSnapPos(wd,dah,biv,trackidx,precisepos) )
	    continue;

	if ( biv.binid != prevbiv.binid
	  || !mIsEqual(biv.value,prevbiv.value,mDefEps) )
	{
	    addPosns( dps, biv, precisepos );
	    prevbiv = biv;
	}
    }
}


void Well::TrackSampler::getLimitPos( const ObjectSet<Marker>& markers,
				      bool isstart, float& val ) const
{
    const BufferString& mrknm = isstart ? topmrkr : botmrkr;
    if ( mrknm == sKeyDataStart )
	val = fulldahrg.start;
    else if ( mrknm == sKeyDataEnd )
	val = fulldahrg.stop;
    else
    {
	mSetUdf(val);
	for ( int idx=0; idx<markers.size(); idx++ )
	{
	    if ( markers[idx]->name() == mrknm )
	    {
		val = markers[idx]->dah_;
		break;
	    }
	}
    }

    float shft = isstart ? (mIsUdf(above) ? above : -above) : below;
    if ( mIsUdf(val) )
	return;

    if ( !mIsUdf(shft) )
	val += shft;
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
    biv.value = pos.z; SI().snapZ( biv.value );
    return true;
}


void Well::TrackSampler::addPosns( DataPointSet& dps, const BinIDValue& biv,
				  const Coord3& precisepos ) const
{
    DataPointSet::DataRow dr;
#define mAddRow(bv,pos) \
    dr.pos_.z_ = bv.value; dr.pos_.set( bv.binid, pos ); dps.addRow( dr )

    mAddRow( biv, precisepos );
    if ( mIsUdf(locradius) || locradius < 1e-3 )
	return;

    const float sqrlocradius = locradius * locradius;
    const BinID stp( SI().inlStep(), SI().crlStep() );

#define mTryAddRow(stmt) \
{ \
    stmt; \
    crd = SI().transform( newbiv.binid ); \
    if ( crd.sqDistTo(precisepos) <= sqrlocradius ) \
	{ mAddRow(newbiv,crd); nradded++; } \
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
					  ObjectSet<DataPointSet>& d )
	: Executor("Well log data extraction")
	, ids(i)
	, dpss(d)
    	, samppol(Med)
	, curid(0)
    	, timesurv(SI().zIsTime())
{
}


void Well::LogDataExtracter::usePar( const IOPar& pars )
{
    pars.get( sKeyLogNm, lognm );
    const char* res = pars.find( sKeySamplePol );
    if ( res && *res ) samppol = eEnum(SamplePol,res);
}


#undef mRetNext
#define mRetNext() { \
    delete ioobj; \
    curid++; \
    return curid >= ids.size() ? Finished : MoreToDo; }

int Well::LogDataExtracter::nextStep()
{
    if ( curid >= ids.size() )
	return 0;
    if ( msg_.isEmpty() )
	{ msg_ = "Extracting '"; msg_ += lognm; msg_ += "'"; return MoreToDo; }

    IOObj* ioobj = 0;
    if ( dpss.size() <= curid ) mRetNext()
    DataPointSet& dps = *dpss[curid];
    if ( dps.isEmpty() ) mRetNext()

    ioobj = IOM().get( MultiID(ids.get(curid)) );
    if ( !ioobj ) mRetNext()
    Well::Data wd;
    Well::Reader wr( ioobj->fullUserExpr(true), wd );
    if ( !wr.getInfo() ) mRetNext()

    PtrMan<Well::Track> timetrack = 0;
    if ( timesurv )
    {
	if ( !wr.getD2T() ) mRetNext()
	timetrack = new Well::Track( wd.track() );
	timetrack->toTime( *wd.d2TModel() );
    }
    const Well::Track& track = timesurv ? *timetrack : wd.track();
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
    int wlidx = wd.logs().indexOf( lognm );
    if ( wlidx < 0 )
	return;

    const Well::Log& wl = wd.logs().getLog( wlidx );
    DataPointSet::ColID dpscolidx = dps.indexOf( lognm );
    if ( dpscolidx < 0 )
    {
	dps.dataSet().add( new DataColDef(lognm) );
	dpscolidx = dps.nrCols() - 1;
	if ( dpscolidx < 0 ) return;
    }

    bool usegenalgo = !track.alwaysDownward();
    int opt = GetEnvVarIVal("DTECT_LOG_EXTR_ALGO",0);
    if ( opt == 1 ) usegenalgo = false;
    if ( opt == 2 ) usegenalgo = true;

    if ( usegenalgo )
    {
	// Much slower, less precise but should always work
	getGenTrackData( dps, track, wl, dpscolidx );
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
					      int dpscolidx )
{
    if ( dps.isEmpty() || track.isEmpty() )
	return;

    const float startdah = track.dah(0);
    const float dahstep = wl.dahStep(true);
    float prevdah = mUdf(float);
    float prevwinsz = mDefWinSz;
    DataPointSet::RowID dpsrowidx = 0;
    BinIDValue biv;
    for ( ; dpsrowidx<dps.size(); dpsrowidx++ )
    {
	biv.binid = dps.binID( dpsrowidx );
	biv.value = dps.z( dpsrowidx );
	float dah = findNearest( track, biv, startdah, dahstep, prevdah );
	if ( mIsUdf(dah) )
	    continue;

	Coord3 pos = track.getPos( dah );
	Coord coord = dps.coord( dpsrowidx );

	const float winsz = mIsUdf(prevdah) ? prevwinsz : dah - prevdah;
	addValAtDah( dah, wl, winsz, dps, dpscolidx, dpsrowidx );
	prevwinsz = winsz;
	prevdah = dah;
    }
}

float Well::LogDataExtracter::findNearest( const Well::Track& track,
		    const BinIDValue& biv, float startdah, float dahstep,
       		    float prevdah ) const
{
    if ( mIsUdf(dahstep) || mIsZero(dahstep,mDefEps) )
	return mUdf(float);

    const float zfac = SI().zIsTime() ? 10 : 1;
		// Use a distance criterion weighing the Z more
    float dah = startdah;
    Coord3 tpos = track.getPos(dah); tpos.z *= zfac;
    Coord coord = SI().transform( biv.binid );
    TypeSet<float> dists; TypeSet<int> idxs;
    const Coord3 bivpos( coord.x, coord.y, biv.value * zfac );
    if ( !mIsUdf(prevdah) )
    {
	// Try to find near previous
	for ( int idx=-4; idx<5; idx++ )
	{
	    dah = prevdah + idx * dahstep;
	    if ( dah < track.dah(0) || dah > track.dah(track.size()-1) )
		continue;
	    tpos = track.getPos(dah); tpos.z *= zfac;
	    dists += tpos.distTo( bivpos );
	    idxs += idx;
	}

	if ( !dists.isEmpty() )
	{
	    float mindist = dists[0]; int mindistidx = 0;
	    for ( int idx=0; idx<dists.size(); idx++ )
	    {
		if ( dists[idx] < mindist )
		    { mindist = dists[idx]; mindistidx = idx; }
	    }
	    if ( mindistidx > 0 && mindistidx < dists.size() - 1 )
		return prevdah + idxs[mindistidx] * dahstep;
	}
    }

    // Do extensive search
    const float lastdah = track.dah( track.size() - 1 );
    float mindist = tpos.distTo( bivpos );
    float mindah = dah;
    for ( dah = startdah; dah <= lastdah; dah += dahstep )
    {
	tpos = track.getPos(dah); tpos.z *= zfac;
	float dist = tpos.distTo( bivpos );
	if ( dist < mindist )
	{
	    mindist = dist;
	    mindah = dah;
	}
    }
    return mindah;
}


void Well::LogDataExtracter::addValAtDah( float dah, const Well::Log& wl,
					  float winsz, DataPointSet& dps,
       					  int dpscolidx, int dpsrowidx ) const
{
    float val = samppol == Nearest ? wl.getValue( dah )
				   : calcVal(wl,dah,winsz);
    dps.getValues(dpsrowidx)[dpscolidx] = val;
}


float Well::LogDataExtracter::calcVal( const Well::Log& wl, float dah,
				       float winsz ) const
{
    Interval<float> rg( dah-winsz, dah+winsz ); rg.sort();
    TypeSet<float> vals;
    for ( int idx=0; idx<wl.size(); idx++ )
    {
	float newdah = wl.dah( idx );
	if ( rg.includes(newdah) )
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
    if ( vals.size() == 2 ) return samppol == Avg ? (vals[0]+vals[1])/2
						  : vals[0];
    const int sz = vals.size();
    if ( samppol == Med )
    {
	sort_array( vals.arr(), sz );
	return vals[sz/2];
    }
    else if ( samppol == Avg )
    {
	float val = 0;
	for ( int idx=0; idx<sz; idx++ )
	    val += vals[idx];
	return val / sz;
    }
    else if ( samppol == MostFreq )
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

    pErrMsg( "SamplePol not supported" );
    return vals[0];
}
