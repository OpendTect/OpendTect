/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : May 2004
-*/

static const char* rcsID = "$Id: wellextractdata.cc,v 1.2 2004-05-05 20:54:28 bert Exp $";

#include "wellextractdata.h"
#include "wellreader.h"
#include "wellmarker.h"
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

DefineEnumNames(Well::TrackSampler,HorPol,2,Well::TrackSampler::sKeyHorSamplPol)
	{ "Median", "Average", "Most frequent", "Nearest sample", 0 };
DefineEnumNames(Well::TrackSampler,VerPol,1,Well::TrackSampler::sKeyVerSamplPol)
	{ "All corners", "Nearest trace only", "Average corners", 0 };

const char* Well::TrackSampler::sKeyTopMrk = "Top marker";
const char* Well::TrackSampler::sKeyBotMrk = "Bottom marker";
const char* Well::TrackSampler::sKeyLimits = "Extraction extension";
const char* Well::TrackSampler::sKeyHorSamplPol = "Horizontal sampling";
const char* Well::TrackSampler::sKeyVerSamplPol = "Vertical sampling";
const char* Well::TrackSampler::sKeyDataStart = "<Start of data>";
const char* Well::TrackSampler::sKeyDataEnd = "<End of data>";


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
    	, horpol(Med)
    	, verpol(Corners)
    	, ids(i)
    	, bivsets(b)
    	, curidx(0)
{
}


void Well::TrackSampler::usePar( const IOPar& pars )
{
    pars.get( sKeyTopMrk, topmrkr );
    pars.get( sKeyBotMrk, botmrkr );
    pars.get( sKeyLimits, above, below );
    const char* res = pars.find( sKeyHorSamplPol );
    if ( res && *res ) horpol = eEnum(HorPol,res);
    res = pars.find( sKeyVerSamplPol );
    if ( res && *res ) verpol = eEnum(VerPol,res);
}


#define mRetNext() { \
    delete ioobj; \
    curidx++; \
    return curidx >= ids.size() ? 0 : 1; }

int Well::TrackSampler::nextStep()
{
    if ( curidx >= ids.size() )
	return 0;

    IOObj* ioobj = IOM().get( MultiID(ids.get(curidx)) );
    if ( !ioobj ) mRetNext()
    Well::Data wd;
    Well::Reader wr( ioobj->fullUserExpr(true), wd );
    if ( !wr.getInfo() ) mRetNext()
    if ( SI().zIsTime() && !wr.getD2T() ) mRetNext()

    //TODO use wd track + d2t model to sample track

    mRetNext();
}
