/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Dec 2011
________________________________________________________________________

-*/
static const char* rcsID = "$Id: cvsaccess.cc,v 1.6 2011-12-13 13:54:50 cvsbert Exp $";

#include "cvsaccess.h"
#include "filepath.h"
#include "file.h"
#include "strmprov.h"
#include "strmoper.h"


static BufferString getHost( const char* dir )
{
    BufferString ret;

    FilePath fp( dir ); fp.add( "CVS" ).add( "Root" );
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
    , serverdir_("/cvsroot")
{
}


CVSAccess::~CVSAccess()
{
}


static const char* sRedirect = " > /dev/null 2>&1";
#define mGetReqFnm() \
    FilePath inpfp( dir_ ); \
    if ( fnm && *fnm ) inpfp.add( fnm ); \
    const BufferString reqfnm( inpfp.fullPath() )


bool CVSAccess::hostOK() const
{
#ifdef __win__
    return true; //TODO
#else
    const BufferString cmd( "@ping -q -c 1 -W 2 ", host_, sRedirect );
    return StreamProvider(cmd).executeCommand();
#endif
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


bool CVSAccess::commit( const char* msg )
{
    BufferStringSet bss; bss.add( "" );
    return commit( bss, msg );
}


#define mGetTmpFnm(op,fnm) \
    FilePath fptmp( File::getTempPath() ); \
    fptmp.add( BufferString(fnm,GetPID(),op) ); \
    const BufferString tmpfnm( fptmp.fullPath() )


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
    BufferString cmd( "@ssh ", host_, "mv " );
    BufferString servdir( serverdir_, subdir && *subdir ? "/" : "", subdir );
    servdir.add( "/" );
    cmd.add( BufferString(servdir,from," ") ).add( BufferString(servdir,to) ); 
    cmd.add( sRedirect );
    return StreamProvider(cmd).executeCommand();
}


#define mRetRmTempFile() \
{ if ( File::exists(tmpfnm) ) File::remove(tmpfnm); return; }


void CVSAccess::checkEdited( const char* fnm, BufferStringSet& edtxts )
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


void CVSAccess::diff( const char* fnm, BufferString& res )
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

