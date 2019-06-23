/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : May 2001
-*/


#include "attribdescsettr.h"

#include "ascstream.h"
#include "attribdescset.h"
#include "conn.h"
#include "file.h"
#include "ioobj.h"
#include "iopar.h"
#include "keystrs.h"
#include "ptrman.h"
#include "uistrings.h"
#include "attribdescsettr.h"
#include "uistrings.h"

defineTranslatorGroup(AttribDescSet,"Attribute definitions");
defineTranslator(dgb,AttribDescSet,mDGBKey);
mDefSimpleTranslatorSelector(AttribDescSet);
mDefSimpleTranslatorioContext(AttribDescSet, Attr );
uiString AttribDescSetTranslatorGroup::sTypeName(int num)
{ return uiStrings::sAttribute(num); }


uiRetVal AttribDescSetTranslator::readFromStream( ascistream& astream,
				  Attrib::DescSet& ads, uiRetVal& warns )
{
    if ( mTranslGroupName(AttribDescSet) != astream.fileType() )
	return tr("File has wrong file type");

    IOPar iopar( astream );
    Attrib::DescSet readset( ads.is2D() );
    uiRetVal uirv = readset.usePar( iopar );
    if ( readset.isEmpty() )
    {
	if ( uirv.isOK() )
	    uirv.add( tr("Empty attribute set") );
	return uirv;
    }

    ads = readset;
    warns = uirv;
    return uiRetVal::OK();
}


uiRetVal AttribDescSetTranslator::retrieve( Attrib::DescSet& ads,
					const char* fnm, uiRetVal* warnings )
{
    if ( !File::exists(fnm) )
	return uiStrings::phrFileDoesNotExist( fnm );

    od_istream odstrm( fnm );
    ascistream astream( odstrm );
    uiRetVal dummy; if ( !warnings ) warnings = &dummy;
    return readFromStream( astream, ads, *warnings );
}


uiRetVal AttribDescSetTranslator::retrieve( Attrib::DescSet& ads,
					const IOObj* ioobj, uiRetVal* warns )
{
    uiRetVal uirv;
    if ( !ioobj )
	{ uirv.add( uiStrings::phrCannotFindObjInDB() ); return uirv; }

    PtrMan<AttribDescSetTranslator> trans
	= dynamic_cast<AttribDescSetTranslator*>( ioobj->createTranslator() );
    if ( !trans )
    {
	uirv.add( tr("Selected object is not an Attribute Set") );
	return uirv;
    }

    PtrMan<Conn> conn = ioobj->getConn( Conn::Read );
    if ( !conn )
    {
	uirv.add( uiStrings::phrCannotOpenForRead(ioobj->fullUserExpr(true)) );
	return uirv;
    }

    uirv = toUiString( trans->read( ads, *conn ) );
    if ( warns )
	*warns = trans->warnings();
    return uirv;
}


uiRetVal AttribDescSetTranslator::store( const Attrib::DescSet& ads,
				     const IOObj* ioobj, uiRetVal* warns )
{
    uiRetVal uirv;
    if ( !ioobj )
	{ uirv.add( sNoIoobjMsg() ); return uirv; }

    PtrMan<AttribDescSetTranslator> trans
	= dynamic_cast<AttribDescSetTranslator*>(ioobj->createTranslator());
    if ( !trans )
    {
	uirv.add( tr("Selected object is not an Attribute Set") );
	return uirv;
    }
    PtrMan<Conn> conn = ioobj->getConn( Conn::Write );
    if ( !conn )
    {
	uirv.add( uiStrings::phrCannotOpenForWrite(ioobj->fullUserExpr(false)));
	return uirv;
    }

    ioobj->pars().set( sKey::Type(), ads.is2D() ? "2D" : "3D" );
    ioobj->commitChanges();
    uirv = toUiString( trans->write(ads,*conn) );
    if ( !uirv.isOK() )
	conn->rollback();
    if ( warns )
	*warns = trans->warnings();
    return uirv;
}



uiRetVal dgbAttribDescSetTranslator::badConnRV()
{
    return uiRetVal( mINTERNAL("bad connection") );
}


uiRetVal dgbAttribDescSetTranslator::read( Attrib::DescSet& ads, Conn& conn )
{
    warns_.setOK();

    if ( !conn.forRead() || !conn.isStream() )
	return badConnRV();

    ascistream astream( ((StreamConn&)conn).iStream() );
    return readFromStream( astream, ads, warns_ );
}


uiRetVal dgbAttribDescSetTranslator::write( const Attrib::DescSet& ads,
						Conn& conn )
{
    warns_.setEmpty();
    if ( !conn.forWrite() || !conn.isStream() )
	return badConnRV();

    IOPar iopar( "Attribute Descriptions" );
    ads.fillPar( iopar );
    if ( !iopar.write( ((StreamConn&)conn).oStream(),
		mTranslGroupName(AttribDescSet) ) )
	return uiRetVal( tr("Cannot write attributes to file") );

    return uiRetVal::OK();
}
