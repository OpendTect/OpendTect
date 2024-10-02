/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "picksetfact.h"
#include "pickset.h"

#include "ascstream.h"
#include "binidvalset.h"
#include "ctxtioobj.h"
#include "datapointset.h"
#include "filepath.h"
#include "iodir.h"
#include "iodirentry.h"
#include "ioman.h"
#include "ioobj.h"
#include "iopar.h"
#include "keystrs.h"
#include "polygon.h"
#include "ptrman.h"
#include "separstr.h"
#include "streamconn.h"
#include "strmprov.h"
#include "survinfo.h"
#include "unitofmeasure.h"

mDefSimpleTranslatorioContext( PickSet, Loc )
mDefSimpleTranslatorSelector( PickSet )

bool PickSetTranslator::retrieve( Pick::Set& ps, const IOObj* ioobj,
				  bool checkdir, uiString& errmsg )
{
    if ( !ioobj )
    {
	errmsg = uiStrings::phrCannotFindObjInDB();
	return false;
    }

    if ( !ioobj->implExists(true) )
    {
	errmsg = tr("No data exists for this object in the database");
	return false;
    }

    mDynamicCast(PickSetTranslator*,PtrMan<PickSetTranslator> trnsl,
		 ioobj->createTranslator());
    if ( !trnsl )
    {
	errmsg = tr("Selected object is not a PointSet");
	return false;
    }

    PtrMan<Conn> conn = ioobj->getConn( Conn::Read );
    if ( !conn )
    {
	errmsg = uiStrings::phrCannotOpen( ioobj->fullUserExpr(), true );
	return false;
    }

    errmsg = trnsl->read( ps, *conn, checkdir );
    IOPar disppars;
    const MultiID& mid = ioobj->key();
    if ( Pick::Mgr().readDisplayPars(mid,disppars) )
	ps.useDisplayPars( disppars );
    else
	ps.setDefaultDispPars();

    return errmsg.isEmpty();
}


bool PickSetTranslator::store( const Pick::Set& ps, const IOObj* ioobj,
							    uiString& errmsg )
{
    if ( !ioobj )
    {
	errmsg = uiStrings::phrCannotFindObjInDB();
	return false;
    }

    mDynamicCast(PickSetTranslator*,PtrMan<PickSetTranslator> trnsl,
		 ioobj->createTranslator());
    if ( !trnsl )
    {
	errmsg = tr("Selected object is not a PointSet");
	return false;
    }

    errmsg.setEmpty();
    PtrMan<Conn> conn = ioobj->getConn( Conn::Write );
    if ( conn )
	errmsg = trnsl->write(ps, *conn);
    else
	errmsg = uiStrings::phrCannotOpen( ioobj->fullUserExpr(false), false );

    IOPar disppars;
    ps.fillDisplayPars( disppars );
    Pick::Mgr().writeDisplayPars( ioobj->key(), disppars );

    FileMultiString pstype;
    ioobj->pars().get( sKey::Type(), pstype );
    if ( pstype.isEmpty() || pstype.size() > 1 )
    {
	ioobj->pars().set( sKey::Type(), ps.isPolygon() ? sKey::Polygon()
							: sKey::PickSet() );
	IOM().commitChanges( *ioobj );
    }

    const bool isok = errmsg.isEmpty();
    if ( isok )
	IOM().implUpdated.trigger( ioobj->key() );

    return isok;
}


