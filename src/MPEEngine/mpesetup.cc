/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          March 2004
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id$";

#include "mpesetup.h"
#include "mpefact.h"
#include "ptrman.h"
#include "ioobj.h"
#include "iopar.h"
#include "ascstream.h"


namespace MPE {

Setup::Setup()
    : pars(*new IOPar)
{}


Setup::~Setup()
{
    delete &pars;
}


bool Setup::usePar( const IOPar& par )
{
    pars = par;
    return true;
}


void Setup::fillPar( IOPar& par ) const
{
    par = pars;
}


}; //namespace MPE



// MPESetupTranslatorGroup --------------------------------------------------

mDefSimpleTranslatorSelector(MPESetup,MPESetupTranslator::keyword)
mDefSimpleTranslatorioContext(MPESetup,Mdl)


// MPESetupTranslator ---------------------------------------------------- 

const char* MPESetupTranslator::keyword = "Tracking setup";

bool MPESetupTranslator::retrieve( MPESetup& setup, const IOObj* ioobj,
				   BufferString& err )
{
    if ( !ioobj ) { err = "Cannot find object in data base"; return false; }

    PtrMan<MPESetupTranslator> tr =
		dynamic_cast<MPESetupTranslator*>(ioobj->getTranslator());
    if ( !tr ) { err = "Selected object is not a Setup"; return false; }

    PtrMan<Conn> conn = ioobj->getConn( Conn::Read );
    if ( !conn )
    {
	err = "Cannot open "; err += ioobj->fullUserExpr(true); 
	return false;
    }

    err = tr->read( setup, *conn );
    bool rv = err.isEmpty();
    if ( rv ) err = tr->warningMsg();
    return rv;
}


bool MPESetupTranslator::store( const MPESetup& setup, const IOObj* ioobj,
				BufferString& err )
{
    if ( !ioobj ) { err = "No object to store in data base"; return false; }

    PtrMan<MPESetupTranslator> tr =
		dynamic_cast<MPESetupTranslator*>(ioobj->getTranslator());
    if ( !tr ) { err = "Selected object is not a Setup"; return false; }

    PtrMan<Conn> conn = ioobj->getConn( Conn::Write );
    if ( !conn )
    {
	err = "Cannot open "; err += ioobj->fullUserExpr(false); 
	return false;
    }

    err = tr->write( setup, *conn );
    bool rv = err.isEmpty();
    if ( rv ) err = tr->warningMsg();
    return rv;
}


const char* dgbMPESetupTranslator::read( MPESetup& setup, Conn& conn )
{
    warningmsg = "";
    if ( !conn.forRead() || !conn.isStream() )
	return "Internal error: bad connection";

    ascistream astream( ((StreamConn&)conn).iStream() );

    IOPar iopar( astream );
    if ( iopar.isEmpty() )
	return "Empty input file";

    if ( !setup.usePar( iopar ))
	return "Could not read setup-file";

    return 0;
}


const char* dgbMPESetupTranslator::write( const MPESetup& setup, Conn& conn )
{
    warningmsg = "";
    if ( !conn.forWrite() || !conn.isStream() )
	return "Internal error: bad connection";

    IOPar iop; setup.fillPar( iop );
    if ( !iop.write(((StreamConn&)conn).oStream(),mTranslGroupName(MPESetup)) )
	return "Cannot write setup to file";
    return 0;
}
