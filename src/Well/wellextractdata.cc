/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : May 2004
-*/

static const char* rcsID = "$Id: wellextractdata.cc,v 1.1 2004-05-04 09:14:47 bert Exp $";

#include "wellinfocollecter.h"
#include "wellreader.h"
#include "wellmarker.h"
#include "welldata.h"
#include "welltransl.h"
#include "iodirentry.h"
#include "multiid.h"
#include "ctxtioobj.h"
#include "ioobj.h"
#include "ioman.h"
#include "ptrman.h"


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
