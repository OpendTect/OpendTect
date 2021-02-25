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
	reqs_[LogInfos] = 1;
    return *this;
}


Well::LoadReqs Well::LoadReqs::All()
{
    LoadReqs ret( false );
    ret.reqs_.set();
    if ( !SI().zIsTime() )
        ret.reqs_[D2T] = 0;

    return ret;
}


void Well::LoadReqs::include( const LoadReqs& oth )
{
    for ( int idx=0; idx<mWellNrSubObjTypes; idx++ )
    {
	if ( oth.reqs_[idx]==1 )
            reqs_[ idx ] = 1;
    }

    if ( reqs_[Logs]==1 )
	reqs_[LogInfos] = 1;
}


void Well::LoadReqs::exclude( const LoadReqs& oth )
{
    for ( int idx=0; idx<mWellNrSubObjTypes; idx++ )
    {
	if ( oth.reqs_[idx]==1 )
	    reqs_[ idx ] = 0;
    }

    if ( reqs_[Logs]==1 )
	reqs_[LogInfos] = 1;
}


bool Well::LoadReqs::includes( const LoadReqs& oth ) const
{
    for ( int idx=0; idx<mWellNrSubObjTypes; idx++ )
	if ( oth.reqs_[idx]==1 && reqs_[idx]==0 )
            return false;
    return true;
}


BufferString Well::LoadReqs::toString() const
{
    BufferString res;
    BufferString tmp;
    tmp.add( "Inf Trck D2T CSMdl Mrkrs Logs LogInfos DispProps2D DispProps3D" );
    BufferStringSet nms;
    nms.unCat( tmp, " " );
    for ( int ib=0; ib<reqs_.size(); ib++ )
    {
	if ( reqs_[ib]==1 )
	    res.add( nms.get(ib) ).addSpace();
    }
    return res;
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
}


