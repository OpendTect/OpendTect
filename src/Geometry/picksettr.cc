/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		Jul 2005
________________________________________________________________________

-*/
static const char* rcsID = "$Id$";

#include "picksetfact.h"
#include "pickset.h"
#include "ctxtioobj.h"
#include "binidvalset.h"
#include "datapointset.h"
#include "ascstream.h"
#include "ioobj.h"
#include "iopar.h"
#include "ptrman.h"
#include "survinfo.h"
#include "streamconn.h"
#include "ioman.h"
#include "polygon.h"
#include "errh.h"
#include "keystrs.h"

mDefSimpleTranslatorioContextWithExtra( PickSet, Loc,
	ctxt->toselect.require_.set( sKey::Type, "PickSet``Polygon" )	)

int PickSetTranslatorGroup::selector( const char* key )
{
    int retval = defaultSelector( theInst().userName(), key );
    if ( retval ) return retval;

    if ( defaultSelector("Locations directory",key) ) return 1;

    return 0;
}


bool PickSetTranslator::retrieve( Pick::Set& ps, const IOObj* ioobj,
				  bool checkdir, BufferString& bs )
{
    if ( !ioobj ) { bs = "Cannot find object in data base"; return false; }
    mDynamicCastGet(PickSetTranslator*,t,ioobj->getTranslator())
    if ( !t ) { bs = "Selected object is not a Pick Set"; return false; }
    PtrMan<PickSetTranslator> tr = t;
    PtrMan<Conn> conn = ioobj->getConn( Conn::Read );
    if ( !conn )
        { bs = "Cannot open "; bs += ioobj->fullUserExpr(true); return false; }
    bs = tr->read( ps, *conn, checkdir );
    return bs.isEmpty();
}


bool PickSetTranslator::store( const Pick::Set& ps, const IOObj* ioobj,
				BufferString& bs )
{
    if ( !ioobj ) { bs = "No object to store set in data base"; return false; }
    mDynamicCastGet(PickSetTranslator*,tr,ioobj->getTranslator())
    if ( !tr ) { bs = "Selected object is not a Pick Set"; return false; }

    bs = "";
    PtrMan<Conn> conn = ioobj->getConn( Conn::Write );
    if ( !conn )
        { bs = "Cannot open "; bs += ioobj->fullUserExpr(false); }
    else
	bs = tr->write( ps, *conn );
    delete tr;
    return bs.isEmpty();
}


const char* dgbPickSetTranslator::read( Pick::Set& ps, Conn& conn,
					bool checkdir )
{
    if ( !conn.forRead() || !conn.isStream() )
	return "Internal error: bad connection";

    ascistream astrm( ((StreamConn&)conn).iStream() );
    std::istream& strm = astrm.stream();
    if ( !strm.good() )
	return "Cannot read from input file";
    if ( !astrm.isOfFileType(mTranslGroupName(PickSet)) )
	return "Input file is not a Pick Set";
    if ( atEndOfSection(astrm) ) astrm.next();
    if ( atEndOfSection(astrm) )
	return "Input file contains no pick sets";

    ps.setName( conn.ioobj ? (const char*)conn.ioobj->name() : "" );

    Pick::Location loc;

    if ( astrm.hasKeyword("Ref") ) // Keep support for pre v3.2 format
    {
	// In old format we can find mulitple pick sets. Just gather them all
	// in the pick set
	for ( int ips=0; !atEndOfSection(astrm); ips++ )
	{
	    astrm.next();
	    if ( astrm.hasKeyword(sKey::Color) )
	    {
		ps.disp_.color_.use( astrm.value() );
		astrm.next();
	    }
	    if ( astrm.hasKeyword(sKey::Size) )
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
		if ( !loc.fromString( astrm.keyWord() ) )
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

	astrm.next();
	while ( !atEndOfSection(astrm) )
	{
	    if ( loc.fromString( astrm.keyWord(), true, checkdir ) )
		ps += loc;

	    astrm.next();
	}
    }

    return ps.size() ? 0 : "No valid picks found";
}


const char* dgbPickSetTranslator::write( const Pick::Set& ps, Conn& conn )
{
    if ( !conn.forWrite() || !conn.isStream() )
	return "Internal error: bad connection";

    ascostream astrm( ((StreamConn&)conn).oStream() );
    astrm.putHeader( mTranslGroupName(PickSet) );
    std::ostream& strm = astrm.stream();
    if ( !strm.good() )
	return "Cannot write to output Pick Set file";

    BufferString str;
    IOPar par;
    ps.fillPar( par );
    par.putTo( astrm );

    for ( int iloc=0; iloc<ps.size(); iloc++ )
    {
	ps[iloc].toString( str );
	strm << str.buf() << '\n';
    }

    astrm.newParagraph();
    return strm.good() ? 0
	:  "Error during write to output Pick Set file";
}


void PickSetTranslator::createBinIDValueSets(
			const BufferStringSet& ioobjids,
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
	    const Coord3& crd( crds[idx] );
	    bs->add( SI().transform(crd), crd.z );
	}
    }
}


