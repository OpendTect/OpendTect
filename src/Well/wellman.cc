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
#include "ioobj.h"
#include "welltransl.h"
#include "ptrman.h"
#include "welldata.h"
#include "wellreader.h"


Well::Man* Well::Man::mgr_ = 0;

Well::Man& Well::MGR()
{
    if ( !::Well::Man::mgr_ )
	::Well::Man::mgr_ = new ::Well::Man;
    return *::Well::Man::mgr_;
}


Well::Man::~Man()
{
    removeAll();
}


void Well::Man::removeAll()
{
    ObjectSet<Well::Data> wellcopy = wells_;
    wells_.erase();
    deepErase( wellcopy );
}


void Well::Man::add( const MultiID& key, Well::Data* wll )
{
    wll->setMultiID( key );
    wells_ += wll;
}


Well::Data* Well::Man::release( const MultiID& key )
{
    const int idx = gtByKey( key );
    return idx < 0 ? 0 : wells_.removeSingle( idx );
}


Well::Data* Well::Man::get( const MultiID& key, bool forcereload )
{
    msg_.setEmpty();
    int wllidx = gtByKey( key );
    bool mustreplace = false;
    if ( wllidx >= 0 )
    {
	if ( !forcereload )
	    return wells_[wllidx];
	mustreplace = true;
    }

    Well::Data* wd = 0;
    PtrMan<IOObj> ioobj = IOM().get( key );
    if ( !ioobj )
    {
	delete wd;
	msg_.set( "Cannot find well ID " ).add( key ).add( "in data store." );
	return 0;
    }
    wd = new Well::Data;
    Well::Reader wr( *ioobj, *wd );
    if ( !wr.get() )
    {
	delete wd;
	msg_.set( "Cannot read '" ).add( ioobj->name() )
	    .add( "':\n" ).add( wr.errMsg() );
	return 0;
    }

    if ( mustreplace )
	delete wells_.replace( wllidx, wd );
    else
	add( key, wd );

    return wd;
}


bool Well::Man::isLoaded( const MultiID& key ) const
{
    return gtByKey( key ) >= 0;
}


bool Well::Man::reload( const MultiID& key )
{
    return get( key, true );
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


IOObj* Well::findIOObj( const char* nm, const char* uwi )
{
    const MultiID mid( IOObjContext::getStdDirData(IOObjContext::WllInf)->id );
    const IODir iodir( mid );
    if ( nm && *nm )
    {
	const IOObj* ioobj = iodir.get( nm, "Well" );
	if ( ioobj ) return ioobj->clone();
    }

    if ( uwi && *uwi )
    {
	PtrMan<CtxtIOObj> ctio = mMkCtxtIOObj( Well );
	const IODirEntryList del( iodir, ctio->ctxt );
	Well::Data data;
	for ( int idx=0; idx<del.size(); idx++ )
	{
	    const IOObj* ioobj = del[idx]->ioobj_;
	    if ( !ioobj ) continue;

	    Well::Reader wr( *ioobj, data );
	    if ( wr.getInfo() && data.info().uwid == uwi )
		return ioobj->clone();
	}
    }

    return 0;
}