uiString dgbPickSetTranslator::read( Pick::Set& ps, Conn& conn,
					bool checkdir )
{
    if ( !conn.forRead() || !conn.isStream() )
    {
	pErrMsg("Internal error: bad connection");
	return uiStrings::phrCannotConnectToDB();
    }

    ascistream astrm( ((StreamConn&)conn).iStream() );
    if ( !astrm.isOK() )
	return tr("Cannot read from PointSet file");
    else if ( !astrm.isOfFileType(mTranslGroupName(PickSet)) )
	return tr("Input file is not a PointSet");
    if ( atEndOfSection(astrm) )
	astrm.next();
    if ( atEndOfSection(astrm) )
	return tr("Input file contains no pick sets");

    ps.setName( IOM().nameOf(conn.linkedTo()) );

    if ( astrm.hasKeyword("Ref") ) // Keep support for pre v3.2 format
    {
	// In old format we can find multiple pick sets. Just gather them all
	// in the pick set
	do
	{
	    astrm.next();
	    if ( astrm.hasKeyword(sKey::Color()) )
	    {
		ps.disp_.color_.use( astrm.value() );
		ps.disp_.color_.setTransparency( 0 );
		astrm.next();
	    }
	    if ( astrm.hasKeyword(sKey::Size()) )
	    {
		ps.disp_.pixsize_ = astrm.getIValue();
		astrm.next();
	    }
	    if ( astrm.hasKeyword(Pick::Set::sKeyMarkerType()) )
	    {
		ps.disp_.markertype_ = astrm.getIValue();
		astrm.next();
	    }
	    while ( !atEndOfSection(astrm) )
	    {
		Pick::Location loc( Coord3::udf() );
		if ( !loc.fromString(astrm.keyWord()) )
		    break;

		ps.add( loc );
		astrm.next();
	    }
	    while ( !atEndOfSection(astrm) ) astrm.next();
	    astrm.next();
	} while( !atEndOfSection(astrm) );
    }
    else // New format
    {
	IOPar iopar;
	iopar.getFrom( astrm );
	if ( !iopar.isEmpty() )
	    ps.usePar( iopar );

	const OD::GeomSystem gs( ps.geomSystem() );
	astrm.next();
	while ( !atEndOfSection(astrm) )
	{
	    Pick::Location loc( Coord3::udf() );
	    loc.setGeomSystem( gs, false );
	    if ( loc.fromString(astrm.keyWord()) )
		ps.add( loc );

	    astrm.next();
	}
    }

    return uiString::empty();
}


uiString dgbPickSetTranslator::write( const Pick::Set& ps, Conn& conn )
{
    if ( !conn.forWrite() || !conn.isStream() )
    {
	pErrMsg("Internal error: bad connection");
	return uiStrings::phrCannotConnectToDB();
    }

    ascostream astrm( ((StreamConn&)conn).oStream() );
    astrm.putHeader( mTranslGroupName(PickSet) );
    if ( !astrm.isOK() )
	return tr("Cannot write to output PointSet file");

    BufferString str;
    IOPar par;
    ps.fillPar( par );
    par.putTo( astrm );

    od_ostream& strm = astrm.stream();
    for ( int iloc=0; iloc<ps.size(); iloc++ )
    {
	ps.get(iloc).toString( str );
	strm << str << od_newline;
    }

    astrm.newParagraph();
    return astrm.isOK() ? uiString::empty()
			:  tr("Error during write to output PointSet file");
}


void PickSetTranslator::createBinIDValueSets(
			const TypeSet<MultiID>& ioobjids,
			ObjectSet<BinIDValueSet>& bivsets )
{
    for ( int idx=0; idx<ioobjids.size(); idx++ )
    {
	TypeSet<Coord3> crds;
	if ( !getCoordSet(ioobjids.get(idx),crds) )
	    continue;

	BinIDValueSet* bs = new BinIDValueSet( 1, true );
	bivsets += bs;

	for ( int ipck=0; ipck<crds.size(); ipck++ )
	{
	    const Coord3& crd( crds[ipck] );
            bs->add( SI().transform(crd), (float) crd.z_ );
	}
    }
}


