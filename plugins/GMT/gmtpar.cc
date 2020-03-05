/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Raman Singh
 Date:          July 2008
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";


#include "gmtpar.h"

#include "debug.h"
#include "envvars.h"
#include "initgmtplugin.h"
#include "keystrs.h"
#include "oddirs.h"
#include "od_istream.h"
#include "strmprov.h"


GMTParFactory& GMTPF()
{
    mDefineStaticLocalObject( GMTParFactory, inst, );
    return inst;
}


int GMTParFactory::add( const char* nm, GMTParCreateFunc fn )
{
    Entry* entry = getEntry( nm );
    if ( entry )
	entry->crfn_ = fn;
    else
    {
	entry = new Entry( nm, fn );
	entries_ += entry;
    }

    return entries_.size() - 1;
}


GMTPar* GMTParFactory::create( const IOPar& iop ) const
{
    const char* grpname = iop.find( ODGMT::sKeyGroupName() );
    if ( !grpname || !*grpname ) return 0;

    Entry* entry = getEntry( grpname );
    if ( !entry ) return 0;

    GMTPar* grp = entry->crfn_( iop );
    return grp;
}


const char* GMTParFactory::name( int idx ) const
{
    if ( idx < 0 || idx >= entries_.size() )
	return 0;

    return entries_[idx]->name_.buf();
}


GMTParFactory::Entry* GMTParFactory::getEntry( const char* nm ) const
{
    for ( int idx=0; idx<entries_.size(); idx++ )
    {
	if ( entries_[idx]->name_ == nm )
	    return const_cast<GMTParFactory*>( this )->entries_[idx];
    }

    return 0;
}


BufferString GMTPar::fileName( const char* fnm ) const
{
    BufferString fnmchg;
    if ( __iswin__ ) fnmchg += "\"";
    fnmchg += fnm;
    if ( __iswin__ ) fnmchg += "\"";
    return fnmchg;
}


bool GMTPar::execCmd( const BufferString& comm, od_ostream& strm )
{
    BufferString cmd;
    const char* errfilenm = GetProcFileName( "gmterr.err" );
    const char* shellnm = GetOSEnvVar( "SHELL" );
    const bool needsbash = shellnm && *shellnm && !firstOcc(shellnm,"bash");
    if ( needsbash )
	cmd += "bash -c \'";

    const char* commstr = comm.buf();
    if ( commstr && commstr[0] == '@' )
	commstr++;

    addWrapperComm( cmd );
    cmd += commstr;
    cmd += " 2> \"";
    cmd += errfilenm;
    cmd += "\"";
    if ( needsbash )
	cmd += "\'";

    DBG::message( DBG_PROGSTART, cmd );
    if ( system(cmd) )
    {
	od_istream errstrm( errfilenm );
	if ( !errstrm.isOK() )
	    return true;

	BufferString buf;
	strm << od_endl;
	while ( errstrm.getLine(buf) )
	    strm << buf << od_endl;
    }

    return true;
}


od_ostream GMTPar::makeOStream( const BufferString& comm, od_ostream& strm )
{
    BufferString cmd;
    const char* errfilenm = GetProcFileName( "gmterr.err" );
    const char* shellnm = GetOSEnvVar( "SHELL" );
    const bool needsbash = shellnm && *shellnm && !firstOcc(shellnm,"bash");
    const char* commptr = comm.buf();
	if (needsbash)
		cmd += "@bash -c \'";
	else
		cmd += "@";

    addWrapperComm( cmd );
	if ( !cmd.isEmpty() && commptr[0] == '@')
		commptr++;

    cmd += commptr;
    cmd += " 2> \"";
    cmd += errfilenm;
    cmd += "\"";
    if ( needsbash )
	cmd += "\'";

    DBG::message( DBG_PROGSTART, cmd );
    od_ostream ret( cmd );
    if ( !ret.isOK() )
    {
	od_istream errstrm( errfilenm );
	if ( !errstrm.isOK() )
	    return ret;

	BufferString buf;
	strm << od_endl;
	while ( errstrm.getLine(buf) )
	    strm << buf << od_endl;
    }

    return ret;
}


void GMTPar::addWrapperComm( BufferString& comm )
{
    const char* wrapper = GMT::sKeyDefaultExec();
    if ( !wrapper )
	return;

    comm.add( wrapper ).addSpace();
}
