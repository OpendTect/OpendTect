/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : May 2004
-*/

static const char* rcsID = "$Id: wellextractdata.cc,v 1.34 2008-02-26 09:17:36 cvsnanne Exp $";

#include "wellextractdata.h"
#include "wellreader.h"
#include "wellmarker.h"
#include "welltrack.h"
#include "welld2tmodel.h"
#include "welllogset.h"
#include "welllog.h"
#include "welldata.h"
#include "welltransl.h"
#include "binidvalset.h"
#include "survinfo.h"
#include "iodirentry.h"
#include "ctxtioobj.h"
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


DefineEnumNames(Well::TrackSampler,SelPol,1,Well::TrackSampler::sKeySelPol)
	{ "Nearest trace only", "All corners", 0 };
const char* Well::TrackSampler::sKeyTopMrk = "Top marker";
const char* Well::TrackSampler::sKeyBotMrk = "Bottom marker";
const char* Well::TrackSampler::sKeyLimits = "Extraction extension";
const char* Well::TrackSampler::sKeySelPol = "Trace selection";
const char* Well::TrackSampler::sKeyDataStart = "<Start of data>";
const char* Well::TrackSampler::sKeyDataEnd = "<End of data>";
const char* Well::TrackSampler::sKeyLogNm = "Log name";
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
				  ObjectSet<BinIDValueSet>& b )
	: Executor("Well data extraction")
	, above(0)
    	, below(0)
    	, selpol(Corners)
    	, ids(i)
    	, bivsets(b)
    	, curidx(0)
    	, timesurv(SI().zIsTime())
{
}


void Well::TrackSampler::usePar( const IOPar& pars )
{
    pars.get( sKeyTopMrk, topmrkr );
    pars.get( sKeyBotMrk, botmrkr );
    pars.get( sKeyLogNm, lognm );
    pars.get( sKeyLimits, above, below );
    const char* res = pars.find( sKeySelPol );
    if ( res && *res ) selpol = eEnum(SelPol,res);
}


#define mRetNext() { \
    delete ioobj; \
    curidx++; \
    return curidx >= ids.size() ? Finished : MoreToDo; }

int Well::TrackSampler::nextStep()
{
    if ( curidx >= ids.size() )
	return 0;

    BinIDValueSet* bivset = new BinIDValueSet(1,true);
    bivsets += bivset;

    IOObj* ioobj = IOM().get( MultiID(ids.get(curidx)) );
    if ( !ioobj ) mRetNext()
    Well::Data wd;
    Well::Reader wr( ioobj->fullUserExpr(true), wd );
    if ( !wr.getInfo() ) mRetNext()
    if ( timesurv && !wr.getD2T() ) mRetNext()
    fulldahrg = wr.getLogDahRange( lognm );
    if ( mIsUdf(fulldahrg.start) ) mRetNext()
    wr.getMarkers();

    getData( wd, *bivset );
    mRetNext();
}


void Well::TrackSampler::getData( const Well::Data& wd, BinIDValueSet& bivset )
{
    Interval<float> dahrg;
    getLimitPos(wd.markers(),true,dahrg.start);
    	if ( mIsUdf(dahrg.start) ) return;
    getLimitPos(wd.markers(),false,dahrg.stop);
    	if ( mIsUdf(dahrg.stop) ) return;
    if ( dahrg.start > dahrg.stop  ) return;

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
	    addBivs( bivset, biv, precisepos );
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


void Well::TrackSampler::addBivs( BinIDValueSet& bivset, const BinIDValue& biv,
				  const Coord3& precisepos ) const
{
    bivset.add( biv );
    if ( selpol == Corners )
    {
	BinID stp( SI().inlStep(), SI().crlStep() );
	BinID bid( biv.binid.inl+stp.inl, biv.binid.crl+stp.crl );
	Coord crd = SI().transform( bid );
	double dist = crd.distTo( precisepos );
	BinID nearest = bid; double lodist = dist;

#define mTestNext(op1,op2) \
	bid = BinID( biv.binid.inl op1 stp.inl, biv.binid.crl op2 stp.crl ); \
	crd = SI().transform( bid ); \
	dist = crd.distTo( precisepos ); \
	if ( dist < lodist ) \
	    { lodist = dist; nearest = bid; }

	mTestNext(+,-)
	mTestNext(-,+)
	mTestNext(-,-)

	BinIDValue newbiv( biv );
	newbiv.binid.inl = nearest.inl; bivset.add( newbiv );
	newbiv.binid.crl = nearest.crl; bivset.add( newbiv );
	newbiv.binid.inl = biv.binid.inl; bivset.add( newbiv );
    }
}


Well::LogDataExtracter::LogDataExtracter( const BufferStringSet& i,
					  const ObjectSet<BinIDValueSet>& b )
	: Executor("Well log data extraction")
	, ids(i)
	, bivsets(b)
    	, samppol(Med)
	, curidx(0)
    	, timesurv(SI().zIsTime())
{
}


void Well::LogDataExtracter::usePar( const IOPar& pars )
{
    pars.get( sKeyLogNm, lognm );
    const char* res = pars.find( sKeySamplePol );
    if ( res && *res ) samppol = eEnum(SamplePol,res);
}


#define mRetNext() { \
    delete ioobj; \
    curidx++; \
    return curidx >= ids.size() ? Finished : MoreToDo; }

