/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Raman Singh
 Date:          July 2008
________________________________________________________________________

-*/


#include "gmtpar.h"

#include "debug.h"
#include "envvars.h"
#include "file.h"
#include "filepath.h"
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


GMTPar* GMTParFactory::create( const IOPar& iop, const char* workdir ) const
{
    const char* grpname = iop.find( ODGMT::sKeyGroupName() );
    if ( !grpname || !*grpname ) return 0;

    Entry* entry = getEntry( grpname );
    if ( !entry ) return 0;

    GMTPar* grp = entry->crfn_( iop, workdir );
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


bool GMTPar::execute( od_ostream& strm, const char* fnm )
{
    const bool res = doExecute( strm, fnm );
    const BufferString gmterrfnm( getErrFnm() );
    checkErrStrm( gmterrfnm, strm );
    File::remove( gmterrfnm );

    const FilePath historyfp( workingdir_,
	    GMT::hasModernGMT() ? "gmt.history" : ".gmtcommands4" );
    File::remove( historyfp.fullPath() );

    return res;
}


bool GMTPar::execCmd( const OS::MachineCommand& machcomm, od_ostream& strm,
		      const char* fnm, bool append )
{
    OS::MachineCommand mc = getWrappedComm( machcomm );
    if ( fnm )
	mc.addFileRedirect( fnm, 1, append );
    const BufferString gmterrfnm( getErrFnm() );
    mc.addFileRedirect( gmterrfnm, 2 );

    OS::CommandExecPars pars( OS::Wait4Finish );
    pars.workingdir( workingdir_ );

    DBG::message( DBG_PROGSTART, mc.toString(&pars) );
    if ( mc.execute(pars) )
	checkErrStrm( gmterrfnm, strm );

    return true;
}


od_ostream GMTPar::makeOStream( const OS::MachineCommand& machcomm,
				od_ostream& strm,
				const char* fnm, bool append )
{
    OS::MachineCommand mc = getWrappedComm( machcomm );
    if ( fnm )
	mc.addFileRedirect( fnm, 1, append );
    const BufferString gmterrfnm( getErrFnm() );
    mc.addFileRedirect( gmterrfnm, 2 );

    DBG::message( DBG_PROGSTART, mc.toString() );
    od_ostream ret( mc, workingdir_ );
    checkErrStrm( gmterrfnm, strm );

    return ret;
}


BufferString GMTPar::getErrFnm()
{
    return BufferString ( GetProcFileName("gmterr.err") );
}


void GMTPar::checkErrStrm( const char* gmterrfnm, od_ostream& strm )
{
    od_istream errstrm( gmterrfnm );
    if ( !errstrm.isOK() )
	return;

    BufferString buf;
    bool first = true;
    while ( errstrm.getLine(buf) )
    {
	if ( first )
	{
	    strm << od_endl;
	    first = false;
	}

	strm << buf << od_endl;
    }
}


OS::MachineCommand GMTPar::getWrappedComm( const OS::MachineCommand& mc )
{
    OS::MachineCommand ret( GMT::sKeyDefaultExec() );
    if ( ret.isBad() )
	return ret;

    ret.addArg( mc.program() );
    ret.addArgs( mc.args() );
    return ret;
}
