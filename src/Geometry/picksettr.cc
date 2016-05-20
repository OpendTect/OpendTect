/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		Jul 2005
________________________________________________________________________

-*/

#include "picksettr.h"
#include "pickset.h"
#include "ctxtioobj.h"
#include "binidvalset.h"
#include "datapointset.h"
#include "ascstream.h"
#include "ioobj.h"
#include "ioman.h"
#include "iopar.h"
#include "ptrman.h"
#include "separstr.h"
#include "survinfo.h"
#include "streamconn.h"
#include "polygon.h"
#include "keystrs.h"
#include "unitofmeasure.h"
#include "uistrings.h"

defineTranslatorGroup(PickSet,"PickSet Group");
defineTranslator(dgb,PickSet,mDGBKey);
mDefSimpleTranslatorioContext( PickSet, Loc )
mDefSimpleTranslatorSelector( PickSet )
uiString PickSetTranslatorGroup::sTypeName( int num)
{ return uiStrings::sPickSet( num ); }


// In earlier versions, 'Type' and 'Category' were mixed
// We want to go to a situation where a pickset is
// Type: either 'Polygon' or 'PickSet'
// Category: empty, or things like 'ArrowAnnotations', 'Text', ...
// This requires some juggling


static bool getCatFromType( const IOPar& iop, BufferString& cat )
{
    FileMultiString fms;
    iop.get( sKey::Type(), fms );
    const int fmssz = fms.size();
    for ( int idx=0; idx<fmssz; idx++ )
    {
	const BufferString typ = fms[idx];
	if ( !typ.isEmpty() && typ != sKey::PickSet() && typ != sKey::Polygon())
	{
	    cat.set( typ );
	    return true;
	}
    }
    return false;
}


static bool getIsPolygon( const IOObj& ioobj, bool& ispoly )
{
    FileMultiString fms;
    ioobj.pars().get( sKey::Type(), fms );
    const int fmssz = fms.size();
    ispoly = false;
    for ( int idx=0; idx<fmssz; idx++ )
    {
	const BufferString typ = fms[idx];
	if ( typ == sKey::PickSet() )
	    return true;
	else if ( typ == sKey::Polygon() )
	    { ispoly = true; return true; }
    }
    return false;
}


bool PickSetTranslator::retrieve( Pick::Set& ps, const IOObj* ioobj,
				  uiString& errmsg )
{
    if ( !ioobj )
    {
	errmsg = uiStrings::phrCannotFindDBEntry( ioobj->uiName() );
	return false;
    }

    mDynamicCast(PickSetTranslator*,PtrMan<PickSetTranslator> tr,
		 ioobj->createTranslator());
    if ( !tr )
    {
	errmsg = uiStrings::phrSelectObjectWrongType( uiStrings::sPickSet() );
	return false;
    }

    PtrMan<Conn> conn = ioobj->getConn( Conn::Read );
    if ( !conn )
    {
	errmsg = uiStrings::phrCannotOpen( ioobj->uiName() );
	return false;
    }

    errmsg = tr->read( ps, *conn );
    if ( !errmsg.isEmpty() )
	return false;

    if ( FixedString(ps.category()).isEmpty() )
	ps.setCategory( getCategory(*ioobj,&ps) );

    bool ispoly = false;
    if ( getIsPolygon(*ioobj,ispoly) )
	ps.setIsPolygon( ispoly );

    ps.setName( ioobj->name() );
    return true;
}


bool PickSetTranslator::isPolygon( const IOObj& ioobj )
{
    bool ispoly = false;
    getIsPolygon( ioobj, ispoly );
    return ispoly;
}


BufferString PickSetTranslator::getCategory( const IOObj& ioobj, Pick::Set* ps )
{
    BufferString cat = ioobj.pars().find( sKey::Category() );
    if ( cat.isEmpty() )
    {
	if ( !getCatFromType(ioobj.pars(),cat) && ps )
	    getCatFromType( ps->pars(), cat );
    }
    return cat;
}


