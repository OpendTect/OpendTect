/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : May 2002
-*/

static const char* rcsID = "$Id: welltransl.cc,v 1.1 2003-08-22 11:20:53 nanne Exp $";


#include "welltransl.h"
#include "wellreader.h"
#include "wellwriter.h"
#include "welldata.h"
#include "iostrm.h"

const IOObjContext& WellTranslator::ioContext()
{
    static IOObjContext* ctxt = 0;

    if ( !ctxt )
    {
	ctxt = new IOObjContext( Translator::groups()[listid] );
	ctxt->crlink = false;
	ctxt->newonlevel = 1;
	ctxt->needparent = false;
	ctxt->maychdir = false;
	ctxt->stdseltype = IOObjContext::WllInf;
    }

    return *ctxt;
}


int WellTranslator::selector( const char* key )
{
    int retval = defaultSelector( "Well", key );
    return retval;
}


bool dgbWellTranslator::read( Well::Data& wd, const IOObj& ioobj )
{
    mDynamicCastGet(const IOStream&,iostrm,ioobj)
    Well::Reader rdr( iostrm.fileName(), wd );
    bool ret = rdr.get();
    if ( ret )
	wd.info().setName( ioobj.name() );
    return ret;
}


bool dgbWellTranslator::write( const Well::Data& wd, const IOObj& ioobj )
{
    mDynamicCastGet(const IOStream&,iostrm,ioobj)
    Well::Writer wrr( iostrm.fileName(), wd );
    return wrr.put();
}
