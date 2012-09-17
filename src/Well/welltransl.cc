/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : May 2002
-*/

static const char* rcsID = "$Id: welltransl.cc,v 1.24 2012/03/01 13:01:02 cvsbruno Exp $";


#include "welltransl.h"
#include "wellfact.h"
#include "wellreader.h"
#include "wellwriter.h"
#include "welldata.h"
#include "wellextractdata.h"
#include "iostrm.h"
#include "strmprov.h"
#include "filepath.h"

mDefSimpleTranslatorSelector(Well,sKeyWellTranslatorGroup)
mDefSimpleTranslatorioContext(Well,WllInf)


#define mImplStart(fn) \
    if ( !ioobj || strcmp(ioobj->translator(),"dGB") ) return false; \
    mDynamicCastGet(const IOStream*,iostrm,ioobj) \
    if ( !iostrm ) return false; \
\
    BufferString pathnm = iostrm->fullDirName(); \
    BufferString filenm = iostrm->fileName(); \
    StreamProvider prov( filenm ); \
    prov.addPathIfNecessary( pathnm ); \
    if ( !prov.fn ) return false;


#define mRemove(ext,nr,extra) \
{ \
    StreamProvider sp( Well::IO::mkFileName(bnm,ext,nr) ); \
    sp.addPathIfNecessary( pathnm ); \
    const bool exists = sp.exists( true ); \
    if ( exists && !sp.remove(false) ) \
	return false; \
    extra; \
}

bool WellTranslator::implRemove( const IOObj* ioobj ) const
{
    mImplStart(remove(false));

    FilePath fp( filenm ); fp.setExtension( 0, true );
    const BufferString bnm = fp.fullPath();
    mRemove(Well::IO::sExtMarkers(),0,)
    mRemove(Well::IO::sExtD2T(),0,)
    mRemove(Well::IO::sExtCSMdl(),0,)
    mRemove(Well::IO::sExtDispProps(),0,)
    mRemove(Well::IO::sExtWellTieSetup(),0,)
    for ( int idx=1; ; idx++ )
	mRemove(Well::IO::sExtLog(),idx,if ( !exists ) break)

    return true;
}


#define mRename(ext,nr,extra) \
{ \
    StreamProvider sp( Well::IO::mkFileName(bnm,ext,nr) ); \
    sp.addPathIfNecessary( pathnm ); \
    StreamProvider spnew( Well::IO::mkFileName(newbnm,ext,nr) ); \
    spnew.addPathIfNecessary( pathnm ); \
    const bool exists = sp.exists( true ); \
    if ( exists && !sp.rename(spnew.fileName(),cb) ) \
	return false; \
    extra; \
}

bool WellTranslator::implRename( const IOObj* ioobj, const char* newnm,
				 const CallBack* cb ) const
{
    mImplStart(rename(newnm,cb));

    FilePath fp( filenm ); fp.setExtension( 0, true );
    const BufferString bnm = fp.fullPath();
    fp.set( newnm ); fp.setExtension( 0, true );
    const BufferString newbnm = fp.fullPath();
    mRename(Well::IO::sExtMarkers(),0,)
    mRename(Well::IO::sExtD2T(),0,)
    mRename(Well::IO::sExtCSMdl(),0,)
    mRename(Well::IO::sExtDispProps(),0,)
    mRename(Well::IO::sExtWellTieSetup(),0,)
    for ( int idx=1; ; idx++ )
	mRename(Well::IO::sExtLog(),idx,if ( !exists ) break)
    
    return true;
}


bool WellTranslator::implSetReadOnly( const IOObj* ioobj, bool ro ) const
{
    mImplStart(setReadOnly(ro));
    return true;
}


Executor* WellTranslator::createDataPointSets(	const BufferStringSet& ids,
						const IOPar& pars, bool for2d,
						ObjectSet<DataPointSet>& dpss,
       						bool ztm )
{
    Well::TrackSampler* ts = new Well::TrackSampler( ids, dpss, ztm );
    ts->for2d_ = for2d;
    ts->usePar( pars );
    return ts;
}


static const char* getFileName( const IOObj& ioobj )
{
    static BufferString ret;
    ret = ioobj.fullUserExpr( true );
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