int Well::LogDataExtracter::nextStep()
{
    if ( curidx >= ids.size() )
	return 0;

    TypeSet<float>* newres = new TypeSet<float>;
    ress += newres;

    IOObj* ioobj = 0;
    const BinIDValueSet& bivs = *bivsets[curidx];
    if ( bivs.isEmpty() ) mRetNext()

    ioobj = IOM().get( MultiID(ids.get(curidx)) );
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
    
    getData( bivs, wd, track, *newres );

    mRetNext();
}


#define mDefWinSz SI().zIsTime() ? wl.dahStep(true)*20 : SI().zStep()


void Well::LogDataExtracter::getData( const BinIDValueSet& bivs,
				      const Well::Data& wd,
				      const Well::Track& track,
				      TypeSet<float>& res ) const
{
    // The order in 'res' is important. We can stop early, but we cannot
    // leave out undefs.

    int wlidx = wd.logs().indexOf( lognm );
    if ( wlidx < 0 )
	return;
    const Well::Log& wl = wd.logs().getLog( wlidx );

    if ( GetEnvVar("OD_WELL_EXTRACTION_DATA_DUMP") )
    {
	StreamProvider sp( GetEnvVar("OD_WELL_EXTRACTION_DATA_DUMP") );
	StreamData sd = sp.makeOStream();
	if ( sd.usable() )
	{
	std::ostream& strm = *sd.ostrm;
	float last_dah = track.dah( track.size()-1 );
	const float dah_step = wl.dahStep(true);
	for ( float d_ah=track.dah(0); d_ah<=last_dah; d_ah += dah_step )
	{
	    Coord3 tpos = track.getPos( d_ah );
	    Coord3 zpos = wd.track().getPos( d_ah );
	    float val = wl.getValue( d_ah );
	    BinID bid = SI().transform( zpos );
	    Coord bidcoord = SI().transform( bid );
	    Coord offs( zpos.x - bidcoord.x, zpos.y - bidcoord.y );
	    strm << d_ah << '\t' << val << '\t'
		 << bid.inl << '\t' << bid.crl << '\t'
		 << offs.x << '\t' << offs.y << '\t'
		 << zpos.z << '\t' << tpos.z << '\n';
	}
	}
	sd.close();
    }

    bool usegenalgo = !track.alwaysDownward();
    int opt = GetEnvVarIVal("DTECT_LOG_EXTR_ALGO",0);
    if ( opt == 1 ) usegenalgo = false;
    if ( opt == 2 ) usegenalgo = true;

    if ( usegenalgo )
    {
	// Much slower, less precise but should always work
	getGenTrackData( bivs, track, wl, res );
	return;
    }

    // The idea here is to calculate the dah from the Z only.
    // Should be OK for all wells without horizontal sections

    int trackidx = 0;
    const float tol = 0.00001;
    float z1 = track.pos(trackidx).z;
    BinIDValueSet::Pos bvpos;
    BinIDValue biv;
    while ( bivs.next(bvpos) )
    {
	bivs.get( bvpos, biv );
	if ( biv.value < z1 - tol )
	    res += mUdf(float);
	else
	    break;
    }
    if ( !bvpos.valid() ) // Duh. All data below track.
	{ res.erase(); return; }

    for ( trackidx=0; trackidx<track.size(); trackidx++ )
    {
	if ( track.pos(trackidx).z > biv.value - tol )
	    break;
    }
    if ( trackidx >= track.size() ) // Duh. Entire track below data.
	return;

    float prevwinsz = mDefWinSz;
    float prevdah = mUdf(float);
    bivs.prev( bvpos );
    while ( bivs.next(bvpos) )
    {
	bivs.get( bvpos, biv );
	float z2 = track.pos( trackidx ).z;
	while ( biv.value > z2 )
	{
	    trackidx++;
	    if ( trackidx >= track.size() )
		return;
	    z2 = track.pos( trackidx ).z;
	}
	if ( trackidx == 0 )
	    continue;

	z1 = track.pos( trackidx - 1 ).z;
	if ( z1 > biv.value )
	{
	    // This is not uncommon. A new binid with higher posns.
	    trackidx = 0;
	    bivs.prev( bvpos );
	    mSetUdf(prevdah);
	    continue;
	}

	float dah = ( (z2-biv.value) * track.dah(trackidx-1)
		    + (biv.value-z1) * track.dah(trackidx) )
		    / (z2 - z1);

	float winsz = mIsUdf(prevdah) ? prevwinsz : dah - prevdah;
	addValAtDah( dah, wl, res, winsz );
	prevwinsz = winsz;
	prevdah = dah;
    }
}


