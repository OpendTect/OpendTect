/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Raman Singh
 * DATE     : Sept 2008
-*/


#include "gmtdef.h"
#include "gmtprocflow.h"
#include "gmtprocflowtr.h"
#include "ascstream.h"
#include "ioman.h"
#include "uistrings.h"

defineTranslatorGroup(ODGMTProcFlow,"GMT process flow");
uiString ODGMTProcFlowTranslatorGroup::sTypeName(int num)
{ return tr("GMT process flow",0,num); }

defineTranslator(dgb,ODGMTProcFlow,mDGBKey);
mDefSimpleTranslatorioContextWithExtra(ODGMTProcFlow,None,
					ctxt->selkey_ = ODGMT::sKeyGMTSelKey())


ODGMT::ProcFlow::ProcFlow( const char* nm )
    : NamedObject(nm)
{
}


ODGMT::ProcFlow::~ProcFlow()
{
}


int ODGMTProcFlowTranslatorGroup::selector( const char* key )
{
    int retval = defaultSelector( sGroupName(), key );
    if ( retval ) return retval;
    return defaultSelector("GMT data",key) ? 1 : 0;
}


bool ODGMTProcFlowTranslator::retrieve( ODGMT::ProcFlow& pf, const IOObj* ioobj,
					BufferString& errmsg )
{
    if ( !ioobj )
	{ errmsg = "Cannot find flow object in data base"; return false; }
    mDynamicCast(ODGMTProcFlowTranslator*,PtrMan<ODGMTProcFlowTranslator> tr,
		 ioobj->createTranslator());
    if ( !tr )
	{ errmsg = "Selected object is not a GMT flow"; return false; }

    PtrMan<Conn> conn = ioobj->getConn( Conn::Read );
    if ( !conn )
        errmsg.set( "Cannot open " ).add( ioobj->fullUserExpr(true) );
    else
	errmsg = tr->read( pf, *conn );

    return errmsg.isEmpty();
}


bool ODGMTProcFlowTranslator::store( const ODGMT::ProcFlow& pf,
				     const IOObj* ioobj, BufferString& errmsg )
{
    if ( !ioobj )
	{ errmsg = "No object to store flow in data base"; return false; }
    mDynamicCast(ODGMTProcFlowTranslator*,PtrMan<ODGMTProcFlowTranslator> tr,
		 ioobj->createTranslator());

    if ( !tr )
	{ errmsg = "Selected object is not a GMT flow"; return false; }

    errmsg.setEmpty();
    PtrMan<Conn> conn = ioobj->getConn( Conn::Write );
    if ( !conn )
        errmsg.set( "Cannot open " ).add( ioobj->fullUserExpr(false) );
    else
	errmsg = tr->write( pf, *conn );

    if ( !errmsg.isEmpty() )
    {
	if ( conn )
	    conn->rollback();
	return false;
    }

    return true;
}


bool ODGMTProcFlowTranslator::retrieve( ODGMT::ProcFlow& pf, const IOObj* ioobj,
					uiString& str )
{
    if ( !ioobj ) { str = uiStrings::phrCannotFind(tr("object in data base"));
								return false; }
    mDynamicCast(ODGMTProcFlowTranslator*,PtrMan<ODGMTProcFlowTranslator> trans,
		 ioobj->createTranslator());
    if ( !trans ) { str = tr("Selected object is not a GMT flow");
								return false; }

    PtrMan<Conn> conn = ioobj->getConn( Conn::Read );
    if ( !conn )
        { str = uiStrings::phrCannotOpen(toUiString(ioobj->fullUserExpr(true)));
								 return false; }
    str = mToUiStringTodo(trans->read( pf, *conn ));
    return str.isEmpty();
}


bool ODGMTProcFlowTranslator::store( const ODGMT::ProcFlow& pf,
				     const IOObj* ioobj, uiString& str )
{
    if ( !ioobj ) { str = uiStrings::phrCannotCreateDBEntryFor(
					   tr("current flow")); return false; }
    mDynamicCast(ODGMTProcFlowTranslator*,PtrMan<ODGMTProcFlowTranslator> trans,
		 ioobj->createTranslator());

    if ( !trans ) { str = tr("Selected object is not a GMT flow");
								return false;}

    str = uiStrings::sEmptyString();
    PtrMan<Conn> conn = ioobj->getConn( Conn::Write );
    if ( !conn )
        { str = uiStrings::phrCannotOpen(
				    toUiString(ioobj->fullUserExpr(false))); }
    else
	str = mToUiStringTodo(trans->write( pf, *conn ));

    return str.isEmpty();
}


const char* dgbODGMTProcFlowTranslator::read( ODGMT::ProcFlow& pf, Conn& conn )
{
    if ( !conn.forRead() || !conn.isStream() )
	return "Internal error: bad connection";

    ascistream astrm( ((StreamConn&)conn).iStream() );
    if ( !astrm.isOK() )
	return "Cannot read from input file";
    if ( !astrm.isOfFileType(mTranslGroupName(ODGMTProcFlow)) )
	return "Input file is not a Processing flow";
    if ( atEndOfSection(astrm) )
	astrm.next();
    if ( atEndOfSection(astrm) )
	return "Input file is empty";

    pf.setName( IOM().nameOf(conn.linkedTo()) );
    pf.pars().getFrom( astrm );
    return 0;
}


const char* dgbODGMTProcFlowTranslator::write( const ODGMT::ProcFlow& pf,
						Conn& conn )
{
    if ( !conn.forWrite() || !conn.isStream() )
	return "Internal error: bad connection";

    ascostream astrm( ((StreamConn&)conn).oStream() );
    astrm.putHeader( mTranslGroupName(ODGMTProcFlow) );
    if ( !astrm.isOK() )
	return "Cannot write to output GMT flow file";

    pf.pars().putTo( astrm );
    return astrm.isOK() ? 0 : "Error during write to GMT flow file";
}
