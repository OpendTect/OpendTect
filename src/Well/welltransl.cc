/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : May 2002
-*/

static const char* rcsID mUsedVar = "$Id$";


#include "welltransl.h"
#include "wellodreader.h"
#include "wellodwriter.h"
#include "welldata.h"
#include "wellextractdata.h"
#include "wellman.h"
#include "iostrm.h"
#include "strmprov.h"
#include "filepath.h"
#include "uistrings.h"

defineTranslatorGroup(Well,"Well");
defineTranslator(od,Well,"dGB");
mDefSimpleTranslatorSelector(Well);

uiString WellTranslatorGroup::sTypeName( int num )
{ return uiStrings::sWell( num ); }

mDefSimpleTranslatorioContext(Well,WllInf)


#define mImplStart(fn) \
    if ( !ioobj || ioobj->translator()!="dGB" ) \
	return false; \
    mDynamicCastGet(const IOStream*,iostrm,ioobj) \
    if ( !iostrm ) return false; \
\
    BufferString pathnm = iostrm->fileSpec().fullDirName(); \
    BufferString filenm = iostrm->fileSpec().fileName(); \
    StreamProvider prov( filenm ); \
    prov.addPathIfNecessary( pathnm ); \
    if ( !prov.fn ) return false;


#define mRemove(ext,nr,extra) \
{ \
    StreamProvider sp( Well::odIO::mkFileName(bnm,ext,nr) ); \
    sp.addPathIfNecessary( pathnm ); \
    const bool exists = sp.exists( true ); \
    if ( exists && !sp.remove(false) ) \
	return false; \
    extra; \
}

bool odWellTranslator::implRemove( const IOObj* ioobj ) const
{
    mImplStart(remove(false));

    FilePath fp( filenm ); fp.setExtension( 0, true );
    const BufferString bnm = fp.fullPath();
    mRemove(Well::odIO::sExtMarkers(),0,)
    mRemove(Well::odIO::sExtD2T(),0,)
    mRemove(Well::odIO::sExtCSMdl(),0,)
    mRemove(Well::odIO::sExtDispProps(),0,)
    mRemove(Well::odIO::sExtWellTieSetup(),0,)
    for ( int idx=1; ; idx++ )
	mRemove(Well::odIO::sExtLog(),idx,if ( !exists ) break)

    return true;
}


#define mRename(ext,nr,extra) \
{ \
    StreamProvider sp( Well::odIO::mkFileName(bnm,ext,nr) ); \
    sp.addPathIfNecessary( pathnm ); \
    StreamProvider spnew( Well::odIO::mkFileName(newbnm,ext,nr) ); \
    spnew.addPathIfNecessary( pathnm ); \
    const bool exists = sp.exists( true ); \
    if ( exists && !sp.rename(spnew.fileName(),cb) ) \
	return false; \
    extra; \
}

bool odWellTranslator::implRename( const IOObj* ioobj, const char* newnm,
				 const CallBack* cb ) const
{
    mImplStart(rename(newnm,cb));

    FilePath fp( filenm ); fp.setExtension( 0, true );
    const BufferString bnm = fp.fullPath();
    fp.set( newnm ); fp.setExtension( 0, true );
    const BufferString newbnm = fp.fullPath();
    mRename(Well::odIO::sExtMarkers(),0,)
    mRename(Well::odIO::sExtD2T(),0,)
    mRename(Well::odIO::sExtCSMdl(),0,)
    mRename(Well::odIO::sExtDispProps(),0,)
    mRename(Well::odIO::sExtWellTieSetup(),0,)
    for ( int idx=1; ; idx++ )
	mRename(Well::odIO::sExtLog(),idx,if ( !exists ) break)

    if ( Well::MGR().isLoaded(ioobj->key()) )
    {
	Well::LoadReqs reqs( Well::Inf );
	Well::Data* wd = Well::MGR().get( ioobj->key(), reqs );
	if ( wd )
	    wd->setName( ioobj->name() );
    }

    return true;
}


bool odWellTranslator::implSetReadOnly( const IOObj* ioobj, bool ro ) const
{
    mImplStart(setReadOnly(ro));
    return true;
}
