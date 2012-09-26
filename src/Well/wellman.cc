/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Aug 2003
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "welldata.h"
#include "wellman.h"
#include "welltransl.h"
#include "ioman.h"
#include "ioobj.h"
#include "ptrman.h"

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
    return idx < 0 ? 0 : wells_.remove( idx );
}


#define mErrRet(s) { delete tr; delete wd; msg_ = s; return 0; }


Well::Data* Well::Man::get( const MultiID& key, bool forcereload )
{
    msg_ = "";
    int wllidx = gtByKey( key );
    bool mustreplace = false;
    if ( wllidx >= 0 )
    {
	if ( !forcereload )
	    return wells_[wllidx];
	mustreplace = true;
    }

    Translator* tr = 0; Well::Data* wd = 0;

    PtrMan<IOObj> ioobj = IOM().get( key );
    if ( !ioobj )
	mErrRet("Cannot find well key in data store")
    tr = ioobj->getTranslator();
    if ( !tr )
	mErrRet("Well translator not found")
    mDynamicCastGet(WellTranslator*,wtr,tr)
    if ( !wtr )
	mErrRet("Translator produced is not a Well Transator")

    wd = new Well::Data;
    if ( !wtr->read(*wd,*ioobj) )
	mErrRet("Cannot read well from files")

    if ( mustreplace )
	delete wells_.replace( wllidx, wd );
    else
	add( key, wd );

    delete tr;
    return wd;
}


bool Well::Man::isLoaded( const MultiID& key ) const
{ return gtByKey( key ) >= 0; }


bool Well::Man::reload( const MultiID& key )
{
    Well::Data* wd = 0;
    if ( isLoaded(key) )
	wd = get( key, true );
    return wd;
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
