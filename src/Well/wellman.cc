/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Aug 2003
-*/

static const char* rcsID = "$Id: wellman.cc,v 1.4 2003-10-31 11:55:23 nanne Exp $";

#include "welldata.h"
#include "wellman.h"
#include "welltransl.h"
#include "ioman.h"
#include "ioobj.h"
#include "ptrman.h"

Well::Man* Well::Man::mgr_ = 0;


Well::Man::~Man()
{
    deepErase( wells_ );
    deepErase( keys_ );
}


void Well::Man::add( const MultiID& key, Well::Data* wll )
{
    wells_ += wll;
    keys_ += new MultiID( key );
}


Well::Data* Well::Man::release( const MultiID& key )
{
    const int idx = indexOf( keys_, key );
    if ( idx < 0 ) return 0;

    delete keys_[idx];
    keys_.remove( idx );
    Well::Data* w = wells_[idx];
    wells_.remove( idx );
    return w;
}


#define mErrRet(s) { delete tr; delete wd; msg_ = s; return 0; }


Well::Data* Well::Man::get( const MultiID& key, bool forcereload )
{
    msg_ = "";
    int wllidx = indexOf( keys_, key );
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
	delete wells_.replace( wd, wllidx );
    else
	add( key, wd );

    delete tr;
    return wd;
}


bool Well::Man::isLoaded( const MultiID& key ) const
{
    const int wllidx = indexOf( keys_, key );
    return wllidx >= 0;
}


bool Well::Man::reload( const MultiID& key )
{
    Well::Data* wd = 0;
    if ( isLoaded(key) )
	wd = get( key, true );
    return wd;
}
