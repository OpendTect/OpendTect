/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          March 2004
________________________________________________________________________

-*/

#include "mpesetup.h"
#include "ptrman.h"
#include "ioobj.h"
#include "iopar.h"
#include "ascstream.h"
#include "uistrings.h"


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

defineTranslatorGroup(MPESetup,"Tracking setup");
defineTranslator(dgb,MPESetup,mDGBKey);
mDefSimpleTranslatorSelector(MPESetup)
mDefSimpleTranslatorioContext(MPESetup,Mdl)
uiString MPESetupTranslatorGroup::sTypeName( int num )
{ return tr("Tracking setup",0,num ); }


// MPESetupTranslator ----------------------------------------------------


bool MPESetupTranslator::retrieve( MPESetup& setup, const IOObj* ioobj,
				   BufferString& err )
{
    if ( !ioobj ) { err = "Cannot find object in data base"; return false; }

    PtrMan<MPESetupTranslator> tr =
		dynamic_cast<MPESetupTranslator*>(ioobj->createTranslator());
    if ( !tr ) { err = "Selected object is not a Setup"; return false; }

    PtrMan<Conn> conn = ioobj->getConn( Conn::Read );
    if ( !conn )
    {
	err = "Cannot open "; err += ioobj->fullUserExpr(true);
	return false;
    }

    err = mFromUiStringTodo(tr->read( setup, *conn ));
    bool rv = err.isEmpty();
    if ( rv ) err = mFromUiStringTodo(tr->warningMsg());
    return rv;
}


bool MPESetupTranslator::retrieve( MPESetup& setup, const IOObj* ioobj,
				   uiString& err )
{
    if ( !ioobj ) { err = uiStrings::phrCannotFind(tr("object in data base"));
								return false; }

    PtrMan<MPESetupTranslator> trnsltr =
		dynamic_cast<MPESetupTranslator*>(ioobj->createTranslator());
    if ( !trnsltr ) {err = tr("Selected object is not a Setup"); return false;}

    PtrMan<Conn> conn = ioobj->getConn( Conn::Read );
    if ( !conn )
	{ err = ioobj->phrCannotOpenObj(); return false; }

    err = toUiString(trnsltr->read( setup, *conn ));
    bool rv = err.isEmpty();
    if ( rv ) err = trnsltr->warningMsg();
    return rv;
}


bool MPESetupTranslator::store( const MPESetup& setup, const IOObj* ioobj,
				BufferString& err )
{
    if ( !ioobj )
	{ err = "No object to store in data base"; return false; }

    PtrMan<MPESetupTranslator> tr =
		dynamic_cast<MPESetupTranslator*>(ioobj->createTranslator());
    if ( !tr )
	{ err = "Selected object is not a Setup"; return false; }

    PtrMan<Conn> conn = ioobj->getConn( Conn::Write );
    if ( !conn )
    {
	err.set( "Cannot open " ).add( ioobj->fullUserExpr(false) );
	return false;
    }

    err = mFromUiStringTodo(tr->write( setup, *conn ));
    bool rv = err.isEmpty();
    if ( rv )
	err = mFromUiStringTodo(tr->warningMsg());
    else
	conn->rollback();

    return rv;
}


uiString dgbMPESetupTranslator::read( MPESetup& setup, Conn& conn )
{
    warningmsg = uiString::empty();
    if ( !conn.forRead() || !conn.isStream() )
	return mINTERNAL("bad connection");

    ascistream astream( ((StreamConn&)conn).iStream() );

    IOPar iopar( astream );
    if ( iopar.isEmpty() )
	return tr("Empty input file");

    if ( !setup.usePar( iopar ))
	return uiStrings::phrCannotRead(tr("setup-file"));

    return uiString::empty();
}


uiString dgbMPESetupTranslator::write( const MPESetup& setup, Conn& conn )
{
    warningmsg = uiString::empty();
    if ( !conn.forWrite() || !conn.isStream() )
	return mINTERNAL("bad connection");

    IOPar iop; setup.fillPar( iop );
    if ( !iop.write(((StreamConn&)conn).oStream(),mTranslGroupName(MPESetup)) )
	return uiStrings::phrCannotWrite(tr("setup to file"));
    return uiString::empty();
}
