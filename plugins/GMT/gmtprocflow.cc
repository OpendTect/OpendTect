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
defineTranslator(dgb,ODGMTProcFlow,mDGBKey);
mDefSimpleTranslatorioContextWithExtra(ODGMTProcFlow,None,
					ctxt->selkey_ = ODGMT::sKeyGMTSelKey())

uiString ODGMTProcFlowTranslatorGroup::sTypeName(int num)
{ return tr("GMT process flow",0,num); }



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
	{ str = ioobj->phrCannotOpenObj(); return false; }
    str = trans->read( pf, *conn );
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

    str = uiString::emptyString();
    PtrMan<Conn> conn = ioobj->getConn( Conn::Write );
    if ( !conn )
	{ str = ioobj->phrCannotOpenObj(); return false; }
    else
	str = trans->write( pf, *conn );

    return str.isEmpty();
}


uiString dgbODGMTProcFlowTranslator::read( ODGMT::ProcFlow& pf, Conn& conn )
{
    if ( !conn.forRead() || !conn.isStream() )
	return mINTERNAL("bad connection");

    ascistream astrm( ((StreamConn&)conn).iStream() );
    if ( !astrm.isOK() )
	return uiStrings::phrCannotRead(tr("from input file"));
    if ( !astrm.isOfFileType(mTranslGroupName(ODGMTProcFlow)) )
	return tr("Input file is not a GMT Processing flow");
    if ( atEndOfSection(astrm) )
	astrm.next();
    if ( atEndOfSection(astrm) )
	return tr("Input file is empty");

    pf.setName( IOM().nameOf(conn.linkedTo()) );
    pf.pars().getFrom( astrm );
    return uiString::emptyString();
}


uiString dgbODGMTProcFlowTranslator::write( const ODGMT::ProcFlow& pf,
					    Conn& conn )
{
    if ( !conn.forWrite() || !conn.isStream() )
	return mINTERNAL("bad connection");

    const uiString filtypstr = tr("GMT flow file");
    ascostream astrm( ((StreamConn&)conn).oStream() );
    astrm.putHeader( mTranslGroupName(ODGMTProcFlow) );
    if ( !astrm.isOK() )
	return uiStrings::phrCannotWrite( filtypstr );

    pf.pars().putTo( astrm );
    return astrm.isOK() ? uiString::emptyString()
			: uiStrings::phrErrDuringWrite( filtypstr );
}
