/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Raman Singh
 * DATE     : Sept 2008
-*/

static const char* rcsID = "$Id: gmtprocflow.cc,v 1.2 2009/07/22 16:01:27 cvsbert Exp $";

#include "gmtdef.h"
#include "gmtprocflow.h"
#include "gmtprocflowtr.h"
#include "ascstream.h"

defineTranslatorGroup(ODGMTProcFlow,"GMT process flow");
defineTranslator(dgb,ODGMTProcFlow,mDGBKey);
mDefSimpleTranslatorioContextWithExtra(ODGMTProcFlow,None,
					ctxt->selkey = ODGMT::sKeyGMTSelKey)


ODGMT::ProcFlow::ProcFlow( const char* nm )
    : NamedObject(nm)
{
}


ODGMT::ProcFlow::~ProcFlow()
{
}


int ODGMTProcFlowTranslatorGroup::selector( const char* key )
{
    int retval = defaultSelector( theInst().userName(), key );
    if ( retval ) return retval;
    return defaultSelector("GMT data",key) ? 1 : 0;
}


bool ODGMTProcFlowTranslator::retrieve( ODGMT::ProcFlow& pf, const IOObj* ioobj,
					BufferString& bs )
{
    if ( !ioobj ) { bs = "Cannot find flow object in data base"; return false; }
    mDynamicCastGet(ODGMTProcFlowTranslator*,t,ioobj->getTranslator())
    if ( !t ) { bs = "Selected object is not a GMT flow"; return false; }
    PtrMan<ODGMTProcFlowTranslator> tr = t;
    PtrMan<Conn> conn = ioobj->getConn( Conn::Read );
    if ( !conn )
        { bs = "Cannot open "; bs += ioobj->fullUserExpr(true); return false; }
    bs = tr->read( pf, *conn );
    return bs.isEmpty();
}


bool ODGMTProcFlowTranslator::store( const ODGMT::ProcFlow& pf,
				     const IOObj* ioobj, BufferString& bs )
{
    if ( !ioobj ) { bs = "No object to store flow in data base"; return false; }
    mDynamicCastGet(ODGMTProcFlowTranslator*,tr,ioobj->getTranslator())
    if ( !tr ) { bs = "Selected object is not a GMT flow"; return false;}

    bs = "";
    PtrMan<Conn> conn = ioobj->getConn( Conn::Write );
    if ( !conn )
        { bs = "Cannot open "; bs += ioobj->fullUserExpr(false); }
    else
	bs = tr->write( pf, *conn );
    delete tr;
    return bs.isEmpty();
}


const char* dgbODGMTProcFlowTranslator::read( ODGMT::ProcFlow& pf, Conn& conn )
{
    if ( !conn.forRead() || !conn.isStream() )
	return "Internal error: bad connection";

    ascistream astrm( ((StreamConn&)conn).iStream() );
    std::istream& strm = astrm.stream();
    if ( !strm.good() )
	return "Cannot read from input file";
    if ( !astrm.isOfFileType(mTranslGroupName(ODGMTProcFlow)) )
	return "Input file is not a Processing flow";
    if ( atEndOfSection(astrm) ) astrm.next();
    if ( atEndOfSection(astrm) )
	return "Input file is empty";

    pf.setName( conn.ioobj ? (const char*)conn.ioobj->name() : "" );
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
    std::ostream& strm = astrm.stream();
    if ( !strm.good() )
	return "Cannot write to output GMT flow file";

    pf.pars().putTo( astrm );
    return strm.good() ? 0 : "Error during write to GMT flow file";
}
