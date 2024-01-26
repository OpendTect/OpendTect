/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "svnaccess.h"
#include "filepath.h"
#include "file.h"
#include "genc.h"
#include "od_iostream.h"
#include "oscommand.h"
#include "dirlist.h"


static void addRedirect( OS::MachineCommand& mc )
{
#ifndef __debug__
    mc.addFileRedirect( "/dev/null" )
      .addFileRedirect( "&1", 2 );
#endif
}

static bool executeCommand( OS::MachineCommand& mc, const BufferString& dir )
{
    addRedirect( mc );
    OS::CommandExecPars pars( OS::Wait4Finish );
    if ( !dir.isEmpty() )
	pars.workingdir( dir );

    return mc.execute( pars );
}


static BufferString getTmpFnm( const char* op, const char* fnm )
{
    return FilePath( File::getTempPath(),
		     BufferString(fnm,GetPID(),op)).fullPath();
}


SVNAccess::SVNAccess( const char* dir )
    : havesvn_(File::exists( FilePath(dir,".svn").fullPath()) )
    , dir_(dir)
{
    if ( !havesvn_ )
	return;

    const BufferString fnm( FilePath(dir,".svn","entries").fullPath() );
    od_istream strm( FilePath(dir,".svn","entries") );
    if ( !strm.isOK() )

    for ( int idx=0; idx<5; idx++ )
	if ( !strm.skipLine() )
	    return;

    BufferString reposinfo;
    if ( strm.getLine(reposinfo) )
    {
	char* reposnm = reposinfo.find( '/' );
	if ( reposnm ) *reposnm++ = '\0';
	const_cast<BufferString&>(reposdir_) = reposnm;
	const char* hostnm = firstOcc( reposinfo.buf(), "//" );
	if ( hostnm ) hostnm++;
	const_cast<BufferString&>(host_) = hostnm;
    }
}


SVNAccess::~SVNAccess()
{
}


bool SVNAccess::isOK() const
{
    OS::MachineCommand machcomm( "svn", "info" );
    return havesvn_ && executeCommand( machcomm, dir_ );
}


bool SVNAccess::isInSVN( const char* fnm ) const
{
    if ( !fnm || !*fnm || !isOK() )
	return false;

    FilePath fp( fnm );
    BufferString dirnm( fp.pathOnly() );

    OS::MachineCommand machcomm( "svn", "status", fp.fileName() );
    return executeCommand( machcomm, dir_ );
    // result starts with ?, return false;
}


bool SVNAccess::update( const char* fnm )
{
    if ( !isOK() )
	return true;

    const BufferString reqfnm( FilePath(dir_,fnm).fullPath() );
    OS::MachineCommand machcomm( "svn", "update", reqfnm );
    return executeCommand( machcomm, dir_ );
}


bool SVNAccess::lock( const char* fnm )
{
    BufferStringSet bss( fnm );
    return lock( bss );
}


bool SVNAccess::lock( const BufferStringSet& fnms )
{
    if ( !isOK() )
	return true;

    OS::MachineCommand machcomm( "svn", "edit" );
    for ( int idx=0; idx<fnms.size(); idx++ )
    {
	const char* fnm = fnms.get(idx).buf();
	const BufferString reqfnm( FilePath(dir_,fnm).fullPath() );
	machcomm.addArg( reqfnm );
    }

    return executeCommand( machcomm, dir_ );
}


bool SVNAccess::add( const char* fnm )
{
    BufferStringSet bss( fnm );
    return add( bss );
}


bool SVNAccess::add( const BufferStringSet& fnms )
{
    if ( fnms.isEmpty() )
	return true;
    if ( !isOK() )
	return false;

    OS::MachineCommand machcomm( "svn", "add" );
    for ( int idx=0; idx<fnms.size(); idx++ )
    {
	const char* fnm = fnms.get(idx).buf();
	const BufferString reqfnm( FilePath(dir_,fnm).fullPath() );
	machcomm.addArg( reqfnm );
    }

    return executeCommand( machcomm, dir_ );
}


bool SVNAccess::remove( const char* fnm )
{
    if ( !isInSVN(fnm) )
	return File::remove(fnm);

    BufferStringSet bss( fnm );
    return remove( bss );
}


