/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		Oct 2008
________________________________________________________________________

-*/

#include "prestackprocessortransl.h"

#include "prestackprocessor.h"
#include "ascstream.h"
#include "uistrings.h"

defineTranslatorGroup(PreStackProc,"Prestack Processor");
uiString PreStackProcTranslatorGroup::sTypeName( int num )
{ return tr("Prestack Processor", 0, num ); }

defineTranslator(dgb,PreStackProc,mDGBKey);

mDefSimpleTranslatorioContext(PreStackProc,Misc);
mDefSimpleTranslatorSelector(PreStackProc);

uiString PreStackProcTranslator::sSelObjNotPreStackProc()
{
    return tr("Selected object is not a Prestack Processing setup");
}


bool PreStackProcTranslator::retrieve( PreStack::ProcessManager& md,
	const IOObj* ioobj, uiString& msg )
{
    if ( !ioobj )
    {
	msg = uiStrings::phrCannotFindObjInDB();
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
	{ msg = ioobj->phrCannotOpenObj(); return false; }

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
	msg = ioobj->phrCannotOpenObj();
    else
	msg = ptrl->write( md, *conn );

    if ( !msg.isEmpty() )
    {
	if ( conn )
	    conn->rollback();
	return false;
    }

    return true;
}


uiString dgbPreStackProcTranslator::read( PreStack::ProcessManager& md,
					  Conn& conn )
{
    if ( !conn.forRead() || !conn.isStream() )
	return sBadConnection();

    ascistream astrm( ((StreamConn&)conn).iStream() );
    if ( !astrm.isOK() )
	return uiStrings::phrCannotRead( toUiString(userName()) );
    if ( !astrm.isOfFileType(mTranslGroupName(PreStackProc)) )
	return tr("Input file is not a Mute Definition file");
    if ( atEndOfSection(astrm) ) astrm.next();
    const IOPar par( astrm );

    if ( md.usePar( par ) ) return uiString::empty();

    return !md.errMsg().isEmpty() ? md.errMsg()
			     : uiStrings::phrCannotRead(tr("processing info."));
}


uiString dgbPreStackProcTranslator::write(const PreStack::ProcessManager& md,
					     Conn& conn)
{
    if ( !conn.forWrite() || !conn.isStream() )
	return sBadConnection();

    ascostream astrm( ((StreamConn&)conn).oStream() );
    astrm.putHeader( mTranslGroupName(PreStackProc) );
    if ( !astrm.isOK() )
	return uiStrings::phrCannotWrite( uiStrings::sMute() );

    IOPar par;
    md.fillPar( par );

    par.putTo( astrm );
    return astrm.isOK() ? uiString::empty()
	    : uiStrings::phrErrDuringWrite( tr("process definition file") );
}
