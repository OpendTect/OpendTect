/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		Oct 2008
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "prestackprocessortransl.h"

#include "prestackprocessor.h"
#include "ascstream.h"
#include "uistrings.h"

defineTranslatorGroup(PreStackProc,"Prestack Processor");
defineTranslator(dgb,PreStackProc,mDGBKey);

mDefSimpleTranslatorioContext(PreStackProc,Misc)


int PreStackProcTranslatorGroup::selector( const char* key )
{
    return defaultSelector( theInst().userName(), key );
}


bool PreStackProcTranslator::retrieve( PreStack::ProcessManager& md,
	const IOObj* ioobj, uiString& msg )
{
    if ( !ioobj )
    {
	msg = uiStrings::sCantFindODB();
	return false;
    }
    mDynamicCast(PreStackProcTranslator*,PtrMan<PreStackProcTranslator> ptrl,
		 ioobj->createTranslator());
    if ( !ptrl )
    {
	msg = uiStrings::sSelObjNotMuteDef();
	return false;
    }
    
    PtrMan<Conn> conn = ioobj->getConn( Conn::Read );
    if ( !conn )
    {
	msg = uiStrings::sCantOpen(mkUiString(ioobj->fullUserExpr(true)));
	return false;
    }

    msg = ptrl->read( md, *conn );
    return msg.isEmpty();
}


bool PreStackProcTranslator::store( const PreStack::ProcessManager& md,
	const IOObj* ioobj, uiString& msg )
{
    if ( !ioobj )
    {
	msg = uiStrings::sNoObjStoreSetDB();
	return false;
    }

    mDynamicCast(PreStackProcTranslator*,PtrMan<PreStackProcTranslator> ptrl,
		 ioobj->createTranslator());
    if ( !ptrl )
    {
	msg = uiStrings::sSelObjNotMuteDef();
	return false;
    }

    msg.setEmpty();
    PtrMan<Conn> conn = ioobj->getConn( Conn::Write );
    if ( !conn )
    {
	msg = uiStrings::sCantOpen(mkUiString(ioobj->fullUserExpr(false)));
    }
    else
    {
	msg = ptrl->write( md, *conn );
    }
    
    return msg.isEmpty();
}


uiString dgbPreStackProcTranslator::read( PreStack::ProcessManager& md,
					  Conn& conn )
{
    if ( !conn.forRead() || !conn.isStream() )
	return uiStrings::sBadConnection();

    ascistream astrm( ((StreamConn&)conn).iStream() );
    if ( !astrm.isOK() )
	return tr("Cannot read from input file");
    if ( !astrm.isOfFileType(mTranslGroupName(PreStackProc)) )
	return tr("Input file is not a Mute Definition file");
    if ( atEndOfSection(astrm) ) astrm.next();
    const IOPar par( astrm );

    if ( md.usePar( par ) ) return 0;

    return md.errMsg().isSet() ? md.errMsg()
			       : tr("Could not read processing info.");
}


uiString dgbPreStackProcTranslator::write(const PreStack::ProcessManager& md,
					     Conn& conn)
{
    if ( !conn.forWrite() || !conn.isStream() )
	return uiStrings::sBadConnection();

    ascostream astrm( ((StreamConn&)conn).oStream() );
    astrm.putHeader( mTranslGroupName(PreStackProc) );
    if ( !astrm.isOK() )
	return tr("Cannot write to output Mute Definition file");

    IOPar par;
    md.fillPar( par );

    par.putTo( astrm );
    return astrm.isOK() ? uiString::emptyString()
			: tr("Error during write to process definition file");
}
