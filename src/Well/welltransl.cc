/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : May 2002
-*/

static const char* rcsID = "$Id: welltransl.cc,v 1.2 2003-08-22 16:40:34 bert Exp $";


#include "welltransl.h"
#include "wellreader.h"
#include "wellwriter.h"
#include "welldata.h"
#include "iostrm.h"
#include "strmprov.h"

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


static const char* getFileName( const IOObj& ioobj )
{
    static BufferString ret;
    mDynamicCastGet(const IOStream&,iostrm,ioobj)
    StreamProvider sp( iostrm.fileName() );
    sp.addPathIfNecessary( iostrm.dirName() );
    ret = sp.fileName();
    return ret.buf();
}


bool dgbWellTranslator::read( Well::Data& wd, const IOObj& ioobj )
{
    Well::Reader rdr( getFileName(ioobj), wd );
    bool ret = rdr.get();
    if ( ret )
	wd.info().setName( ioobj.name() );
    return ret;
}


bool dgbWellTranslator::write( const Well::Data& wd, const IOObj& ioobj )
{
    Well::Writer wrr( getFileName(ioobj), wd );
    return wrr.put();
}