void PickSetTranslator::createDataPointSets( const TypeSet<MultiID>& ioobjids,
					     RefObjectSet<DataPointSet>& dpss,
					     bool is2d, bool mini )
{
    for ( int idx=0; idx<ioobjids.size(); idx++ )
    {
	const MultiID key = ioobjids.get( idx );
	const int setidx = Pick::Mgr().indexOf( key );
	ConstRefMan<Pick::Set> ps = setidx < 0 ? 0 : Pick::Mgr().get( setidx );
	RefMan<Pick::Set> createdps = nullptr;
	if ( !ps )
	{
	    PtrMan<IOObj>ioobj = IOM().get( key );
	    BufferString msg;
	    if ( !ioobj )
	    {
		msg = "Cannot find PointSet with key "; msg += key;
		ErrMsg( msg );
		continue;
	    }

	    ps = createdps = new Pick::Set;
	    uiString errmsg;
	    if ( !retrieve(*createdps,ioobj,true,errmsg) )
	    {
		msg = errmsg.getString();
		ErrMsg( msg );
		continue;
	    }
	}

	auto* dps = new DataPointSet( is2d, mini );
	if ( !ps->fillDataPointSet(*dps) )
	    continue;

	dpss += dps;
	dps->dataChanged();
    }
}


bool PickSetTranslator::getCoordSet( const MultiID& id, TypeSet<Coord3>& crds )
{
    TypeSet<TrcKey> tks;
    return getCoordSet( id, crds, tks );
}


bool PickSetTranslator::getCoordSet( const MultiID& key, TypeSet<Coord3>& crds,
				     TypeSet<TrcKey>& tks )
{
    const int setidx = Pick::Mgr().indexOf( key );
    ConstRefMan<Pick::Set> ps = setidx < 0 ? 0 : Pick::Mgr().get( setidx );
    RefMan<Pick::Set> createdps = nullptr;
    if ( !ps )
    {
	PtrMan<IOObj>ioobj = IOM().get( key );
	BufferString msg;
	if ( !ioobj )
	{
	    msg = "Cannot find PointSet with key "; msg += key;
	    ErrMsg( msg ); return false;
	}

	ps = createdps = new Pick::Set;
	uiString errmsg;
	if ( !retrieve(*createdps,ioobj,true,errmsg) )
	{
	    msg = errmsg.getString();
	    ErrMsg( msg );
	    return false;
	}
    }

    for ( int ipck=0; ipck<ps->size(); ipck++ )
    {
	const Pick::Location& loc = ps->get( ipck );
	crds += loc.pos();
	tks += loc.trcKey();
    }

    return true;
}


void PickSetTranslator::createBinIDValueSets( const BufferStringSet& ioobjids,
					 ObjectSet<BinIDValueSet>& set )
{
    TypeSet<MultiID> keys;
    for ( const auto* id : ioobjids )
    {
	MultiID key;
	key.fromString( id->buf() );
	keys.add( key );
    }

    createBinIDValueSets( keys, set );
}


void PickSetTranslator::createDataPointSets( const BufferStringSet& ioobjids,
					     RefObjectSet<DataPointSet>& set,
					     bool is2d, bool mini )
{
    TypeSet<MultiID> keys;
    for ( const auto* id : ioobjids )
    {
	MultiID key;
	key.fromString( id->buf() );
	keys.add( key );
    }

    createDataPointSets( keys, set, is2d, mini );
}


bool PickSetTranslator::getCoordSet( const char* ioobjkey,
				     TypeSet<Coord3>& crds )
{
    MultiID key;
    key.fromString( ioobjkey );
    return getCoordSet( key, crds );
}


bool PickSetTranslator::getCoordSet( const char* ioobjkey,
				     TypeSet<Coord3>& crds,
				     TypeSet<TrcKey>& tks )
{
    MultiID key;
    key.fromString( ioobjkey );
    return getCoordSet( key, crds, tks );
}


void PickSetTranslator::fillConstraints( IOObjContext& ctxt, bool ispoly )
{
    ctxt.requireType( ispoly ? sKey::Polygon() : sKey::PickSet() );
}