bool SVNAccess::remove( const BufferStringSet& fnms )
{
    if ( fnms.isEmpty() )
	return true;

    const bool isok = isOK();

    OS::MachineCommand machcomm( "svn", "delete" );
    for ( int idx=0; idx<fnms.size(); idx++ )
    {
	const char* fnm = fnms.get(idx).buf();
	if ( !isok )
	    File::remove( fnm );
	else
	    machcomm.addArg( fnm );
    }
    if ( !isok )
	return true;

    return executeCommand( machcomm, dir_ );
}


bool SVNAccess::commit( const char* fnm, const char* msg )
{
    BufferStringSet bss( fnm );
    return commit( bss, msg );
}


bool SVNAccess::commit( const BufferStringSet& fnms, const char* msg )
{
    if ( fnms.isEmpty() || !isOK() )
	return true;

    OS::MachineCommand machcomm( "svn", "commit" );
    const BufferString tmpfnm = getTmpFnm( "svncommit", fnms.get(0) );
    bool havetmpfile = false;
    if ( !msg || !*msg )
	machcomm.addArg( "-m" ).addArg( "update" );
    else
    {
	od_ostream strm( tmpfnm );
	if ( strm.isOK() )
	{
	    strm << msg << od_endl;
	    havetmpfile = true;
	    machcomm.addArg( "-F" ).addArg( tmpfnm );
	}
    }

    for ( int idx=0; idx<fnms.size(); idx++ )
    {
	const char* fnm = fnms.get(idx).buf();
	if ( fnm && *fnm )
	{
	    const BufferString reqfnm( FilePath(dir_,fnm).fullPath() );
	    machcomm.addArg( reqfnm );
	}
    }

    const bool res = executeCommand( machcomm, dir_ );
    if ( havetmpfile )
	File::remove( tmpfnm );
    return res;
}


bool SVNAccess::rename( const char* subdir, const char* from, const char* to )
{
    const FilePath sdfp( dir_, subdir );
    FilePath fromfp( sdfp, from ), tofp( sdfp, to );
    const BufferString fromfullfnm( fromfp.fullPath() );
    const BufferString tofullfnm( tofp.fullPath() );
    fromfp.set( subdir );
    fromfp.add( from );
    tofp.set( subdir );
    tofp.add( to );

    OS::MachineCommand machcomm( "svn", "rename" );
    machcomm.addArg( fromfp.fullPath() )
	    .addArg( tofp.fullPath() ).addArg( "." );
    return executeCommand( machcomm, dir_ );
}


bool SVNAccess::changeFolder( const char* fnm, const char* fromsubdir,
			      const char* tosubdir )
{
    FilePath tofp( dir_, tosubdir, fnm );
    FilePath fromfp( dir_, fromsubdir, fnm );
    const BufferString fromfullfnm( fromfp.fullPath() );
    const BufferString tofullfnm( tofp.fullPath() );
    fromfp.set( fromsubdir );
    fromfp.add( fnm );
    tofp.set( tosubdir );
    tofp.add( fnm );
    const BufferString fromfnm( fromfp.fullPath() );
    const BufferString tofnm( tofp.fullPath() );

    OS::MachineCommand machcomm( "svn", "move" );
    machcomm.addArg( fromfp.fullPath() )
	    .addArg( tofp.fullPath() ).addArg( "." );
    return executeCommand( machcomm, dir_ );
}


void SVNAccess::diff( const char* fnm, BufferString& res ) const
{
    res.setEmpty();
    if ( !isOK() )
	return;

    const BufferString reqfnm( FilePath(dir_,fnm).fullPath() );

    if ( __iswin__ )
	return; //TODO

    const BufferString tmpfnm = getTmpFnm( "svndiff", fnm );
    OS::MachineCommand machcomm( "svn", "diff", reqfnm );
    machcomm.addFileRedirect( tmpfnm );
    const bool commres = executeCommand( machcomm, dir_ );
    if ( !commres )
    {
	if ( File::exists(tmpfnm) )
	    File::remove(tmpfnm);
	return;
    }

    od_istream strm( tmpfnm );
    if ( !strm.isOK() )
    {
	if ( File::exists(tmpfnm) )
	    File::remove(tmpfnm);
	return;
    }

    BufferString line;
    while ( strm.getLine(line) )
    {
	if ( line.isEmpty() )
	    continue;
	if ( !res.isEmpty() )
	    res.add( "\n" );
	res.add( line );
    }

    if ( File::exists(tmpfnm) )
	File::remove(tmpfnm);
}
