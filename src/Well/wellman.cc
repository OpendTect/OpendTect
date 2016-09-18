/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Aug 2003
-*/


#include "wellman.h"

#include "iodir.h"
#include "iodirentry.h"
#include "welltransl.h"
#include "ptrman.h"
#include "welldata.h"
#include "wellinfo.h"
#include "wellreader.h"
#include "welllogset.h"
#include "wellmarker.h"


Well::Man* Well::Man::mgr_ = 0;

Well::Man& Well::MGR()
{
    if ( !::Well::Man::mgr_ )
	::Well::Man::mgr_ = new ::Well::Man;
    return *::Well::Man::mgr_;
}


Well::Man::~Man()
{
}


void Well::Man::removeObject( const Well::Data* wd )
{
    const int idx = wells_.indexOf( wd );
    if ( idx < 0 ) return;

    wells_.removeSingle( idx );
}


void Well::Man::add( const DBKey& key, Well::Data* wll )
{
    if ( !wll ) return;

    wll->setDBKey( key );
    wells_ += wll;
}


Well::Data* Well::Man::release( const DBKey& key )
{
    const int idx = gtByKey( key );
    if ( idx < 0 ) return 0;

    return wells_.removeSingle( idx );
}


Well::Data* Well::Man::get( const DBKey& key )
{
    msg_.setEmpty();

    const int wdidx = gtByKey( key );
    if ( wdidx>=0 )
	return wells_[wdidx];

    Well::Data* wd = new Well::Data;
    wd->ref();

    Well::Reader rdr( key, *wd );
    if ( !rdr.get() )
    {
	msg_ = rdr.errMsg();
	wd->unRef();
	return 0;
    }

    add( key, wd );
    wd->unRefNoDelete();
    return wd;
}


bool Well::Man::isLoaded( const DBKey& key ) const
{
    return gtByKey( key ) >= 0;
}


bool Well::Man::reload( const DBKey& key )
{
    msg_.setEmpty();
    const int wdidx = gtByKey( key );
    if ( wdidx<0 )
	return false;

    Well::Data* wd = wells_[wdidx];
    wd->ref();
    Well::Reader rdr( key, *wd );
    if ( !rdr.get() )
    {
	msg_ = rdr.errMsg();
	wd->unRef();
	return false;
    }

    wd->reloaded.trigger();
    wd->unRef();
    return true;
}


int Well::Man::gtByKey( const DBKey& key ) const
{
    for ( int idx=0; idx<wells_.size(); idx++ )
    {
	if ( wells_[idx]->dbKey() == key )
	    return idx;
    }
    return -1;
}


bool Well::Man::getLogNames( const DBKey& ky, BufferStringSet& nms )
{
    nms.setEmpty();
    if ( MGR().isLoaded(ky) )
    {
	RefMan<Well::Data> wd = MGR().get( ky );
	if ( !wd )
	    return false;
	wd->logs().getNames( nms );
    }
    else
    {
	RefMan<Well::Data> wd = new Well::Data;
	Well::Reader rdr( ky, *wd );
	rdr.getLogInfo( nms );
	if ( nms.isEmpty() )
	    return rdr.getInfo(); // returning whether the well exists
    }
    return true;
}


bool Well::Man::getMarkerNames( BufferStringSet& nms )
{
    nms.setEmpty();
    const IODir iodir( IOObjContext::WllInf );
    IODirEntryList entrylist( iodir, &WellTranslatorGroup::theInst() );
    for ( IODirEntryList::IdxType idx=0; idx<entrylist.size(); idx++ )
    {
	const IOObj& ioobj = entrylist.ioobj( idx );
	RefMan<Well::Data> data = new Well::Data;
	Well::Reader rdr( ioobj, *data );
	if ( rdr.getTrack() && rdr.getMarkers() )
	{
	    BufferStringSet wllmarkernms;
	    data->markers().getNames( wllmarkernms );
	    nms.add( wllmarkernms, false );
	}
    }

    return true;
}


IOObj* Well::findIOObj( const char* nm, const char* uwi )
{
    const IODir iodir( IOObjContext::WllInf );
    if ( nm && *nm )
    {
	const IOObj* ioobj = iodir.getEntryByName( nm, "Well" );
	if ( ioobj )
	    return ioobj->clone();
    }

    if ( uwi && *uwi )
    {
	IODirEntryList entrylist( iodir, &WellTranslatorGroup::theInst() );
	for ( IODirEntryList::IdxType idx=0; idx<entrylist.size(); idx++ )
	{
	    const IOObj& ioobj = entrylist.ioobj( idx );
	    RefMan<Well::Data> data = new Well::Data;
	    Well::Reader rdr( ioobj, *data );
	    if ( rdr.getInfo() && data->info().UWI() == uwi )
		return ioobj.clone();
	}
    }

    return 0;
}


Well::ManData::ManData( const DBKey& id )
    : id_(id)
{
}


bool Well::ManData::isAvailable() const
{
    return Well::MGR().get( id_ );
}


Well::Data& Well::ManData::data()
{
    return *Well::MGR().get( id_ );
}


const Well::Data& Well::ManData::data() const
{
    return *Well::MGR().get( id_ );
}
