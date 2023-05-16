/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "welldbupdater.h"


#include "iodir.h"
#include "iodirentry.h"
#include "welldata.h"
#include "wellman.h"
#include "welltransl.h"
#include "wellwriter.h"

namespace Well
{

DatabaseUpdater::DatabaseUpdater()
    : Executor("Updating Well Database")
{
    IOObjContext ctxt = mIOObjContext(Well);
    ctxt.toselect_.allowtransls_.add( "dGB" );
    const IODir iodir( ctxt.getSelKey() );
    const IODirEntryList list( iodir, ctxt );
    for ( int idx=0; idx<list.size(); idx++ )
    {
	const IOObj* ioobj = list.get(idx)->ioobj_;
	if ( ioobj )
	    wellids_ += ioobj->key();
    }

    totalnr_ = wellids_.size();
}


DatabaseUpdater::~DatabaseUpdater()
{}


uiString DatabaseUpdater::uiMessage() const
{
    return messages_.cat();
}


uiString DatabaseUpdater::uiNrDoneText() const
{
    return tr("Wells done");
}


int DatabaseUpdater::nextStep()
{
    if ( !wellids_.validIdx(nrdone_) )
	return Finished();

    const MultiID wellid = wellids_[nrdone_];
    RefMan<Data> wdin = MGR().get( wellid );
    if ( wdin )
    {
	const Writer wrr( wellid, *wdin );
	const bool res = wrr.putInfoAndTrack() && wrr.putLogs() &&
			 wrr.putD2T() && wrr.putCSMdl();
	if ( !res )
	    messages_.add( wrr.errMsg() );
    }

    nrdone_++;

    return MoreToDo();
}

} // namespace Well
