/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          March 2004
 RCS:           $Id: mpesetup.cc,v 1.1 2005-03-11 16:55:15 cvsnanne Exp $
________________________________________________________________________

-*/

#include "mpesetup.h"
#include "mpefact.h"
#include "ptrman.h"
#include "ioobj.h"
#include "ioparlist.h"
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

int MPESetupTranslatorGroup::selector( const char* key )
{
    int retval = defaultSelector( theInst().userName(), key );
    if ( retval ) return retval;

    if ( defaultSelector(MPESetupTranslator::keyword,key) ) return 1;
    return 0;
}


const IOObjContext& MPESetupTranslatorGroup::ioContext()
{
    static IOObjContext* ctxt = 0;
    
    if ( !ctxt )
    {
	ctxt = new IOObjContext( &theInst() );
	ctxt->crlink = false;
	ctxt->newonlevel = 1;
	ctxt->needparent = false;
	ctxt->maychdir = false;
	ctxt->stdseltype = IOObjContext::Mdl;
    }

    return *ctxt;
}


// MPESetupTranslator ---------------------------------------------------- 

const char* MPESetupTranslator::keyword = "MPE setup";

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
    bool rv = err == "";
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
    bool rv = err == "";
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
    if ( !iopar.size() )
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

    IOPar* iopar = new IOPar( MPESetupTranslator::keyword );
    setup.fillPar( *iopar );
    IOParList iopl( mTranslGroupName(MPESetup) );
    iopl.deepErase(); // Protection is necessary!
    iopl += iopar;
    if ( !iopl.write(((StreamConn&)conn).oStream()) )
	return "Cannot write setup to file";
    return 0;
}