Well::Data* Well::Man::release( const MultiID& key )
{
    const int idx = gtByKey( key );
    if ( idx < 0 ) return 0;

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
    if ( wd && wd->loadState().includes(reqs) )
        return wd;
    if ( wd && wdidx >=0 )
    {
	reqs.exclude( wd->loadState() );
	if ( !readReqData(key,wd,reqs) )
	    return nullptr;

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
    else if ( reqs.includes(LogInfos) )
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


bool Well::Man::reload( const MultiID& key, LoadReqs lreqs )
{
    const int wdidx = gtByKey( key );
    if ( wdidx<0 ) return false;

    Well::Data* wd = wells_[wdidx];
    wd->ref();
    if ( lreqs.isEmpty() )
	lreqs = wd->loadState();
    bool res = readReqData( key, wd, lreqs );
    wd->reloaded.trigger();
    wd->unRef();
    return res;
}


bool Well::Man::reloadDispPars( const MultiID& key, bool for2d )
{
    const int wdidx = gtByKey( key );
    if ( wdidx<0 ) return false;

    const LoadReqs lreqs(for2d ? Well::DispProps2D : Well::DispProps3D);
    Well::Data* wd = wells_[wdidx];
    wd->ref();
    bool res = readReqData( key, wd, lreqs );
    if ( res )
	for2d ? wd->disp2dparschanged.trigger()
	      : wd->disp3dparschanged.trigger();

    wd->unRef();
    return res;
}


bool Well::Man::validID( const MultiID& mid ) const
{
    const IOObj* ioobj = IOM().get( mid );
    return ioobj ? ioobj->group()==mTranslGroupName(Well) : false;
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


bool Well::Man::getAllMarkerNames( BufferStringSet& nms, bool onlyloaded )
{
    nms.setEmpty();
    TypeSet<MultiID> ids;
    Well::MGR().getWellKeys( ids, onlyloaded );
    for ( int idx=0; idx<ids.size(); idx++ )
    {
	ConstRefMan<Well::Data> wd = Well::MGR().get( ids[idx],
						Well::LoadReqs(Well::Mrkrs) );
	BufferStringSet markernms;
	wd->markers().getNames( markernms );
	nms.add( markernms, false );
    }
    return !nms.isEmpty();
}


bool Well::Man::getAllMarkerInfo( BufferStringSet& nms,
				    TypeSet<OD::Color>& cols, bool onlyloaded )
{
    nms.setEmpty();
    cols.setEmpty();
    TypeSet<MultiID> ids;
    Well::MGR().getWellKeys( ids, onlyloaded );
    for ( int idx=0; idx<ids.size(); idx++ )
    {
	ConstRefMan<Well::Data> wd = Well::MGR().get( ids[idx],
						Well::LoadReqs(Well::Mrkrs) );
	BufferStringSet markernms;
	TypeSet<OD::Color> colors;
	wd->markers().getNames( markernms );
	wd->markers().getColors( colors );
	for ( int im=0; im<markernms.size(); im++ )
	{
	    if ( !nms.isPresent( markernms.get( im ) ) )
	    {
		nms.add( markernms.get( im ) );
		cols += colors[im];
	    }
	}
    }
    return !nms.isEmpty();
}


bool Well::Man::getAllLogNames( BufferStringSet& lognms, bool onlyloaded )
{
    lognms.setEmpty();
    TypeSet<MultiID> ids;
    getWellKeys( ids, onlyloaded );
    for ( int idx=0; idx<ids.size(); idx++ )
    {
	BufferStringSet logs;
	Well::MGR().getLogNamesByID( ids[idx], logs, onlyloaded );
	lognms.add( logs, false );
    }
    return !lognms.isEmpty();
}


bool Well::Man::getMarkersByID( const MultiID& mid, BufferStringSet& nms )
{
    nms.setEmpty();
    if ( Well::MGR().validID( mid ) )
    {
	ConstRefMan<Well::Data> wd =
			Well::MGR().get( mid, Well::LoadReqs( Well::Mrkrs ) );
	if ( wd )
	    wd->markers().getNames( nms );
    }
    return !nms.isEmpty();
}


bool Well::Man::getMarkersByID( const MultiID& mid, BufferStringSet& nms,
				TypeSet<OD::Color>& cols )
{
    nms.setEmpty();
    cols.setEmpty();
    if ( Well::MGR().validID( mid ) )
    {
	ConstRefMan<Well::Data> wd =
			Well::MGR().get( mid, Well::LoadReqs( Well::Mrkrs ) );
	if ( wd )
	{
	    wd->markers().getNames( nms );
	    wd->markers().getColors( cols );
	}
    }
    return nms.isEmpty();
}


bool Well::Man::getMarkersByID( const MultiID& mid, BufferStringSet& nms,
				TypeSet<OD::Color>& cols, TypeSet<float>& zs )
{
    nms.setEmpty();
    cols.setEmpty();
    zs.setEmpty();
    if ( Well::MGR().validID( mid ) )
    {
	ConstRefMan<Well::Data> wd =
			Well::MGR().get( mid, Well::LoadReqs( Well::Mrkrs ) );
	if ( wd )
	    wd->markers().getNamesColorsMDs( nms, cols, zs );
    }
    return !nms.isEmpty();
}


bool Well::Man::getLogNamesByID( const MultiID& ky, BufferStringSet& nms,
				 bool onlyloaded )
{
    nms.setEmpty();
    if ( !Well::MGR().validID( ky ) )
	return false;

    const int idx = Well::MGR().gtByKey( ky );
    const bool isloaded = idx>=0;
    if ( !onlyloaded )
    {
	RefMan<Well::Data> wd = new Well::Data;
	if ( wd )
	{
	    Reader rdr( ky, *wd );
	    rdr.getLogInfo( nms );
	}
    }
    else if ( isloaded )
	Well::MGR().wells()[idx]->logs().getNames( nms );

    return !nms.isEmpty();
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
    return Well::MGR().getAllMarkerNames( nms );
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

void Well::Man::dumpMgrInfo( IOPar& res )
{
    auto& wells = MGR().wells();
    res.set( "Number of Wells", wells.size() );
    for ( int idx=0; idx<wells.size(); idx++ )
    {
	IOPar wpar;
	ConstRefMan<Well::Data> wd = wells[idx];
	if ( wd )
	{
	    wpar.set( "References", wd->nrRefs()-1 );
	    wpar.set( "Load State", wd->loadState().toString() );
	    wpar.set( "Markers", wd->markers().size() );
	    const LogSet& ls = wd->logs();
	    int nlogswithdata = 0;
	    for (int il=0; il<ls.size(); il++)
	    {
		const Log& log = ls.getLog( il );
		if ( log.isLoaded() )
		    nlogswithdata++;
	    }
	    wpar.set( "Logs available", ls.size() );
	    wpar.set( "Logs with Info", ls.size() );
	    wpar.set( "Logs with data", nlogswithdata );
	    res.mergeComp( wpar, wd->info().name() );
	}
    }
}
