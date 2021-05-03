/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		March 2007
________________________________________________________________________

-*/

#include "volproctrans.h"

#include "volprocchain.h"
#include "ascstream.h"
#include "uistrings.h"

defineTranslatorGroup(VolProcessing,"Volume Processing Setup");
defineTranslator(dgb,VolProcessing,mDGBKey);

uiString VolProcessingTranslatorGroup::sTypeName(int num)
{ return tr("Volume Processing Setup",0,num); }

mDefSimpleTranslatorioContext(VolProcessing,Misc)
mDefSimpleTranslatorSelector(VolProcessing);


bool VolProcessingTranslator::retrieve( VolProc::Chain& vr,
				    const IOObj* ioobj,
				    uiString& bs )
{
    if ( !ioobj )
    {
	bs = uiStrings::phrCannotFindDBEntry(
	   mToUiStringTodo(VolProcessingTranslatorGroup::sGroupName()));
	return false;
    }

    PtrMan<Conn> conn = ioobj->getConn( Conn::Read );
    if ( !conn )
    { bs = uiStrings::phrCannotOpen( ioobj->uiName() ); return false; }

    mDynamicCastGet(VolProcessingTranslator*,t,ioobj->createTranslator())
    if ( t )
    {
	PtrMan<VolProcessingTranslator> tr = t;
	bs = toUiString( tr->read(vr,*conn) );
    }
    else
    {
	mDynamicCastGet(VolProcessing2DTranslator*,t2,ioobj->createTranslator())
	if ( !t2 )
	{
	    bs = uiStrings::phrCannotOpen( ioobj->uiName() );
	    return false;
	}

	PtrMan<VolProcessing2DTranslator> tr = t2;
	bs = toUiString( tr->read(vr,*conn) );
    }

    if ( bs.isEmpty() )
    {
	vr.setStorageID( ioobj->key() );
	return true;
    }

    return false;
}


bool VolProcessingTranslator::store( const VolProc::Chain& vr,
				const IOObj* ioobj, uiString& bs )
{
    if ( !ioobj )
    {
	bs = uiStrings::phrCannotFindDBEntry(
		 mToUiStringTodo(VolProcessingTranslatorGroup::sGroupName()));
	return false;
    }
    else if ( ioobj->implExists(false) && ioobj->implReadOnly() )
    {
	bs = uiStrings::phrJoinStrings(
				uiStrings::phrCannotWrite( uiStrings::sFile() ),
				toUiString(ioobj->fullUserExpr()) );
	return false;
    }

    bs = uiString::emptyString();
    PtrMan<Conn> conn = ioobj->getConn( Conn::Write );
    if ( !conn )
    { bs = uiStrings::phrCannotOpen( ioobj->uiName() ); return false; }

    mDynamicCastGet(VolProcessingTranslator*,t,ioobj->createTranslator())
    if ( t )
    {
	PtrMan<VolProcessingTranslator> tr = t;
	bs = toUiString( tr->write(vr,*conn) );
    }
    else
    {
	mDynamicCastGet(VolProcessing2DTranslator*,t2,ioobj->createTranslator())
	if ( !t2 )
	{
	    bs = uiStrings::phrCannotOpen( ioobj->uiName() );
	    return false;
	}

	PtrMan<VolProcessing2DTranslator> tr = t2;
	bs = toUiString( tr->write(vr,*conn) );
    }

    return bs.isEmpty();
}


const char* dgbVolProcessingTranslator::read( VolProc::Chain& chain,
					      Conn& conn )
{
    if ( !conn.forRead() || !conn.isStream() )
	return "Internal error: bad connection";

    ascistream astrm( ((StreamConn&)conn).iStream() );
    if ( !astrm.isOK() )
	return "Cannot read from input file";
    if ( !astrm.isOfFileType(mTranslGroupName(VolProcessing)) )
	return "Input file is not a Volume Processing setup file";
    if ( atEndOfSection(astrm) ) astrm.next();

    IOPar par;
    par.getFrom( astrm );
    if ( par.isEmpty() )
	return "Input file contains no data";
    if ( !chain.usePar(par) )
    {
	mDeclStaticString(errmsg);
	errmsg = chain.errMsg().getFullString();
	return errmsg;
    }

    return 0;
}


const char* dgbVolProcessingTranslator::write( const VolProc::Chain& chain,
					   Conn& conn )
{
    if ( !conn.forWrite() || !conn.isStream() )
	return "Internal error: bad connection";

    ascostream astrm( ((StreamConn&)conn).oStream() );
    astrm.putHeader( mTranslGroupName(VolProcessing) );
    if ( !astrm.isOK() )
	return "Cannot write to output Volume Processing setup file";

    IOPar par;
    chain.fillPar( par );
    par.putTo( astrm );

    return astrm.isOK() ? 0
	:  "Error during write to output Volume Processing setup file";
}


// 2D Stuff

defineTranslatorGroup(VolProcessing2D,"2D Volume Processing Setup");
defineTranslator(dgb,VolProcessing2D,mDGBKey);

uiString VolProcessing2DTranslatorGroup::sTypeName(int num)
{ return tr("2D Volume Processing Setup",0,num); }

mDefSimpleTranslatorioContext(VolProcessing2D,Misc)
mDefSimpleTranslatorSelector(VolProcessing2D);


bool VolProcessing2DTranslator::retrieve( VolProc::Chain& vr,
				    const IOObj* ioobj,
				    uiString& errmsg )
{
    return VolProcessingTranslator::retrieve( vr, ioobj, errmsg );
}


bool VolProcessing2DTranslator::store( const VolProc::Chain& vr,
				const IOObj* ioobj, uiString& errmsg )
{
    return VolProcessingTranslator::store( vr, ioobj, errmsg );
}


const char* dgbVolProcessing2DTranslator::read( VolProc::Chain& chain,
					      Conn& conn )
{
    if ( !conn.forRead() || !conn.isStream() )
	return "Internal error: bad connection";

    ascistream astrm( ((StreamConn&)conn).iStream() );
    if ( !astrm.isOK() )
	return "Cannot read from input file";
    if ( !astrm.isOfFileType(mTranslGroupName(VolProcessing2D)) )
	return "Input file is not a 2D Volume Processing setup file";
    if ( atEndOfSection(astrm) ) astrm.next();

    IOPar par;
    par.getFrom( astrm );
    if ( par.isEmpty() )
	return "Input file contains no data";
    if ( !chain.usePar( par ) )
	return chain.errMsg().getFullString();

    return 0;
}


const char* dgbVolProcessing2DTranslator::write( const VolProc::Chain& chain,
					   Conn& conn )
{
    if ( !conn.forWrite() || !conn.isStream() )
	return "Internal error: bad connection";

    ascostream astrm( ((StreamConn&)conn).oStream() );
    astrm.putHeader( mTranslGroupName(VolProcessing2D) );
    if ( !astrm.isOK() )
	return "Cannot write to output 2D Volume Processing setup file";

    IOPar par;
    chain.fillPar( par );
    par.putTo( astrm );

    return astrm.isOK() ? 0
	:  "Error during write to output 2D Volume Processing setup file";
}
