/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : May 2004
-*/

static const char* rcsID = "$Id: wellextractdata.cc,v 1.5 2004-05-06 21:03:52 bert Exp $";

#include "wellextractdata.h"
#include "wellreader.h"
#include "wellmarker.h"
#include "welltrack.h"
#include "welld2tmodel.h"
#include "welldata.h"
#include "welltransl.h"
#include "survinfo.h"
#include "iodirentry.h"
#include "ctxtioobj.h"
#include "ioobj.h"
#include "ioman.h"
#include "iopar.h"
#include "ptrman.h"
#include "multiid.h"

DefineEnumNames(Well::TrackSampler,SelPol,1,Well::TrackSampler::sKeySelPol)
	{ "Nearest trace only", "All corners", 0 };
const char* Well::TrackSampler::sKeyTopMrk = "Top marker";
const char* Well::TrackSampler::sKeyBotMrk = "Bottom marker";
const char* Well::TrackSampler::sKeyLimits = "Extraction extension";
const char* Well::TrackSampler::sKeySelPol = "Trace selection";
const char* Well::TrackSampler::sKeyDataStart = "<Start of data>";
const char* Well::TrackSampler::sKeyDataEnd = "<End of data>";
const char* Well::TrackSampler::sKeyLogNm = "Log name";

DefineEnumNames(Well::LogDataExtracter,SamplePol,2,
		Well::LogDataExtracter::sKeySamplePol)
	{ "Median", "Average", "Most frequent", "Nearest sample", 0 };
const char* Well::LogDataExtracter::sKeySamplePol = "Data sampling";


Well::InfoCollector::InfoCollector( bool dologs, bool domarkers )
    : Executor("Well information extraction")
    , domrkrs_(domarkers)
    , dologs_(dologs_)
    , curidx_(0)
{
    PtrMan<CtxtIOObj> ctio = mMkCtxtIOObj(Well);
    IOM().to( ctio->ctxt.stdSelKey() );
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

    BinIDValueSet* bivset = new BinIDValueSet;
    bivsets += bivset;

    IOObj* ioobj = IOM().get( MultiID(ids.get(curidx)) );
    if ( !ioobj ) mRetNext()
    Well::Data wd;
    Well::Reader wr( ioobj->fullUserExpr(true), wd );
    if ( !wr.getInfo() ) mRetNext()
    if ( timesurv && !wr.getD2T() ) mRetNext()
    fulldahrg = wr.getLogZRange( lognm );
    if ( mIsUndefined(fulldahrg.start) ) mRetNext()
    wr.getMarkers();

    getData( wd, *bivset );
    mRetNext();
}


void Well::TrackSampler::getData( const Well::Data& wd, BinIDValueSet& bivset )
{
    Interval<float> dahrg;
    getLimitPos(wd.markers(),true,dahrg.start);
    	if ( mIsUndefined(dahrg.start) ) return;
    getLimitPos(wd.markers(),false,dahrg.stop);
    	if ( mIsUndefined(dahrg.stop) ) return;
    if ( dahrg.start > dahrg.stop  ) return;

    float dahincr = SI().zRange().step * .5;
    if ( SI().zIsTime() )
	dahincr = 1000 * dahincr; // As dx = v * dt , Using v = 1000 m/s

    BinIDValue biv; float dah = dahrg.start;
    int trackidx = 0; Coord3 precisepos;
    if ( !getSnapPos(wd,dah,biv,trackidx,precisepos) )
	return;

    addBivs( bivset, biv, precisepos );
    BinIDValue prevbiv = biv;

    while ( true )
    {
	dah += dahincr;
	if ( dah > dahrg.stop || !getSnapPos(wd,dah,biv,trackidx,precisepos) )
	    return;

	if ( biv.binid != prevbiv.binid || !mIS_ZERO(biv.value-prevbiv.value) )
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
	val = mUndefValue;
	for ( int idx=0; idx<markers.size(); idx++ )
	{
	    if ( markers[idx]->name() == mrknm )
	    {
		val = markers[idx]->dah;
		break;
	    }
	}
    }

    float shft = isstart ? (mIsUndefined(above) ? above : -above) : below;
    if ( mIsUndefined(val) )
	return;

    if ( !mIsUndefined(shft) )
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
	if ( mIsUndefined(pos.z) )
	    return false;
    }
    biv.value = SI().zRange().snap( pos.z );
    return true;
}


void Well::TrackSampler::addBivs( BinIDValueSet& bivset, const BinIDValue& biv,
				  const Coord3& precisepos ) const
{
    bivset += biv;
    if ( selpol == Corners )
    {
	BinID stp( SI().inlStep(), SI().crlStep() );
	BinID bid( biv.binid.inl+stp.inl, biv.binid.crl+stp.crl );
	Coord crd = SI().transform( bid );
	double dist = crd.distance( precisepos );
	BinID nearest = bid; double lodist = dist;

#define mTestNext(op1,op2) \
	bid = BinID( biv.binid.inl op1 stp.inl, biv.binid.crl op2 stp.crl ); \
	crd = SI().transform( bid ); \
	dist = crd.distance( precisepos ); \
	if ( dist < lodist ) \
	    { lodist = dist; nearest = bid; }

	mTestNext(+,-)
	mTestNext(-,+)
	mTestNext(-,-)

	BinIDValue newbiv( biv );
	newbiv.binid.inl = nearest.inl; bivset += newbiv;
	newbiv.binid.crl = nearest.crl; bivset += newbiv;
	newbiv.binid.inl = biv.binid.inl; bivset += newbiv;
    }
}


Well::LogDataExtracter::LogDataExtracter( const BufferStringSet& i )
	: Executor("Well log data extraction")
	, ids(i)
    	, samppol(Med)
	, curidx(0)
    	, timesurv(SI().zIsTime())
{
}


void Well::LogDataExtracter::usePar( const IOPar& pars )
{
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

    IOObj* ioobj = IOM().get( MultiID(ids.get(curidx)) );
    if ( !ioobj ) mRetNext()
    Well::Data wd;
    Well::Reader wr( ioobj->fullUserExpr(true), wd );
    if ( !wr.getInfo() ) mRetNext()
    if ( timesurv && !wr.getD2T() ) mRetNext()


    mRetNext();
}
