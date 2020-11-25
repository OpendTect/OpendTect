/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Aug 2003
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "wellman.h"

#include "iodir.h"
#include "iodirentry.h"
#include "ioman.h"
#include "ptrman.h"
#include "survinfo.h"
#include "welldata.h"
#include "welllog.h"
#include "welllogset.h"
#include "wellmarker.h"
#include "wellreader.h"
#include "wellwriter.h"
#include "welltrack.h"
#include "welltransl.h"


Well::LoadReqs::LoadReqs( bool addall )
{
    if ( addall )
        setToAll();
}


Well::LoadReqs::LoadReqs( SubObjType typ )
{
    add( typ );
}


Well::LoadReqs::LoadReqs( SubObjType typ1, SubObjType typ2 )
{
    add( typ1 ).add( typ2 );
}


Well::LoadReqs::LoadReqs( SubObjType typ1, SubObjType typ2, SubObjType typ3 )
{
    add( typ1 ).add( typ2 ).add( typ3 );
}


Well::LoadReqs& Well::LoadReqs::add( SubObjType typ )
{
    if ( typ != D2T || SI().zIsTime() )
        reqs_[typ] = 1;
    if ( typ == Trck )
        reqs_[Inf] = 1;
    if ( typ == Logs )
	reqs_[LogInfos] = 0;
    return *this;
}


Well::LoadReqs Well::LoadReqs::All()
{
    LoadReqs ret( false );
    ret.reqs_.set();
    if ( !SI().zIsTime() )
        ret.reqs_[D2T] = 0;

    ret.reqs_[LogInfos] = 0;
    return ret;
}


void Well::LoadReqs::include( const LoadReqs& oth )
{
    for ( int idx=0; idx<mWellNrSubObjTypes; idx++ )
    {
        if ( oth.reqs_[idx] )
            reqs_[ idx ] = 1;
    }

    if ( reqs_[Logs] )
	reqs_[LogInfos] = 0;
}


bool Well::LoadReqs::includes( const LoadReqs& oth ) const
{
    for ( int idx=0; idx<mWellNrSubObjTypes; idx++ )
        if ( oth.reqs_[idx] && !reqs_[idx] )
            return false;
    return true;
}


Well::Man* Well::Man::mgr_ = 0;

Well::Man& Well::MGR()
{
    if ( !::Well::Man::mgr_ )
	::Well::Man::mgr_ = new ::Well::Man;
    return *::Well::Man::mgr_;
}


Well::Man::~Man()
{
    cleanup();
}


void Well::Man::cleanup()
{
    deepUnRef( wells_ );
}


void Well::Man::removeObject( const Well::Data* wd )
{
    const int idx = wells_.indexOf( wd );
    if ( idx < 0 ) return;

    wells_.removeSingle( idx );
    loadstates_.removeSingle( idx );
}


void Well::Man::add( const MultiID& key, Well::Data* wll )
{
    if ( !wll ) return;

    wll->ref();
    wll->setMultiID( key );
    wells_ += wll;
    LoadReqs reqs( Well::Inf );
    if ( !wll->track().isEmpty() )
	reqs.add( Well::Trck );
    if ( wll->d2TModel() && !wll->d2TModel()->isEmpty() )
	reqs.add( Well::D2T );
    if ( wll->checkShotModel() && !wll->checkShotModel()->isEmpty() )
	reqs.add( Well::CSMdl );
    if ( !wll->markers().isEmpty() )
	reqs.add( Well::Mrkrs );
    if ( !wll->logs().isEmpty() )
	reqs.add( Well::Logs );

    loadstates_ += reqs;
}


Well::Data* Well::Man::release( const MultiID& key )
{
    const int idx = gtByKey( key );
    if ( idx < 0 ) return 0;

    loadstates_.removeSingle( idx );
    Well::Data* ret = wells_.removeSingle( idx );
    ret->unRef();
    return ret;
}


Well::Data* Well::Man::get( const MultiID& key )
{
    return get( key, Well::LoadReqs() );
}


Well::Data* Well::Man::get( const MultiID& key, Well::LoadReqs reqs )
{
    msg_.setEmpty();

    const int wdidx = gtByKey( key );
    Well::Data* wd = wdidx < 0 ? nullptr : wells_[wdidx];
    if ( wd && loadstates_[wdidx].includes(reqs) )
        return wd;

    if ( wdidx >=0 )
    {
	reqs.include( loadstates_[wdidx] );
	if ( !readReqData(key,wd,reqs) )
	    return nullptr;

	loadstates_[wdidx] = reqs;
	return wd;
    }

    wd = new Well::Data;
    wd->ref();
    if ( !readReqData(key,wd,reqs) )
    {
	wd->unRef();
	return nullptr;
    }

    wd->setMultiID( key );
    wells_ += wd;
    loadstates_ += reqs;
    return wd;
}


