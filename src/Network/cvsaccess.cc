/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Dec 2011
________________________________________________________________________

-*/
static const char* rcsID = "$Id: cvsaccess.cc,v 1.8 2011-12-14 08:17:27 cvsbert Exp $";

#include "cvsaccess.h"
#include "filepath.h"
#include "file.h"
#include "strmprov.h"
#include "strmoper.h"

static const char* sRedirect = " > /dev/null 2>&1";

#define mGetReqFnm() \
    FilePath inpfp( dir_ ); \
    if ( fnm && *fnm ) inpfp.add( fnm ); \
    const BufferString reqfnm( inpfp.fullPath() )

#define mGetTmpFnm(op,fnm) \
    FilePath fptmp( File::getTempPath(), BufferString(fnm,GetPID(),op) ); \
    const BufferString tmpfnm( fptmp.fullPath() )

#define mRetRmTempFile() \
{ if ( File::exists(tmpfnm) ) File::remove(tmpfnm); return; }


static BufferString getHost( const char* dir )
{
    BufferString ret;

    FilePath fp( dir, "CVS", "Root" );
    StreamData sd( StreamProvider(fp.fullPath()).makeIStream() );
    if ( !sd.usable() )
	return ret;

    BufferString line;
    StrmOper::readLine( *sd.istrm, &line );
    sd.close();
    if ( line.isEmpty() )
	return ret;
    const char* atptr = strchr( line.buf(), '@' );
    if ( !atptr )
	return ret;

    ret = atptr + 1;
    char* cptr = strchr( ret.buf(), ':' );
    if ( cptr ) *cptr = '\0';
    return ret;
}


