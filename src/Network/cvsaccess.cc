/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Dec 2011
________________________________________________________________________

-*/
static const char* rcsID = "$Id: cvsaccess.cc,v 1.2 2011-12-12 13:22:22 cvsbert Exp $";

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


bool CVSAccess::update( const char* fnm )
{
    mGetReqFnm();
    const BufferString cmd( "@cvs update ", reqfnm, sRedirect );
    return StreamProvider(cmd).executeCommand();
}


bool CVSAccess::edit( const char* fnm )
{
    mGetReqFnm();
    const BufferString cmd( "@cvs edit ", reqfnm, sRedirect );
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

    BufferString cmd( "cvs commit" );
    for ( int idx=0; idx<fnms.size(); idx++ )
    {
	const char* fnm = fnms.get(idx).buf();
	mGetReqFnm();
	cmd.add( " \"" ).add( reqfnm ).add( "\"" );
    }

    mGetTmpFnm("cvscommit",fnms.get(0));
    bool havetmpfile = false;
    if ( msg && *msg )
    {
	StreamData sd( StreamProvider(tmpfnm).makeOStream() );
	if ( sd.usable() )
	{
	    *sd.ostrm << msg; sd.close();
	    havetmpfile = true;
	    cmd.add( " -F \"" ).add( tmpfnm ).add( "\"" );
	}
    }

    cmd.add( sRedirect );
    const bool res = StreamProvider(cmd).executeCommand();
    if ( havetmpfile )
	File::remove( tmpfnm );
    return res;
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
    {
	if ( line.isEmpty() )
	    continue;
	edtxts.add( line );
    }

    mRetRmTempFile()
#endif
}
