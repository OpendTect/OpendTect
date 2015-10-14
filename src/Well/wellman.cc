/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Aug 2003
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "wellman.h"

#include "iodir.h"
#include "iodirentry.h"
#include "welltransl.h"
#include "ptrman.h"
#include "welldata.h"
#include "wellreader.h"
#include "welllogset.h"


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


void Well::Man::add( const MultiID& key, Well::Data* wll )
{
    if ( !wll ) return;

    wll->setMultiID( key );
    wells_ += wll;
}


Well::Data* Well::Man::release( const MultiID& key )
{
    const int idx = gtByKey( key );
    if ( idx < 0 ) return 0;

    return wells_.removeSingle( idx );
}


Well::Data* Well::Man::get( const MultiID& key )
{
    msg_.setEmpty();

    const int wdidx = gtByKey( key );
    if ( wdidx>=0 )
	return wells_[wdidx];

    Well::Data* wd = new Well::Data;
    wd->ref();

    Well::Reader wr( key, *wd );
    if ( !wr.get() )
    {
	msg_.set( wr.errMsg() );
	wd->unRef();
	return 0;
    }

    add( key, wd );
    wd->unRefNoDelete();
    return wd;
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
    Well::Reader wr( key, *wd );
    if ( !wr.get() )
    {
	msg_.set( wr.errMsg() );
	wd->unRef();
	return false;
    }

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


bool Well::Man::getLogNames( const MultiID& ky, BufferStringSet& nms )
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
	Well::Reader wr( ky, *wd );
	wr.getLogInfo( nms );
	if ( nms.isEmpty() )
	    return wr.getInfo(); // returning whether the well exists
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

