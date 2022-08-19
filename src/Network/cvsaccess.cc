/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "cvsaccess.h"
#include "filepath.h"
#include "file.h"
#include "genc.h"
#include "od_iostream.h"
#include "oscommand.h"


static void addRedirect( OS::MachineCommand& mc )
{
#ifndef __debug__
    mc.addFileRedirect( "/dev/null", 3 );
#endif
}

#define mExecCmd( stmt, machcomm ) \
    addRedirect( machcomm ); \
    OS::CommandExecPars pars( OS::Wait4Finish ); \
    if ( !dir_.isEmpty() ) \
	pars.workingdir( dir_ );  \
    stmt machcomm.execute( pars )
#define mRetExecCmd( machcomm ) mExecCmd( return, machcomm )

#define mGetTmpFnm(op,fnm) \
    const BufferString tmpfnm( \
	FilePath(File::getTempPath(),BufferString(fnm,GetPID(),op)).fullPath() )

#define mRetRmTempFile() \
{ if ( File::exists(tmpfnm) ) File::remove(tmpfnm); return; }


static BufferString getHost( const char* dir )
{
    BufferString ret;

    od_istream strm( FilePath(dir,"CVS","Root") );
    if ( !strm.isOK() )
	return ret;

    BufferString line;
    strm.getLine( line );
    if ( line.isEmpty() )
	return ret;
    const char* atptr = firstOcc( line.buf(), '@' );
    if ( !atptr )
	return ret;

    ret = atptr + 1;
    char* cptr = ret.find( ':' );
    if ( cptr ) *cptr = '\0';
    return ret;
}


CVSAccess::CVSAccess( const char* dir )
    : dir_(dir)
    , host_(getHost(dir))
{
    if ( isOK() )
    {
	od_istream strm( FilePath(dir,"CVS","Repository") );
	if ( strm.isOK() )
	    strm.getLine( reposdir_ );
    }
}


CVSAccess::~CVSAccess()
{
}


bool CVSAccess::hostOK() const
{
#ifdef __win__
    return true; //TODO
#else
    OS::MachineCommand machcomm( "ping", "-q", "-c", "1", "-W", "2" );
    machcomm.addArg( host_ );
    mRetExecCmd( machcomm );
#endif
}


bool CVSAccess::isInCVS( const char* fnm ) const
{
    if ( !fnm || !*fnm || !isOK() )
	return false;

    FilePath fp( fnm );
    BufferString dirnm( fp.pathOnly() );
    BufferStringSet entries; getEntries( dirnm, entries );
    return entries.isPresent( fp.fileName() );
}


void CVSAccess::getEntries( const char* dir, BufferStringSet& entries ) const
{
    entries.erase();

    FilePath fp( dir, "CVS", "Entries" );
    if ( !fp.isAbsolute() )
	fp = FilePath( dir_, fp.fullPath() );

    od_istream strm( fp );
    if ( !strm.isOK() )
	return;

    BufferString line;
    while ( strm.getLine(line) )
    {
	char* nmptr = line.find( '/' );
	if ( !nmptr ) continue;
	nmptr++; char* endptr = firstOcc( nmptr, '/' );
	if ( endptr ) *endptr = '\0';
	if ( *nmptr )
	    entries.add( nmptr );
    }
}


bool CVSAccess::update( const char* fnm )
{
    if ( !isOK() )
	return true;

    OS::MachineCommand machcomm( "cvs", "update", fnm );
    mRetExecCmd( machcomm );
}


bool CVSAccess::edit( const char* fnm )
{
    BufferStringSet bss; bss.add( fnm );
    return edit( bss );
}


bool CVSAccess::edit( const BufferStringSet& fnms )
{
    if ( !isOK() )
	return true;

    OS::MachineCommand machcomm( "cvs", "edit" );
    for ( int idx=0; idx<fnms.size(); idx++ )
    {
	const char* fnm = fnms.get(idx).buf();
	machcomm.addArg( fnm );
    }

    mRetExecCmd( machcomm );
}


bool CVSAccess::add( const char* fnm, bool bin )
{
    BufferStringSet bss; bss.add( fnm );
    return add( bss, bin );
}


bool CVSAccess::add( const BufferStringSet& fnms, bool bin )
{
    if ( fnms.isEmpty() )
	return true;
    else if ( !isOK() )
	return false;

    OS::MachineCommand machcomm( "cvs", "add" );
    if ( bin )
	machcomm.addArg( "-kb" );
    for ( int idx=0; idx<fnms.size(); idx++ )
    {
	const char* fnm = fnms.get(idx).buf();
	machcomm.addArg( fnm );
    }

    mRetExecCmd( machcomm );
}