bool PickSetTranslator::store( const Pick::Set& ps, const IOObj* ioobj,
			       uiString& errmsg )
{
    ConstRefMan<Pick::Set> psrefman( &ps ); // keep it alive
    if ( !ioobj )
    {
	errmsg = uiStrings::phrCannotFindDBEntry( ioobj->uiName() );
	return false;
    }

    mDynamicCast(PickSetTranslator*,PtrMan<PickSetTranslator> tr,
		 ioobj->createTranslator());
    if ( !tr )
    {
	errmsg = uiStrings::phrSelectObjectWrongType( uiStrings::sPickSet() );
	return false;
    }

    PtrMan<Conn> conn = ioobj->getConn( Conn::Write );
    if ( !conn )
    {
	errmsg = uiStrings::phrCannotOpen( ioobj->uiName() );
	return false;
    }

    errmsg = tr->write( ps, *conn );
    if ( !errmsg.isEmpty() )
	return false;

    // Now that we have the set, make sure it gets a standard entry in the omf
    bool needcommit = false;
    const FixedString ioobjpstype = ioobj->pars().find(sKey::Type());
    if ( ioobjpstype != ps.type() )
    {
	ioobj->pars().set( sKey::Type(), ps.type() );
	needcommit = true;
    }
    const FixedString ioobjpscat = ioobj->pars().find( sKey::Category() );
    if ( ioobjpscat != ps.category() )
    {
	ioobj->pars().set( sKey::Category(), ps.category() );
	needcommit = true;
    }

    if ( needcommit )
	IOM().commitChanges( *ioobj );
    const_cast<Pick::Set&>(ps).setName( ioobj->name() );

    return true;
}


uiString dgbPickSetTranslator::read( Pick::Set& ps, Conn& conn )
{
    if ( !conn.forRead() || !conn.isStream() )
	return uiStrings::sCantOpenInpFile();

    ascistream astrm( ((StreamConn&)conn).iStream() );
    if ( !astrm.isOK() )
	return uiStrings::sCantOpenInpFile();
    else if ( !astrm.isOfFileType(mTranslGroupName(PickSet)) )
	return uiStrings::phrSelectObjectWrongType( uiStrings::sPickSet() );
    if ( atEndOfSection(astrm) ) astrm.next();
    if ( atEndOfSection(astrm) )
	return uiStrings::sNoValidData();

    if ( astrm.hasKeyword("Ref") ) // Keep support for pre v3.2 format
    {
	Pick::Set::Disp disp;
	// In old format we can find mulitple pick sets. Just gather them all
	// in the pick set
	for ( int ips=0; !atEndOfSection(astrm); ips++ )
	{
	    astrm.next();
	    if ( astrm.hasKeyword(sKey::Color()) )
	    {
		disp.mkstyle_.color_.use( astrm.value() );
		disp.mkstyle_.color_.setTransparency( 0 );
		astrm.next();
	    }
	    if ( astrm.hasKeyword(sKey::Size()) )
	    {
		disp.mkstyle_.size_ = astrm.getIValue();
		astrm.next();
	    }
	    if ( astrm.hasKeyword(Pick::Set::sKeyMarkerType()) )
	    {
		// OD::MarkerStyle3D::Type used to start with -1. This has
		// changed and thus a '+1' is needed to keep the same shapes
		const int markertype = astrm.getIValue() + 1;
		disp.mkstyle_.type_ = (OD::MarkerStyle3D::Type)markertype;
		astrm.next();
	    }
	    while ( !atEndOfSection(astrm) )
	    {
		Pick::Location loc;
		if ( !loc.fromString(astrm.keyWord()) )
		    break;

		ps += loc;
		astrm.next();
	    }
	    while ( !atEndOfSection(astrm) ) astrm.next();
	    astrm.next();
	}
	ps.setDisp( disp );
    }
    else // New format
    {
	IOPar iopar; iopar.getFrom( astrm );
	ps.usePar( iopar );

	astrm.next();
	while ( !atEndOfSection(astrm) )
	{
	    Pick::Location loc;
	    if ( loc.fromString(astrm.keyWord()) )
		ps.add( loc );

	    astrm.next();
	}
    }

    return ps.size() ? uiStrings::sEmptyString() : uiStrings::sNoValidData();
}


uiString dgbPickSetTranslator::write( const Pick::Set& ps, Conn& conn )
{
    if ( !conn.forWrite() || !conn.isStream() )
	return uiStrings::sCantOpenOutpFile();

    ascostream astrm( ((StreamConn&)conn).oStream() );
    astrm.putHeader( mTranslGroupName(PickSet) );
    if ( !astrm.isOK() )
	return uiStrings::sCantOpenOutpFile();

    MonitorLock ml( ps );
    IOPar par;
    ps.fillPar( par );
    par.set( sKey::ZUnit(),
	     UnitOfMeasure::surveyDefZStorageUnit()->name() );
    par.putTo( astrm );

    od_ostream& strm = astrm.stream();
    for ( int iloc=0; iloc<ps.size(); iloc++ )
    {
	BufferString str;
	ps.get( iloc ).toString( str );
	strm << str << od_newline;
    }
    ml.unlockNow();

    astrm.newParagraph();
    return astrm.isOK() ? uiStrings::sEmptyString()
			: uiStrings::phrCannotWrite( uiStrings::sPickSet() );
}