void Well::LogDataExtracter::getGenTrackData( const BinIDValueSet& bivs,
					      const Well::Track& track,
					      const Well::Log& wl,
					      TypeSet<float>& res ) const
{
    BinIDValueSet::Pos bvpos;
    BinIDValue biv;
    while ( bivs.next(bvpos) )
    {
	bivs.get( bvpos, biv );
	if ( !mIsUdf(biv.value) )
	    break;
	res += mUdf(float);
    }

    if ( !bvpos.valid() || track.isEmpty() )
	{ res.erase(); return; }

    BinID b( biv.binid.inl+SI().inlStep(),  biv.binid.crl+SI().crlStep() );
    const float dtol = SI().transform(biv.binid).distTo( SI().transform(b) );
    const float ztol = SI().zStep() * 5;
    const float startdah = track.dah(0);
    const float dahstep = wl.dahStep(true);

    float prevdah = mUdf(float);
    float prevwinsz = mDefWinSz;
    bivs.prev( bvpos );
    while ( bivs.next(bvpos) )
    {
	bivs.get( bvpos, biv );
	float dah = findNearest( track, biv, startdah, dahstep );
	if ( mIsUdf(dah) )
	    { res += mUdf(float); continue; }
	else
	{
	    Coord3 pos = track.getPos( dah );
	    Coord coord = SI().transform( biv.binid );
	    if ( coord.distTo(pos) > dtol || fabs(pos.z-biv.value) > ztol )
		res += mUdf(float);
	    else
	    {
		float winsz = mIsUdf(prevdah) ? prevwinsz : dah - prevdah;
		addValAtDah( dah, wl, res, winsz );
		prevwinsz = winsz;
		prevdah = dah;
	    }
	}
    }
}

float Well::LogDataExtracter::findNearest( const Well::Track& track,
		    const BinIDValue& biv, float startdah, float dahstep ) const
{
    if ( mIsUdf(dahstep) || mIsZero(dahstep,mDefEps) )
	return mUdf(float);

    const float zfac = SI().zIsTime() ? 10000 : 1;
		// Use a distance criterion weighing the Z a lot
    float dah = startdah;
    const float lastdah = track.dah( track.size() - 1 );
    Coord3 tpos = track.getPos(dah); tpos.z *= zfac;
    Coord coord = SI().transform( biv.binid );
    Coord3 bivpos( coord.x, coord.y, biv.value * zfac );
    float mindist = tpos.distTo( bivpos );
    float mindah = dah;
    for ( dah = dah + dahstep; dah <= lastdah; dah += dahstep )
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
					  TypeSet<float>& res,
					  float winsz ) const
{
    float val = samppol == Nearest ? wl.getValue( dah )
				   : calcVal(wl,dah,winsz);
    res += val;
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