bool CVSAccess::remove( const char* fnm )
{
    if ( !isInCVS(fnm) )
	return File::remove(fnm);

    BufferStringSet bss; bss.add( fnm );
    return remove( bss );
}


bool CVSAccess::remove( const BufferStringSet& fnms )
{
    if ( fnms.isEmpty() )
	return true;
    const bool isok = isOK();

    OS::MachineCommand machcomm( "cvs", "delete", "-f" );
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

    mRetExecCmd( machcomm );
}


bool CVSAccess::commit( const char* fnm, const char* msg )
{
    BufferStringSet bss; bss.add( fnm );
    return commit( bss, msg );
}


bool CVSAccess::commit( const BufferStringSet& fnms, const char* msg )
{
    if ( fnms.isEmpty() || !isOK() )
	return true;

    OS::MachineCommand machcomm( "cvs", "commit" );
    mGetTmpFnm("cvscommit",fnms.get(0));
    bool havetmpfile = false;
    if ( !msg || !*msg )
	machcomm.addArg( "-m" ).addArg( "." );
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
	    machcomm.addArg( fnm );
    }

    mExecCmd( const bool res =, machcomm );
    if ( havetmpfile )
	File::remove( tmpfnm );
    return res;
}


bool CVSAccess::rename( const char* subdir, const char* from, const char* to )
{
    const FilePath sdfp( dir_, subdir );
    FilePath fromfp( sdfp, from ), tofp( sdfp, to );
    const BufferString fromfullfnm( fromfp.fullPath() );
    const BufferString tofullfnm( tofp.fullPath() );
    fromfp.set( subdir ); fromfp.add( from );
    tofp.set( subdir ); tofp.add( to );
    const BufferString fromfnm( fromfp.fullPath() );
    const BufferString tofnm( tofp.fullPath() );
    return doRename( fromfullfnm, tofullfnm, fromfnm, tofnm );
}


bool CVSAccess::changeFolder( const char* fnm, const char* fromsubdir,
			      const char* tosubdir )
{
    FilePath tofp( dir_, tosubdir, fnm );
    FilePath fromfp( dir_, fromsubdir, fnm );
    const BufferString fromfullfnm( fromfp.fullPath() );
    const BufferString tofullfnm( tofp.fullPath() );
    fromfp.set( fromsubdir ); fromfp.add( fnm );
    tofp.set( tosubdir ); tofp.add( fnm );
    const BufferString fromfnm( fromfp.fullPath() );
    const BufferString tofnm( tofp.fullPath() );

    return doRename( fromfullfnm, tofullfnm, fromfnm, tofnm );
}


bool CVSAccess::doRename( const char* fromfullfnm, const char* tofullfnm,
			  const char* fromfnm, const char* tofnm )
{
    if ( !File::exists(fromfullfnm) )
	return false;
    if ( File::exists(tofullfnm) && !File::remove(tofullfnm) )
	return false;
    if ( !File::rename(fromfullfnm,tofullfnm) )
	return false;
    if ( !remove(fromfnm) )
	return false;
    return isInCVS(tofnm) ? true : add( tofnm );
}


void CVSAccess::getEditTxts( const char* fnm, BufferStringSet& edtxts ) const
{
    edtxts.erase();
    if ( !isOK() )
	return;

#ifdef __win__
    return; //TODO
#else

    mGetTmpFnm("cvseditors",fnm);
    OS::MachineCommand machcomm( "cvs", "editors", fnm );
    machcomm.addFileRedirect( tmpfnm )
	    .addFileRedirect( "/dev/null", 2 );
    mExecCmd( const bool res =, machcomm );
    if ( !res )
	mRetRmTempFile()

    od_istream strm( tmpfnm );
    if ( !strm.isOK() )
	mRetRmTempFile()

    BufferString line;
    while ( strm.getLine(line) )
	if ( !line.isEmpty() )
	    edtxts.add( line );

    mRetRmTempFile()
#endif
}


void CVSAccess::diff( const char* fnm, BufferString& res ) const
{
    res.setEmpty();
    if ( !isOK() )
	return;

#ifdef __win__
    return; //TODO
#else

    mGetTmpFnm("cvsdiff",fnm);
    OS::MachineCommand machcomm( "cvs", "diff", fnm );
    machcomm.addFileRedirect( tmpfnm )
	    .addFileRedirect( "/dev/null", 2 );
    mExecCmd( const bool commres =, machcomm );
    if ( !commres )
	mRetRmTempFile()

    od_istream strm( tmpfnm );
    if ( !strm.isOK() )
	mRetRmTempFile()

    BufferString line;
    while ( strm.getLine(line) )
    {
	if ( line.isEmpty() )
	    continue;
	if ( !res.isEmpty() )
	    res.add( "\n" );
	res.add( line );
    }

    mRetRmTempFile()
#endif
}
