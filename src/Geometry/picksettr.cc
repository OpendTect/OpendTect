/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		Jul 2005
________________________________________________________________________

-*/

#include "picksetfact.h"
#include "picksetmgr.h"
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

mDefSimpleTranslatorioContext( PickSet, Loc )
mDefSimpleTranslatorSelector( PickSet )

bool PickSetTranslator::retrieve( Pick::Set& ps, const IOObj* ioobj,
				  uiString& bs )
{
    if ( !ioobj )
    {
	bs = uiStrings::phrCannotFindDBEntry( ioobj->uiName() );
	return false;
    }

    mDynamicCast(PickSetTranslator*,PtrMan<PickSetTranslator> tr,
		 ioobj->createTranslator());
    if ( !tr )
    {
	bs = uiStrings::phrSelectObjectWrongType( uiStrings::sPickSet() );
	return false;
    }

    PtrMan<Conn> conn = ioobj->getConn( Conn::Read );
    if ( !conn )
    {
	bs = uiStrings::phrCannotOpen( ioobj->uiName() );
	return false;
    }

    bs = tr->read( ps, *conn );
    return bs.isEmpty();
}


bool PickSetTranslator::store( const Pick::Set& ps, const IOObj* ioobj,
			       uiString& bs )
{
    if ( !ioobj )
    {
	bs = uiStrings::phrCannotFindDBEntry( ioobj->uiName() );
	return false;
    }

    mDynamicCast(PickSetTranslator*,PtrMan<PickSetTranslator> tr,
		 ioobj->createTranslator());
    if ( !tr )
    {
	bs = uiStrings::phrSelectObjectWrongType( uiStrings::sPickSet() );
	return false;
    }

    bs = uiString::emptyString();
    PtrMan<Conn> conn = ioobj->getConn( Conn::Write );
    if ( !conn )
	{ bs = uiStrings::phrCannotOpen( ioobj->uiName() ); }
    else
	bs = tr->write( ps, *conn );

    FileMultiString pstype;
    ioobj->pars().get( sKey::Type(), pstype );
    if ( pstype.isEmpty() || pstype.size() > 1 )
    {
	ioobj->pars().set( sKey::Type(), ps.isPolygon() ? sKey::Polygon()
							: sKey::PickSet() );
	IOM().commitChanges( *ioobj );
    }

    return bs.isEmpty();
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

    ps.setName( IOM().nameOf(conn.linkedTo()) );

    if ( astrm.hasKeyword("Ref") ) // Keep support for pre v3.2 format
    {
	// In old format we can find mulitple pick sets. Just gather them all
	// in the pick set
	for ( int ips=0; !atEndOfSection(astrm); ips++ )
	{
	    astrm.next();
	    if ( astrm.hasKeyword(sKey::Color()) )
	    {
		ps.disp_.mkstyle_.color_.use( astrm.value() );
		ps.disp_.mkstyle_.color_.setTransparency( 0 );
		astrm.next();
	    }
	    if ( astrm.hasKeyword(sKey::Size()) )
	    {
		ps.disp_.mkstyle_.size_ = astrm.getIValue();
		astrm.next();
	    }
	    if ( astrm.hasKeyword(Pick::Set::sKeyMarkerType()) )
	    {
		// OD::MarkerStyle3D::Type used to start with -1. This has
		// changed and thus a '+1' is needed to keep the same shapes
		const int markertype = astrm.getIValue() + 1;
		ps.disp_.mkstyle_.type_ = (OD::MarkerStyle3D::Type)markertype;
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
    }
    else // New format
    {
	IOPar iopar; iopar.getFrom( astrm );
	ps.usePar( iopar );
	const Pos::SurvID survid( ps.getSurvID() );

	astrm.next();
	while ( !atEndOfSection(astrm) )
	{
	    Pick::Location loc;
	    loc.setSurvID( survid, false );
	    if ( loc.fromString(astrm.keyWord()) )
		ps += loc;

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

    BufferString str;
    IOPar par;
    ps.fillPar( par );
    par.putTo( astrm );

    od_ostream& strm = astrm.stream();
    for ( int iloc=0; iloc<ps.size(); iloc++ )
    {
	ps[iloc].toString( str );
	strm << str << od_newline;
    }

    astrm.newParagraph();
    return astrm.isOK() ? uiStrings::sEmptyString()
			: uiStrings::phrCannotWrite( uiStrings::sPickSet() );
}


void PickSetTranslator::createBinIDValueSets(
			const BufferStringSet& ioobjids,
			ObjectSet<BinIDValueSet>& bivsets,
			uiString& errmsg )
{
    for ( int idx=0; idx<ioobjids.size(); idx++ )
    {
	TypeSet<Coord3> crds;
	TypeSet<TrcKey> tks;
	if ( !getCoordSet(ioobjids.get(idx),crds,tks,errmsg) )
	    continue;

	BinIDValueSet* bs = new BinIDValueSet( 1, true );
	bivsets += bs;

	for ( int ipck=0; ipck<crds.size(); ipck++ )
	    bs->add( tks[idx].binID(), mCast(float, crds[idx].z ) );
    }
}


void PickSetTranslator::createDataPointSets( const BufferStringSet& ioobjids,
					     ObjectSet<DataPointSet>& dpss,
					     uiString& errmsg, bool is2d,
					     bool mini )
{
    for ( int idx=0; idx<ioobjids.size(); idx++ )
    {
	TypeSet<Coord3> crds; TypeSet<TrcKey> tks;
	if ( !getCoordSet(ioobjids.get(idx),crds,tks,errmsg) )
	    continue;

	DataPointSet* dps = new DataPointSet( is2d, mini );
	dpss += dps;

	DataPointSet::DataRow dr;
	for ( int ipck=0; ipck<crds.size(); ipck++ )
	{
	    dr.pos_ = crds[ipck];
	    //TODO use tks[ipck] in some way;
	    dps->addRow( dr );
	}
	dps->dataChanged();
    }
}


bool PickSetTranslator::getCoordSet( const char* id, TypeSet<Coord3>& crds,
				     TypeSet<TrcKey>& tks, uiString& errmsg )
{
    const MultiID key( id );
    const int setidx = Pick::Mgr().indexOf( key );
    const Pick::Set* ps = setidx < 0 ? 0 : &Pick::Mgr().get( setidx );
    Pick::Set* createdps = 0;
    if ( !ps )
    {
	PtrMan<IOObj> ioobj = IOM().get( key );
	if ( !ioobj )
	{
	    errmsg = uiStrings::phrCannotFindDBEntry( uiStrings::sPickSet() );
	    return false;
	}

	ps = createdps = new Pick::Set;
	if ( !retrieve(*createdps,ioobj,errmsg) )
	    { delete createdps; return false; }
    }

    for ( int ipck=0; ipck<ps->size(); ipck++ )
    {
	crds += ((*ps)[ipck]).pos();
	tks += ((*ps)[ipck]).trcKey();
    }

    delete createdps;
    return true;
}


void PickSetTranslator::fillConstraints( IOObjContext& ctxt, bool ispoly )
{
    if ( ispoly )
	ctxt.toselect_.require_.set( sKey::Type(), sKey::Polygon() );
    else
    {
	BufferString types = sKey::PickSet();
	types += "`"; // Allow Type to be empty or missing
	ctxt.toselect_.require_.set( sKey::Type(), types.buf() );
    }
}


ODPolygon<float>* PickSetTranslator::getPolygon( const IOObj& ioobj,
						 uiString& errmsg )
{
    Pick::Set ps;
    if ( !PickSetTranslator::retrieve(ps,&ioobj,errmsg) )
	return 0;

    if ( ps.size() < 2 )
    {
	errmsg = tr( "Polygon '%1' contains less than 2 points" )
		   .arg( ioobj.uiName() );
	return 0;
    }

    ODPolygon<float>* ret = new ODPolygon<float>;
    for ( int idx=0; idx<ps.size(); idx++ )
    {
	const Pick::Location& pl = ps[idx];
	Coord fbid = SI().binID2Coord().transformBackNoSnap( pl.pos() );
	ret->add( Geom::Point2D<float>((float) fbid.x,(float) fbid.y) );
    }

    return ret;
}
