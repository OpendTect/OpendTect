/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "prestackprocessortransl.h"

#include "prestackprocessor.h"
#include "ascstream.h"
#include "uistrings.h"

// PreStackProcTranslatorGroup

defineTranslatorGroup(PreStackProc,"Prestack Processor");

PreStackProcTranslatorGroup::PreStackProcTranslatorGroup()
    : TranslatorGroup("PreStackProc")
{
}


uiString PreStackProcTranslatorGroup::sTypeName( int num )
{ return tr("Prestack Processor", 0, num ); }

defineTranslator(dgb,PreStackProc,mDGBKey);

mDefSimpleTranslatorioContext(PreStackProc,Misc);
mDefSimpleTranslatorSelector(PreStackProc);

// PreStackProcTranslator

PreStackProcTranslator::PreStackProcTranslator( const char* nm, const char* unm)
    : Translator(nm,unm)
{
}


PreStackProcTranslator::~PreStackProcTranslator()
{
}


uiString PreStackProcTranslator::sSelObjNotPreStackProc()
{
    return tr("Selected object is not a Prestack Processing setup");
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
	msg = sSelObjNotPreStackProc();
	return false;
    }

    PtrMan<Conn> conn = ioobj->getConn( Conn::Read );
    if ( !conn )
    {
	msg = uiStrings::phrCannotOpen(toUiString(ioobj->fullUserExpr(true)));
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
	msg = sNoIoobjMsg();
	return false;
    }

    mDynamicCast(PreStackProcTranslator*,PtrMan<PreStackProcTranslator> ptrl,
		 ioobj->createTranslator());
    if ( !ptrl )
    {
	msg = sSelObjNotPreStackProc();
	return false;
    }

    msg.setEmpty();
    PtrMan<Conn> conn = ioobj->getConn( Conn::Write );
    if ( !conn )
    {
	msg = uiStrings::phrCannotOpen(toUiString(ioobj->fullUserExpr(false)));
    }
    else
    {
	msg = ptrl->write( md, *conn );
    }

    return msg.isEmpty();
}


// dgbPreStackProcTranslator

dgbPreStackProcTranslator::dgbPreStackProcTranslator( const char* nm,
						      const char* unm )
    : PreStackProcTranslator(nm,unm)
{
}


uiString dgbPreStackProcTranslator::read( PreStack::ProcessManager& md,
					  Conn& conn )
{
    if ( !conn.forRead() || !conn.isStream() )
	return sBadConnection();

    ascistream astrm( ((StreamConn&)conn).iStream() );
    if ( !astrm.isOK() )
	return uiStrings::phrCannotRead( mToUiStringTodo(userName()) );
    if ( !astrm.isOfFileType(mTranslGroupName(PreStackProc)) )
	return tr("Input file is not a Mute Definition file");
    if ( atEndOfSection(astrm) ) astrm.next();
    const IOPar par( astrm );

    if ( md.usePar( par ) ) return uiString::emptyString();

    return md.errMsg().isSet() ? md.errMsg()
			       : tr("Could not read processing info.");
}


uiString dgbPreStackProcTranslator::write(const PreStack::ProcessManager& md,
					     Conn& conn)
{
    if ( !conn.forWrite() || !conn.isStream() )
	return sBadConnection();

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
