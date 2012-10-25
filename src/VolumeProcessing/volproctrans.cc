/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		March 2007
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "volproctrans.h"

#include "volprocchain.h"
#include "ascstream.h"

defineTranslatorGroup(VolProcessing,"Volume Processing Setup");
defineTranslator(dgb,VolProcessing,mDGBKey);

mDefSimpleTranslatorioContext(VolProcessing,Misc)


int VolProcessingTranslatorGroup::selector( const char* key )
{
    return defaultSelector( theInst().userName(), key );
}


bool VolProcessingTranslator::retrieve( VolProc::Chain& vr,
				    const IOObj* ioobj,
				    BufferString& bs )
{
    if ( !ioobj ) { bs = "Cannot find object in data base"; return false; }
    mDynamicCastGet(VolProcessingTranslator*,t,ioobj->createTranslator())
    if ( !t )
    {
	bs = "Selected object is not a Volume Processing Setup";
	return false;
    }
    PtrMan<VolProcessingTranslator> tr = t;
    PtrMan<Conn> conn = ioobj->getConn( Conn::Read );
    if ( !conn )
        { bs = "Cannot open "; bs += ioobj->fullUserExpr(true); return false; }

    bs = tr->read( vr, *conn );
    if ( bs.isEmpty() )
    {
	vr.setStorageID( ioobj->key() );
	return true;
    }

    return false;
}


bool VolProcessingTranslator::store( const VolProc::Chain& vr,
				const IOObj* ioobj, BufferString& bs )
{
    if ( !ioobj ) { bs = "No object to store set in data base"; return false; }
    mDynamicCast(VolProcessingTranslator*,PtrMan<VolProcessingTranslator> tr,
		 ioobj->createTranslator())
    if ( !tr )
    {
	bs = "Selected object is not a Volume Processing Setup";
	return false;
    }

    bs = "";
    PtrMan<Conn> conn = ioobj->getConn( Conn::Write );
    if ( !conn )
        { bs = "Cannot open "; bs += ioobj->fullUserExpr(false); }
    else
	bs = tr->write( vr, *conn );

    return bs.isEmpty();
}


const char* dgbVolProcessingTranslator::read( VolProc::Chain& tr,
					      Conn& conn )
{
    if ( !conn.forRead() || !conn.isStream() )
	return "Internal error: bad connection";

    ascistream astrm( ((StreamConn&)conn).iStream() );
    std::istream& strm = astrm.stream();
    if ( !strm.good() )
	return "Cannot read from input file";
    if ( !astrm.isOfFileType(mTranslGroupName(VolProcessing)) )
	return "Input file is not a Volume Processing setup file";
    if ( atEndOfSection(astrm) ) astrm.next();

    IOPar par;
    par.getFrom( astrm );
    if ( par.isEmpty() )
	return "Input file contains no data";
    if ( !tr.usePar( par ) )
	return tr.errMsg();

    return 0;
}


const char* dgbVolProcessingTranslator::write( const VolProc::Chain& tr,
					   Conn& conn )
{
    if ( !conn.forWrite() || !conn.isStream() )
	return "Internal error: bad connection";

    ascostream astrm( ((StreamConn&)conn).oStream() );
    astrm.putHeader( mTranslGroupName(VolProcessing) );
    std::ostream& strm = astrm.stream();
    if ( !strm.good() )
	return "Cannot write to output Volume Processing setup file";

    IOPar par;
    tr.fillPar( par );
    par.putTo( astrm );

    return strm.good() ? 0
	:  "Error during write to output Volume Processing setup file";
}
