/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Satyaki Maitra
 Date:		September 2016
________________________________________________________________________

-*/

#include "probetr.h"
#include "probe.h"
#include "ioobjctxt.h"
#include "ascstream.h"
#include "ioobj.h"
#include "iopar.h"
#include "ptrman.h"
#include "survinfo.h"
#include "streamconn.h"
#include "keystrs.h"
#include "uistrings.h"

defineTranslatorGroup(Probe,"Probe Group");
defineTranslator(dgb,Probe,mDGBKey);
mDefSimpleTranslatorioContext( Probe, Loc )
mDefSimpleTranslatorSelector( Probe )
uiString ProbeTranslatorGroup::sTypeName( int num)
{ return uiStrings::sProbe( num ); }

#define mErrRet( uimsg ) \
{ \
    errmsg = uimsg; \
    return 0; \
}

Probe* ProbeTranslator::retrieve( const IOObj* ioobj, uiString& errmsg )
{
    if ( !ioobj )
	mErrRet( uiStrings::phrCannotFindDBEntry(uiStrings::sProbe()) );

    mDynamicCast(ProbeTranslator*,PtrMan<ProbeTranslator> trl,
		 ioobj->createTranslator());
    if ( !trl )
	mErrRet( uiStrings::phrSelectObjectWrongType(uiStrings::sProbe()) )

    PtrMan<Conn> conn = ioobj->getConn( Conn::Read );
    if ( !conn )
	mErrRet( ioobj->phrCannotOpenObj() )

    Probe* probe = trl->read( *conn, errmsg );
    if ( !errmsg.isEmpty() )
	return 0;

    probe->setName( ioobj->name() );
    return probe;
}


bool ProbeTranslator::store( const Probe& probe, const IOObj* ioobj,
			       uiString& errmsg )
{
    ConstRefMan<Probe> proberef( &probe ); // keep it alive
    if ( !ioobj )
	mErrRet( uiStrings::phrCannotFindDBEntry(uiStrings::sProbe()) );

    mDynamicCast(ProbeTranslator*,PtrMan<ProbeTranslator> trl,
		 ioobj->createTranslator());
    if ( !trl )
	mErrRet( uiStrings::phrSelectObjectWrongType(uiStrings::sProbe()) )

    PtrMan<Conn> conn = ioobj->getConn( Conn::Write );
    if ( !conn )
	mErrRet( ioobj->phrCannotOpenObj() )

    errmsg = trl->write( probe, *conn );
    if ( !errmsg.isEmpty() )
	{ conn->rollback(); return false; }

    // Now that we have the set, make sure it gets a standard entry in the omf
    bool needcommit = false;
    const FixedString ioobjprbtype = ioobj->pars().find(sKey::Type());
    if ( ioobjprbtype != probe.type() )
    {
	ioobj->pars().set( sKey::Type(), probe.type() );
	needcommit = true;
    }

    if ( needcommit )
	ioobj->commitChanges();

    return true;
}



Probe* dgbProbeTranslator::read( Conn& conn, uiString& errmsg )
{
    if ( !conn.forRead() || !conn.isStream() )
	mErrRet( uiStrings::phrCannotOpenInpFile() )

    ascistream astrm( ((StreamConn&)conn).iStream() );
    if ( !astrm.isOK() )
	mErrRet( uiStrings::phrCannotOpenInpFile() )
    else if ( !astrm.isOfFileType(mTranslGroupName(Probe)) )
	mErrRet( uiStrings::phrSelectObjectWrongType(uiStrings::sProbe()) )
    if ( atEndOfSection(astrm) ) astrm.next();
    if ( atEndOfSection(astrm) )
	mErrRet( uiStrings::sNoValidData() )

    IOPar iopar; iopar.getFrom( astrm );
    return ProbeFac().create( iopar );
}


uiString dgbProbeTranslator::write( const Probe& probe, Conn& conn )
{
    if ( !conn.forWrite() || !conn.isStream() )
	return uiStrings::phrCannotOpenOutpFile();

    ascostream astrm( ((StreamConn&)conn).oStream() );
    astrm.putHeader( mTranslGroupName(Probe) );
    if ( !astrm.isOK() )
	return uiStrings::phrCannotOpenOutpFile();

    IOPar par;
    probe.fillPar( par );
    par.putTo( astrm );
    return astrm.isOK() ? uiString::empty()
			: uiStrings::phrCannotWrite( uiStrings::sProbe() );
}
