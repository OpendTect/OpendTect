/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Aug 2003
-*/

static const char* rcsID = "$Id: wellman.cc,v 1.1 2003-08-27 10:19:39 bert Exp $";

#include "welldata.h"
#include "wellman.h"
#include "welltransl.h"
#include "ioman.h"
#include "ioobj.h"
#include "ptrman.h"

Well::Man* Well::Man::mgr_ = 0;

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
    {
	wells_ += wd;
	keys_ += new MultiID( key );
    }

    delete tr;
    return wd;

}
