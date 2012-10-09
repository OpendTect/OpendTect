/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		Oct 2008
________________________________________________________________________

-*/
static const char* rcsID = "$Id$";

#include "prestackprocessortransl.h"

#include "prestackprocessor.h"
#include "ascstream.h"

defineTranslatorGroup(PreStackProc,"Prestack Processor");
defineTranslator(dgb,PreStackProc,mDGBKey);

mDefSimpleTranslatorioContext(PreStackProc,Misc)


int PreStackProcTranslatorGroup::selector( const char* key )
{
    return defaultSelector( theInst().userName(), key );
}


bool PreStackProcTranslator::retrieve( PreStack::ProcessManager& md,
	const IOObj* ioobj, BufferString& bs )
{
    if ( !ioobj ) { bs = "Cannot find object in data base"; return false; }
    mDynamicCastGet(PreStackProcTranslator*,t,ioobj->getTranslator())
    if ( !t ) { bs = "Selected object is not a Mute Definition"; return false; }
    PtrMan<PreStackProcTranslator> tr = t;
    PtrMan<Conn> conn = ioobj->getConn( Conn::Read );
    if ( !conn )
        { bs = "Cannot open "; bs += ioobj->fullUserExpr(true); return false; }
    bs = tr->read( md, *conn );
    return bs.isEmpty();
}


bool PreStackProcTranslator::store( const PreStack::ProcessManager& md,
	const IOObj* ioobj, BufferString& bs )
{
    if ( !ioobj ) { bs = "No object to store set in data base"; return false; }
    mDynamicCastGet(PreStackProcTranslator*,tr,ioobj->getTranslator())
    if ( !tr ) { bs = "Selected object is not a Mute Definition"; return false;}

    bs = "";
    PtrMan<Conn> conn = ioobj->getConn( Conn::Write );
    if ( !conn )
        { bs = "Cannot open "; bs += ioobj->fullUserExpr(false); }
    else
    {
	bs = tr->write( md, *conn );
    }

    delete tr;
    return bs.isEmpty();
}


const char* dgbPreStackProcTranslator::read( PreStack::ProcessManager& md,
					     Conn& conn )
{
    if ( !conn.forRead() || !conn.isStream() )
	return "Internal error: bad connection";

    ascistream astrm( ((StreamConn&)conn).iStream() );
    std::istream& strm = astrm.stream();
    if ( !strm.good() )
	return "Cannot read from input file";
    if ( !astrm.isOfFileType(mTranslGroupName(PreStackProc)) )
	return "Input file is not a Mute Definition file";
    if ( atEndOfSection(astrm) ) astrm.next();
    const IOPar par( astrm );

    if ( md.usePar( par ) ) return 0;

    return md.errMsg() ? md.errMsg() : "Could not read processing info.";
}


const char* dgbPreStackProcTranslator::write(const PreStack::ProcessManager& md,
					     Conn& conn)
{
    if ( !conn.forWrite() || !conn.isStream() )
	return "Internal error: bad connection";

    ascostream astrm( ((StreamConn&)conn).oStream() );
    astrm.putHeader( mTranslGroupName(PreStackProc) );
    std::ostream& strm = astrm.stream();
    if ( !strm.good() )
	return "Cannot write to output Mute Definition file";

    IOPar par;
    md.fillPar( par );

    par.putTo( astrm );

    if ( strm.good() )
	return 0;

    return "Error during write to output process definition file";
}