void PickSetTranslator::createDataPointSets( const BufferStringSet& ioobjids,
					     ObjectSet<DataPointSet>& dpss,
       					     bool is2d, bool mini )
{
    for ( int idx=0; idx<ioobjids.size(); idx++ )
    {
	TypeSet<Coord3> crds;
	if ( !getCoordSet(ioobjids.get(idx),crds) )
	    continue;

	DataPointSet* dps = new DataPointSet( is2d, mini );
	dpss += dps;

	DataPointSet::DataRow dr;
	for ( int ipck=0; ipck<crds.size(); ipck++ )
	{
	    const Coord3& crd( crds[ipck] );
	    dr.pos_.set( crd );
	    dps->addRow( dr );
	}
	dps->dataChanged();
    }
}


bool PickSetTranslator::getCoordSet( const char* id, TypeSet<Coord3>& crds )
{
    const MultiID key( id );
    const int setidx = Pick::Mgr().indexOf( key );
    const Pick::Set* ps = setidx < 0 ? 0 : &Pick::Mgr().get( setidx );
    Pick::Set* createdps = 0;
    if ( !ps )
    {
	PtrMan<IOObj>ioobj = IOM().get( key );
	BufferString msg;
	if ( !ioobj )
	{
	    msg = "Cannot find PickSet with key "; msg += key;
	    ErrMsg( msg ); return false;
	}
	ps = createdps = new Pick::Set;
	if ( !retrieve(*createdps,ioobj,true,msg) )
	    { ErrMsg( msg ); delete createdps; return false; }
    }

    for ( int ipck=0; ipck<ps->size(); ipck++ )
	crds += ((*ps)[ipck]).pos;

    delete createdps;
    return true;
}


ODPolygon<float>* PickSetTranslator::getPolygon( const IOObj& ioobj,
						 BufferString& emsg )
{
    Pick::Set ps; BufferString msg;
    if ( !PickSetTranslator::retrieve(ps,&ioobj,true,msg) )
    {
	emsg = "Cannot read polygon '"; emsg += ioobj.name();
	emsg += "':\n"; emsg += msg;
	return 0;
    }
    if ( ps.size() < 2 )
    {
	emsg = "Polygon '"; emsg += ioobj.name();
	emsg += "' contains less than 2 points";
	return 0;
    }

    ODPolygon<float>* ret = new ODPolygon<float>;
    for ( int idx=0; idx<ps.size(); idx++ )
    {
	const Pick::Location& pl = ps[idx];
	Coord fbid = SI().binID2Coord().transformBackNoSnap( pl.pos );
	ret->add( Geom::Point2D<float>(fbid.x,fbid.y) );
    }

    return ret;
}
