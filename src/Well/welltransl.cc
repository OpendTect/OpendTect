/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : May 2002
-*/

static const char* rcsID = "$Id: welltransl.cc,v 1.6 2003-11-07 12:21:58 bert Exp $";


#include "welltransl.h"
#include "wellfact.h"
#include "wellreader.h"
#include "wellwriter.h"
#include "welldata.h"
#include "iostrm.h"
#include "strmprov.h"
#include "filegen.h"

const IOObjContext& WellTranslatorGroup::ioContext()
{
    static IOObjContext* ctxt = 0;

    if ( !ctxt )
    {
	ctxt = new IOObjContext( &theInst() );
	ctxt->crlink = false;
	ctxt->newonlevel = 1;
	ctxt->needparent = false;
	ctxt->maychdir = false;
	ctxt->stdseltype = IOObjContext::WllInf;
    }

    return *ctxt;
}


int WellTranslatorGroup::selector( const char* key )
{
    int retval = defaultSelector( theInst().userName(), key );
    return retval;
}

#define mImplStart(fn) \
    if ( !ioobj || strcmp(ioobj->translator(),"dGB") ) return false; \
    mDynamicCastGet(const IOStream*,iostrm,ioobj) \
    if ( !iostrm ) return false; \
\
    BufferString pathnm = iostrm->dirName(); \
    BufferString filenm = iostrm->fileName(); \
    StreamProvider sp( filenm ); \
    sp.addPathIfNecessary( pathnm ); \
    if ( !sp.fn ) return false;


#define mRemove(ext,nr) \
{ \
    StreamProvider sp( Well::IO::mkFileName(bnm,ext,nr) ); \
    sp.addPathIfNecessary( pathnm ); \
    if ( !sp.exists(true) )  return true; \
    if ( !sp.remove(false) ) return false; \
}

bool WellTranslator::implRemove( const IOObj* ioobj ) const
{
    mImplStart(remove(false));

    BufferString bnm = File_removeExtension( filenm );
    mRemove(Well::IO::sExtMarkers,0)
    mRemove(Well::IO::sExtD2T,0)
    for ( int idx=1; ; idx++ )
	mRemove(Well::IO::sExtLog,idx)

    return true;
}


#define mRename(ext,nr) \
{ \
    StreamProvider sp( Well::IO::mkFileName(bnm,ext,nr) ); \
    sp.addPathIfNecessary( pathnm ); \
    StreamProvider spnew( Well::IO::mkFileName(newbnm,ext,nr) ); \
    spnew.addPathIfNecessary( pathnm ); \
    if ( !sp.exists(true) )  return true; \
    if ( !sp.rename(spnew.fileName(),cb) ) return false; \
}

bool WellTranslator::implRename( const IOObj* ioobj, const char* newnm,
				 const CallBack* cb ) const
{
    mImplStart(rename(newnm,cb));

    BufferString bnm = File_removeExtension( filenm );
    BufferString newbnm = File_removeExtension( newnm );
    mRename(Well::IO::sExtMarkers,0)
    mRename(Well::IO::sExtD2T,0)

    for ( int idx=1; ; idx++ )
	mRename(Well::IO::sExtLog,idx)
    
    return true;
}


bool WellTranslator::implSetReadOnly( const IOObj* ioobj, bool ro ) const
{
    mImplStart(setReadOnly(ro));
    return true;
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