ODPolygon<float>* PickSetTranslator::getPolygon( const IOObj& ioobj,
						 uiString& emsg )
{
    RefMan<Pick::Set> ps = new Pick::Set;
    uiString errmsg;
    if ( !PickSetTranslator::retrieve(*ps,&ioobj,true,errmsg) )
    {
	emsg = uiStrings::phrCannotRead( ioobj.name() );
	emsg.appendPhrase( errmsg, uiString::MoreInfo );
	return nullptr;
    }
    if ( ps->size() < 2 )
    {
	emsg = tr("Polygon '%1' contains less than 2 points").
							    arg(ioobj.name());
	return nullptr;
    }

    ODPolygon<float>* ret = new ODPolygon<float>;
    for ( int idx=0; idx<ps->size(); idx++ )
    {
	const Pick::Location& pl = ps->get( idx );
	Coord fbid = SI().binID2Coord().transformBackNoSnap( pl.pos() );
        ret->add( Geom::Point2D<float>((float) fbid.x_,(float) fbid.y_) );
    }

    return ret;
}


bool PickSetTranslator::implRemove( const IOObj* ioobj, bool ) const
{
    if ( !ioobj )
	return false;

    const bool res = Translator::implRemove( ioobj );
    if ( !res )
	return false;

    StreamProvider sp( Pick::SetMgr::getDispFileName(ioobj->key()) );
    if ( sp.exists(true) )
	sp.remove();

    return res;
}


bool PickSetTranslator::implRename( const IOObj* ioobj,
				    const char* newnm ) const
{
    if ( !ioobj )
	return false;

    const bool res = Translator::implRename( ioobj, newnm );
    if ( !res )
	return false;

    const FilePath fp( Pick::SetMgr::getDispFileName(ioobj->key()) );
    StreamProvider oldsp( fp.fullPath().buf() );
    FilePath newfp( newnm );
    newfp.setExtension( fp.extension() );
    if ( oldsp.exists(true) )
	oldsp.rename( newfp.fullPath().buf() );

    const int setidx = Pick::Mgr().indexOf( ioobj->key() );
    if ( setidx>= 0 )
    {
	RefMan<Pick::Set> ps = Pick::Mgr().get( setidx );
	ps->setName( ioobj->name() );
    }

    return res;
}


void PickSetTranslator::tagLegacyPickSets()
{
    IOObjContext ctxt = mIOObjContext( PickSet );
    ctxt.fixTranslator( dgbPickSetTranslator::translKey() );
    const IODir iodir( ctxt.getSelKey() );
    const ObjectSet<IOObj>& objs = iodir.getObjs();
    bool haschanges = false;
    for ( auto ioobj : objs )
    {
	if ( !ioobj || !ctxt.validIOObj(*ioobj) ||
					ioobj->pars().hasKey(sKey::Type()) )
	    continue;

	ioobj->pars().set( sKey::Type(), sKey::PickSet() );
	haschanges = true;
    }

    if ( haschanges )
	iodir.doWrite();
}


RefMan<Pick::Set> Pick::getSet( const MultiID& mid, uiString& errmsg )
{
    PtrMan<IOObj> ioobj = IOM().get( mid );
    const int setidx = Pick::Mgr().indexOf( mid );
    if ( setidx<0 )
    {
	RefMan<Pick::Set> ps = new Pick::Set;
	if ( PickSetTranslator::retrieve(*ps,ioobj,true,errmsg) )
	{
	    Pick::Mgr().set( mid, ps );
	    return ps;
	}

	return nullptr;
    }

    return Pick::Mgr().get(setidx);
}



RefMan<Pick::Set> Pick::getSet( const DBKey& key, uiString& errmsg )
{
    if ( !key.hasSurveyLocation() )
	return getSet( sCast(const MultiID&,key), errmsg );

    SurveyChanger chgr( key.surveyDiskLocation() );
    PtrMan<IOObj> ioobj = IODir::getObj( key );
    if ( !ioobj )
	return nullptr;

    RefMan<Pick::Set> ps = new Pick::Set;
    if ( PickSetTranslator::retrieve(*ps,ioobj,true,errmsg) )
	return ps;

    return nullptr;
}
