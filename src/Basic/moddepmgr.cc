/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "moddepmgr.h"
#include "debug.h"
#include "file.h"
#include "ptrman.h"
#include "filepath.h"
#include "oddirs.h"
#include "sharedlibs.h"
#include "od_istream.h"


const OD::ModDepMgr& OD::ModDeps()
{
    static PtrMan<ModDepMgr> mgr =
	 new ModDepMgr(
		 FilePath(GetSWSetupShareFileName(
			    BufferString("ModDeps.od").str())).fullPath(),
		 GetLibPlfDir() );
    return *mgr;
}


OD::ModDepMgr::ModDepMgr( const char* mdfnm, const char* libplfdir )
    : libplfdir_(libplfdir)
{
    if ( !File::exists(mdfnm) )
	return;

    const FilePath moddepfp( mdfnm );
    prefix_ = moddepfp.extension();
    od_istream strm( mdfnm );
    if ( !strm.isOK() )
    {
	const uiString msg = strm.errMsg();
	pErrMsg( BufferString("Missing module dependency. ", msg ) );
	return;
    }

    if ( DBG::isOn(DBG_PROGSTART) )
	DBG::message( BufferString("Starting reading ",mdfnm,".") );

    readDeps( strm );
    if ( DBG::isOn(DBG_PROGSTART) )
	DBG::message( BufferString("Reading ",mdfnm," done.") );

    if ( !File::exists( GetExecPlfDir()) )
	{ ErrMsg( BufferString( "Cannot find ", GetExecPlfDir() ) ); return; }
}


OD::ModDepMgr::~ModDepMgr()
{
    deepErase( shlibaccs_ );
}


void OD::ModDepMgr::closeAll()
{
    for ( auto* shlibaccs : shlibaccs_ )
	shlibaccs->close();
}


static BufferString mkErrMsg( od_istream& strm, const char* msg,
				const char* detail )
{
    BufferString ret( strm.fileName(), ": ", msg );
    if ( detail )
	ret.add( "\n'" ).add( detail ).add( "'" );
    return ret;
}


void OD::ModDepMgr::readDeps( od_istream& strm )
{
    BufferString word;
    while ( strm.getWord(word,true) )
    {
	word.trimBlanks();
	if ( word.isEmpty() || *word.buf() == '#' )
	    continue;

	const int wordlen = word.size();
	if ( word[wordlen-1] != ':' )
	    { ErrMsg( mkErrMsg(strm,"Invalid line found",word) ); continue; }
	word[wordlen-1] = '\0';

	auto* newdep = new ModDep( word );

	BufferStringSet filedeps;
	while ( strm.getWord(word,false) )
	{
	    if ( word[1] != '.' || (word[0] != 'S' && word[0] != 'D') )
		{ ErrMsg( mkErrMsg(strm,"Invalid moddep",word) ); continue; }

	    filedeps.add( word );
	}


	BufferStringSet depmods;
	for ( int idx=filedeps.size()-1; idx>=0; idx-- )
	{
	    const BufferString& filedep = filedeps.get(idx);
	    const char* modnm = filedep.buf() + 2;
	    if ( filedep[0] == 'S' )
		{ depmods.add( modnm ); continue; }

	    const ModDep* depdep = find( modnm );
	    if ( !depdep && this != &ModDeps() )
		depdep = ModDeps().find( modnm );

	    if ( !depdep )
		{ ErrMsg( mkErrMsg(strm,"Moddep not found",modnm) ); continue; }

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
    if ( !nm || !*nm )
	return;

    const OD::ModDep* md = find( nm );
    if ( !md && this != &ModDeps() )
	md = ModDeps().find( nm );

    if ( !md )
	return;

    for ( int idep=md->mods_.size()-1; idep>=0; idep-- )
    {
	const BufferString& modnm( md->mods_.get(idep) );
	if ( modnm == "AllNonUi" )
	    continue; // Not a library, just a group label

	const int loadedidx = getLoadIdx( modnm );
	if ( loadedidx >= 0 )
	    continue;

	BufferString libnm( 256, false );
	SharedLibAccess::getLibName( md->mods_.get(idep),
				     libnm.getCStr(), libnm.bufSize() );
	BufferString prefix( prefix_ );
	FilePath fp( libplfdir_, libnm );
	if ( !fp.exists() )
	{
	    const FilePath odfp( ModDeps().getLibPlfDir(), libnm );
	    if ( odfp.exists() )
	    {
		fp.set( odfp.fullPath() );
		prefix = ModDeps().getPrefix();
	    }
	}

	auto* sla = new SharedLibAccess( fp.fullPath() );
	if ( !sla->isOK() )
	    { ErrMsg( sla->errMsg() ); delete sla; continue; }

	loadedmods_.add( modnm );
	shlibaccs_ += sla;

	BufferString fnnm( prefix.buf(), "_" );
	fnnm.add( modnm ).add( "_initStdClasses" );

	using ModuleInitFn = void(*)(void);
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
