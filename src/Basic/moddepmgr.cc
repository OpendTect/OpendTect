/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Aug 2011
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";


#include "moddepmgr.h"
#include "debugmasks.h"
#include "file.h"
#include "filepath.h"
#include "oddirs.h"
#include "strmprov.h"
#include "envvars.h"
#include "errh.h"
#include "sharedlibs.h"
#include <iostream>

static const bool isdebug = 
# ifdef __debug__
    true;
# else
    false;
# endif

const OD::ModDepMgr& OD::ModDeps()
{
    static ModDepMgr* mgr = 0;
    if ( !mgr )
	mgr = new ModDepMgr;
    return *mgr;
}


OD::ModDepMgr::ModDepMgr( const char* mdfnm )	
{
    if ( !mdfnm || !*mdfnm )
	mdfnm = "ModDeps.od";
    
    const FilePath moddepfp( GetSoftwareDir(0), "data", mdfnm );
    const BufferString moddepfnm = moddepfp.fullPath();
    StreamData sd( StreamProvider( moddepfnm ).makeIStream() );
    if ( !sd.usable() )
    {
	if ( DBG::isOn(DBG_PROGSTART) )
	{
	    BufferString msg( "Ouch. Cannot read ", moddepfnm,"." );
	    DBG::message( msg );
	}
	return;
    }

    if ( DBG::isOn(DBG_PROGSTART) )
	DBG::message( BufferString("Start reading ",moddepfnm,".") );
    readDeps( *sd.istrm );
    if ( DBG::isOn(DBG_PROGSTART) )
	DBG::message( BufferString("Read ",moddepfnm,".") );
    sd.close();

    if ( !File::exists( GetBinPlfDir()) )
    {
	const BufferString msg( "Cannot find ", GetBinPlfDir() );
	pErrMsg( msg );
	return;
    }
}


void OD::ModDepMgr::readDeps( std::istream& strm )
{
    char linebuf[1024]; char wordbuf[256];

    while ( strm )
    {
	strm.getline( linebuf, 1024 );
	char* bufptr = linebuf; 
	mTrimBlanks(bufptr);
	if ( ! *bufptr || *bufptr == '#' )
	    continue;

	char* nextptr = (char*)getNextWord(bufptr,wordbuf);
	if ( ! wordbuf[0] ) continue;
	od_int64 l = strlen( wordbuf );
	if ( wordbuf[l-1] == ':' ) wordbuf[l-1] = '\0';
	if ( ! wordbuf[0] ) continue;

	*nextptr++ = '\0';
	mSkipBlanks(nextptr);

	ModDep* newdep = new ModDep( wordbuf ) ;
	BufferStringSet filedeps;
	while ( nextptr && *nextptr )
	{
	    mSkipBlanks(nextptr);
	    nextptr = (char*)getNextWord(nextptr,wordbuf);
	    if ( !wordbuf[0] ) break;

	    if ( wordbuf[1] != '.' || (wordbuf[0] != 'S' && wordbuf[0] != 'D') )
	    {
		if ( DBG::isOn(DBG_PROGSTART) )
		    DBG::message( BufferString("Found bad dep: ",wordbuf,".") );
		continue;
	    }

	    filedeps.add( wordbuf );
	}


	BufferStringSet depmods;
	for ( int idx=filedeps.size()-1; idx>=0; idx-- )
	{
	    const char* filedep = filedeps.get(idx).buf();
	    const char* modnm = filedep + 2;
	    if ( *filedep == 'S' )
	    {
		depmods.add( modnm );
	        continue;
	    }

	    const ModDep* depdep = find( modnm );
	    if ( !depdep )
	    {
		if ( DBG::isOn(DBG_PROGSTART) )
		    DBG::message( BufferString("Cannot find dep: ",modnm,".") );
		continue;
	    }

	    for ( int idep=depdep->mods_.size()-1; idep>=0; idep-- )
	    {
		const char* depdepmod = depdep->mods_.get(idep).buf();
		if ( !depmods.isPresent(depdepmod) )
		    depmods.add( depdepmod );
	    }
	}
	if ( depmods.size() < 1 )
	    { delete newdep; continue; }
	deps_ += newdep;

	for ( int idx=depmods.size()-1; idx>=0; idx-- )
	    newdep->mods_.add( depmods.get(idx) );
    }
}


const OD::ModDep* OD::ModDepMgr::find( const char* nm ) const
{
    return ::find( deps_, nm );
}


void OD::ModDepMgr::ensureLoaded( const char* nm ) const
{
    if ( !nm || !*nm ) return;

    const OD::ModDep* md = find( nm );
    if ( !md ) return;

    for ( int idep=md->mods_.size()-1; idep>=0; idep-- )
    {
	const BufferString& modnm( md->mods_.get(idep) );
	const int loadedidx = getLoadIdx( modnm );
	if ( loadedidx >= 0 )
	    continue;

	char libnm[256];
	SharedLibAccess::getLibName( md->mods_.get(idep), libnm );
	FilePath fp( GetBinPlfDir(), libnm );
	SharedLibAccess* sla = new SharedLibAccess( fp.fullPath() );
	if ( !sla->isOK() )
	    { delete sla; continue; }

	loadedmods_.add( modnm );
	shlibaccs_ += sla;

	BufferString fnnm( "od_" );
	fnnm.add( modnm ).add( "_initStdClasses" );
	typedef void (*ModuleInitFn)(void);
	ModuleInitFn fn = (ModuleInitFn)sla->getFunction( fnnm );
	if ( fn )
	    (*fn)();
    }
}


const SharedLibAccess* OD::ModDepMgr::shLibAccess( const char* nm ) const
{
    const int loadedidx = getLoadIdx( nm );
    return loadedidx < 0 ? 0 : shlibaccs_[loadedidx];
}


int OD::ModDepMgr::getLoadIdx( const char* nm ) const
{
    return loadedmods_.indexOf( nm );
}