CVSAccess::CVSAccess( const char* dir )
    : dir_(dir)
    , host_(getHost(dir))
{
    if ( isOK() )
    {
	FilePath fp( dir_, "CVS", "Repository" );
	StreamData sd( StreamProvider(fp.fullPath()).makeIStream() );
	if ( sd.usable() )
	    StrmOper::readLine(*sd.istrm,&reposdir_);
	sd.close();
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
    const BufferString cmd( "@ping -q -c 1 -W 2 ", host_, sRedirect );
    return StreamProvider(cmd).executeCommand();
#endif
}


bool CVSAccess::isInCVS( const char* fnm ) const
{
    if ( !fnm || !*fnm ) return false;
    FilePath fp( fnm );
    BufferString dirnm( fp.pathOnly() );
    BufferStringSet entries; getEntries( dirnm, entries );
    return entries.isPresent( fp.fileName() );
}


void CVSAccess::getEntries( const char* dir, BufferStringSet& entries ) const
{
    entries.erase();
    FilePath fp( dir_ ); if ( dir && *dir ) fp.add( dir );
    fp.add( "CVS" ).add( "Entries" );
    StreamData sd( StreamProvider(fp.fullPath()).makeIStream() );
    if ( !sd.usable() )
	return;

    BufferString line;
    while ( StrmOper::readLine(*sd.istrm,&line) )
    {
	char* nmptr = strchr( line.buf(), '/' );
	if ( !nmptr ) continue;
	nmptr++; char* endptr = strchr( nmptr, '/' );
	if ( endptr ) *endptr = '\0';
	if ( *nmptr )
	    entries.add( nmptr );
    }
}


bool CVSAccess::update( const char* fnm )
{
    mGetReqFnm();
    const BufferString cmd( "@cvs update ", reqfnm, sRedirect );
    return StreamProvider(cmd).executeCommand();
}


bool CVSAccess::edit( const char* fnm )
{
    BufferStringSet bss; bss.add( fnm );
    return edit( bss );
}


bool CVSAccess::edit( const BufferStringSet& fnms )
{
    BufferString cmd( "@cvs edit " );
    for ( int idx=0; idx<fnms.size(); idx++ )
    {
	const char* fnm = fnms.get(idx).buf();
	mGetReqFnm();
	cmd.add( " \"" ).add( reqfnm ).add( "\"" );
    }
    cmd.add( sRedirect );
    return StreamProvider(cmd).executeCommand();
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

    BufferString cmd( "@cvs add" );
    if ( bin ) cmd.add( " -kb" );
    for ( int idx=0; idx<fnms.size(); idx++ )
    {
	const char* fnm = fnms.get(idx).buf();
	mGetReqFnm();
	cmd.add( " \"" ).add( reqfnm ).add( "\"" );
    }
    cmd.add( sRedirect );
    return StreamProvider(cmd).executeCommand();
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

    BufferString cmd( "@cvs delete -f" );
    for ( int idx=0; idx<fnms.size(); idx++ )
    {
	const char* fnm = fnms.get(idx).buf();
	mGetReqFnm();
	cmd.add( " \"" ).add( reqfnm ).add( "\"" );
    }
    cmd.add( sRedirect );
    return StreamProvider(cmd).executeCommand();
}


bool CVSAccess::commit( const char* fnm, const char* msg )
{
    BufferStringSet bss; bss.add( fnm );
    return commit( bss, msg );
}


bool CVSAccess::commit( const BufferStringSet& fnms, const char* msg )
{
    if ( fnms.isEmpty() )
	return true;

    BufferString cmd( "@cvs commit" );
    mGetTmpFnm("cvscommit",fnms.get(0));
    bool havetmpfile = false;
    if ( !msg || !*msg )
	cmd.add( " -m \".\"" );
    else
    {
	StreamData sd( StreamProvider(tmpfnm).makeOStream() );
	if ( sd.usable() )
	{
	    *sd.ostrm << msg; sd.close();
	    havetmpfile = true;
	    cmd.add( " -F \"" ).add( tmpfnm ).add( "\"" );
	}
    }

    for ( int idx=0; idx<fnms.size(); idx++ )
    {
	const char* fnm = fnms.get(idx).buf();
	if ( fnm && *fnm )
	{
	    mGetReqFnm();
	    cmd.add( " \"" ).add( reqfnm ).add( "\"" );
	}
    }

    cmd.add( sRedirect );
    const bool res = StreamProvider(cmd).executeCommand();
    if ( havetmpfile )
	File::remove( tmpfnm );
    return res;
}


bool CVSAccess::rename( const char* subdir, const char* from, const char* to )
{
    FilePath sdfp( dir_ ); if ( subdir && *subdir ) sdfp.add( subdir );
    FilePath fromfp( sdfp ); fromfp.add( from );
    FilePath tofp( sdfp ); tofp.add( to );
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
    mGetReqFnm();
    edtxts.erase();

#ifdef __win__
    return; //TODO
#else

    BufferString cmd( "@cvs editors ", reqfnm, " > " );
    mGetTmpFnm("cvseditors",fnm);
    cmd.add( "\"" ).add( tmpfnm ).add( "\" 2> /dev/null" );
    if ( !StreamProvider(cmd).executeCommand() )
	mRetRmTempFile()

    StreamData sd( StreamProvider(tmpfnm).makeIStream() );
    if ( !sd.usable() )
	mRetRmTempFile()

    BufferString line;
    while ( StrmOper::readLine(*sd.istrm,&line) )
	if ( !line.isEmpty() )
	    edtxts.add( line );

    mRetRmTempFile()
#endif
}


void CVSAccess::diff( const char* fnm, BufferString& res ) const
{
    mGetReqFnm();
    res.setEmpty();

#ifdef __win__
    return; //TODO
#else

    BufferString cmd( "@cvs diff ", reqfnm, " > " );
    mGetTmpFnm("cvsdiff",fnm);
    cmd.add( "\"" ).add( tmpfnm ).add( "\" 2> /dev/null" );
    if ( !StreamProvider(cmd).executeCommand() )
	mRetRmTempFile()

    StreamData sd( StreamProvider(tmpfnm).makeIStream() );
    if ( !sd.usable() )
	mRetRmTempFile()

    BufferString line;
    while ( StrmOper::readLine(*sd.istrm,&line) )
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