bool Well::Man::readReqData( const MultiID& key, Well::Data* wd, LoadReqs reqs )
{
    if ( !wd )
	return false;

    wd->ref();
    Reader rdr( key, *wd );
    if ( reqs.includes(Inf) && !rdr.getInfo() )
    { msg_ = rdr.errMsg(); wd->unRef(); return false; }

    if ( reqs.includes(Trck) && !rdr.getTrack() )
    { msg_ = rdr.errMsg(); wd->unRef(); return false; }

    if ( reqs.includes(D2T) )
	rdr.getD2T();
    if ( reqs.includes(Mrkrs) )
	rdr.getMarkers();
    if ( reqs.includes(Logs) )
	rdr.getLogs();
    if ( reqs.includes(LogInfos) )
	rdr.getLogs( true );
    if ( reqs.includes(CSMdl) )
	rdr.getCSMdl();
    if ( reqs.includes(DispProps2D) || reqs.includes(DispProps3D) )
	rdr.getDispProps();

    wd->unRef();

    return true;
}


bool Well::Man::isLoaded( const MultiID& key ) const
{
    return gtByKey( key ) >= 0;
}


bool Well::Man::reload( const MultiID& key )
{
    const int wdidx = gtByKey( key );
    if ( wdidx<0 ) return false;

    Well::Data* wd = wells_[wdidx];
    wd->ref();
    readReqData( key, wd, loadstates_[wdidx] );
    wd->reloaded.trigger();
    wd->unRef();
    return true;
}


int Well::Man::gtByKey( const MultiID& key ) const
{
    for ( int idx=0; idx<wells_.size(); idx++ )
    {
	if ( wells_[idx]->multiID() == key )
	    return idx;
    }
    return -1;
}


bool Well::Man::getWellKeys( TypeSet<MultiID>& ids, bool onlyloaded )
{
    if ( !onlyloaded )
    {
	const IOObjContext ctxt = mIOObjContext(Well);
	const IODir iodir( ctxt.getSelKey() );
	const IODirEntryList list( iodir, ctxt );
	for ( int idx=0; idx<list.size(); idx++ )
	{
	    const IOObj* ioobj = list.get(idx)->ioobj_;
	    if ( ioobj )
		ids += ioobj->key();
	}
    }
    else
    {
	ObjectSet<Data>& wells = MGR().wells();
	for ( int idx=0; idx<wells.size(); idx++ )
	{
	    if ( wells[idx] )
		ids += wells[idx]->multiID();
	}
    }

    return !ids.isEmpty();
}


bool Well::Man::getWellNames( BufferStringSet& wellnms, bool onlyloaded )
{
    TypeSet<MultiID> ids;
    getWellKeys( ids );
    for ( int idx=0; idx<ids.size(); idx++ )
	wellnms.add( IOM().nameOf(ids[idx]) );

    return !wellnms.isEmpty();
}


bool Well::Man::getLogNames( const MultiID& ky, BufferStringSet& nms,
			     bool forceLoad )
{
    nms.setEmpty();
    if ( MGR().isLoaded(ky) && forceLoad )
    {
	RefMan<Well::Data> wd = MGR().get( ky, Well::LoadReqs(Well::LogInfos) );
	if ( !wd )
	    return false;
	wd->logs().getNames( nms );
    }
    else if ( MGR().isLoaded(ky) )
    {
	RefMan<Well::Data> wd = MGR().get( ky );
	if ( !wd )
	    return false;
	wd->logs().getNames( nms );
    }
    else
    {
	RefMan<Well::Data> wd = new Well::Data;
	Well::Reader wr( ky, *wd );
	wr.getLogInfo( nms );
	if ( nms.isEmpty() )
	    return wr.getInfo(); // returning whether the well exists
    }
    return true;
}


bool Well::Man::getMarkerNames( BufferStringSet& nms )
{
    nms.setEmpty();
    const MultiID mid( IOObjContext::getStdDirData(IOObjContext::WllInf)->id_ );
    const IODir iodir( mid );
    PtrMan<CtxtIOObj> ctio = mMkCtxtIOObj( Well );
    const IODirEntryList del( iodir, ctio->ctxt_ );
    RefMan<Well::Data> data = new Well::Data;
    for ( int idx=0; idx<del.size(); idx++ )
    {
	const IOObj* ioobj = del[idx]->ioobj_;
	if ( !ioobj ) continue;

	Well::Reader wr( *ioobj, *data );
	if ( !wr.getTrack() || !wr.getMarkers() ) continue;

	BufferStringSet wllmarkernms;
	data->markers().getNames( wllmarkernms );
	nms.add( wllmarkernms, false );
    }

    return true;
}


IOObj* Well::findIOObj( const char* nm, const char* uwi )
{
    const MultiID mid( IOObjContext::getStdDirData(IOObjContext::WllInf)->id_ );
    const IODir iodir( mid );
    if ( nm && *nm )
    {
	const IOObj* ioobj = iodir.get( nm, "Well" );
	if ( ioobj ) return ioobj->clone();
    }

    if ( uwi && *uwi )
    {
	PtrMan<CtxtIOObj> ctio = mMkCtxtIOObj( Well );
	const IODirEntryList del( iodir, ctio->ctxt_ );
	RefMan<Well::Data> data = new Well::Data;
	for ( int idx=0; idx<del.size(); idx++ )
	{
	    const IOObj* ioobj = del[idx]->ioobj_;
	    if ( !ioobj ) continue;

	    Well::Reader wr( *ioobj, *data );
	    if ( wr.getInfo() && data->info().uwid == uwi )
		return ioobj->clone();
	}
    }

    return 0;
}
