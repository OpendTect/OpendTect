/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bruno
 * DATE     : Sept 2010
-*/

static const char* rcsID = "$Id: stratreftree.cc,v 1.3 2010-09-08 13:27:39 cvsbruno Exp $";


#include "stratreftree.h"

#include "ascstream.h"
#include "iopar.h"
#include "keystrs.h"
#include "sorting.h"
#include "stratlith.h"
#include "stratunitrepos.h"

static const char* sKeyGeneral = "General";
static const char* sKeyUnits = "Units";


namespace Strat
{

bool RefTree::addUnit( const char* code, const char* dumpstr, bool rev )
{
    if ( !code || !*code )
	use( dumpstr );

    CompoundKey ck( code );
    UnitRef* par = find( ck.upLevel().buf() );
    if ( !par || par->isLeaf() )
	return false;

    const bool isleaf = strchr( dumpstr, '`' ); // a bit of a hack, really
    const BufferString ky( ck.key( ck.nrKeys()-1 ) );
    NodeUnitRef* parnode = (NodeUnitRef*)par;
    UnitRef* newun = isleaf ? (UnitRef*)new LeafUnitRef( parnode, ky )
			    : (UnitRef*)new NodeUnitRef( parnode, ky );
    if ( !newun->use(dumpstr) )
	{ delete newun; return false; }

    newun->acquireID();
    parnode->add( newun, rev );
    return true;
}


bool RefTree::addCopyOfUnit( const UnitRef& ur, bool rev )
{
    BufferString str;
    ur.fill( str );
    return addUnit( ur.fullCode(), str );
}


int RefTree::getID( const char* code ) const
{
    const UnitRef* ur = find( code );
    return ur? ur->getID() : -1;
}


void RefTree::getUnitIDs( TypeSet<int>& ids ) const
{
    ids.erase();
    UnitRef::Iter it( *this );
    UnitRef* un = it.unit();
    while ( un )
    {
	ids += un->getID();
	if ( !it.next() ) break;
	un = it.unit();
    }
}


UnitRef* RefTree::fnd( int id ) const 
{
    UnitRef::Iter it( *this );
    const UnitRef* un = it.unit();
    while ( un )
    {
	un = it.unit();
	if ( un->getID() == id )
	    return const_cast<UnitRef*>(un);
	if ( !it.next() ) break;
	un = it.unit();
    }
    return 0;
}


void RefTree::gatherLeavesByTime( const NodeUnitRef& un, 
					ObjectSet<UnitRef>& refunits ) const
{
    TypeSet<float> timestarts; TypeSet<int> sortedidxs;
    for ( int idunit=0; idunit<un.nrRefs(); idunit++ )
    {
	IOPar iop; un.ref(idunit).putTo( iop );
	Interval<float> rg; iop.get( sKey::Time, rg );
	timestarts += rg.start;
	sortedidxs += idunit;
    }
    sort_coupled( timestarts.arr(), sortedidxs.arr(), un.nrRefs() );
    for ( int idunit=0; idunit<sortedidxs.size(); idunit++ )
	refunits += const_cast<UnitRef*>( &un.ref(sortedidxs[idunit]) );
}


void RefTree::constrainUnits( UnitRef& ur ) const
{
    mDynamicCastGet(NodeUnitRef*,nur,&ur)
    if ( nur )
	constrainUnitTimes( *nur );
    constrainUnitLvls( ur );
}


#define mSetRangeOverlap(rg1,rg2)\
    if ( rg1.start < rg2.start )\
	rg1.start = rg2.start;\
    if ( rg1.stop > rg2.stop )\
	rg1.stop = rg2.stop;
void RefTree::constrainUnitTimes( NodeUnitRef& un ) const
{
    UnitRef::Iter it( un );
    UnitRef* ur = it.unit();
    while ( ur )
    {
	Interval<float> timerg = ur->timeRange();
	NodeUnitRef* upur = ur->upNode();
	if ( upur && !upur->code().isEmpty() )
	{
	    //parent's times
	    const Interval<float> urtimerg = upur->timeRange();
	    mSetRangeOverlap( timerg, urtimerg )
	    if ( timerg.start >= urtimerg.stop ) 
	    {
		upur->remove( upur->indexOf( ur ) );
		break;
	    }
	    
	    //children's times
	    bool found = false; ObjectSet<UnitRef> refunits;
	    gatherLeavesByTime( *upur, refunits );
	    for ( int idunit=0; idunit<refunits.size(); idunit++ )
	    {
		const UnitRef& un = *refunits[idunit];
		if ( un.code() == ur->code() ) 
		{ found = true; continue; }
		const Interval<float> cmptimerg = un.timeRange();
		if ( !found )
		{
		    if( timerg.start < cmptimerg.stop )
			timerg.start = cmptimerg.stop;
		}
		else 
		{
		    if( timerg.stop > cmptimerg.start )
			timerg.stop = cmptimerg.start;
		}
	    }
	}
	ur->setTimeRange( timerg );
	if ( !it.next() ) break;
	ur = it.unit();
    }
}


void RefTree::getLeavesTimeGaps( const NodeUnitRef& node,
				    TypeSet< Interval<float> >& timergs ) const
{
    if ( node.nrRefs() <= 0 ) return;
    const Interval<float> partimerg = node.timeRange();
    ObjectSet<UnitRef> refunits;
    gatherLeavesByTime( node, refunits );
    const float refstart = refunits[0]->timeRange().start;
    const float refstop = refunits[refunits.size()-1]->timeRange().stop;

    if ( partimerg.start < refstart )
	timergs += Interval<float>( partimerg.start, refstart );
    if ( partimerg.stop > refstop )
	timergs += Interval<float>( refstop, partimerg.stop );

    for ( int iref=0; iref<refunits.size()-1; iref++ )
    {
	const float refstop = refunits[iref]->timeRange().stop;
	const float nextrefstart = refunits[iref+1]->timeRange().start;
	if ( refstop < nextrefstart )
	    timergs += Interval<float>( refstop, nextrefstart );
    }
}


void RefTree::constrainUnitLvls( UnitRef& uref ) const
{
    float timestart = uref.timeRange().start; 
    int lvlid = uref.getLvlID();
    NodeUnitRef* ur = uref.upNode();
    while ( ur && ur->upNode() )
    {
	if( timestart == ur->timeRange().start )
	    ur->setLvlID( lvlid );
	ur = ur->upNode();
    }
    mDynamicCastGet(NodeUnitRef*,un,&uref)
    if ( !un ) return; 
    UnitRef::Iter it( *un );
    UnitRef* lur = it.unit();
    while ( lur )
    {
	if( timestart == lur->timeRange().start )
	    lur->setLvlID( lvlid );
	if ( !it.next() ) break;
	lur = it.unit();
    }
}


void RefTree::assignEqualTimesToUnits( Interval<float> toptimerg ) const
{
    UnitRef* un = const_cast<RefTree*>( this );
    UnitRef::Iter it( *this );
    while ( un )
    {
	Interval<float> timerg( 0, 0 );
	if ( un->upNode() ) 
	{
	    constrainUnitLvls( *un );
	    timerg = un->timeRange(); 
	}
	else
	    timerg = toptimerg;

	if ( !un->isLeaf() ) 
	{
	    const int nrrefs = ((NodeUnitRef*)un)->nrRefs();
	    if ( timerg.width() < nrrefs)
		break;
	    for ( int idx=0; idx<nrrefs; idx++ )
	    {
		UnitRef& ref = ((NodeUnitRef*)un)->ref(idx);
		Interval<float> rg = ref.timeRange();
		rg.start = timerg.start + (float)idx*timerg.width()/(nrrefs);
		rg.stop = timerg.start +(float)(idx+1)*timerg.width()/(nrrefs);
		ref.setTimeRange( rg );
	    }
	}
	if ( un->upNode() && !it.next() ) break;
	un = it.unit();
    }
}


void RefTree::removeEmptyNodes()
{
    UnitRef::Iter it( *this );
    ObjectSet<UnitRef> torem;
    while ( it.next() )
    {
	UnitRef* curun = it.unit();
	if ( !curun->isLeaf() && ((NodeUnitRef*)curun)->nrRefs() < 1 )
	    torem += curun;
    }

    for ( int idx=0; idx<torem.size(); idx++ )
    {
	UnitRef* un = torem[idx];
	NodeUnitRef& par = *un->upNode();
	par.remove( par.indexOf(un) );
    }
}



bool RefTree::write( std::ostream& strm ) const
{
    ascostream astrm( strm );
    const UnitRepository& repo = UnRepo();
    astrm.putHeader( UnitRepository::filetype() );
    astrm.put( sKeyGeneral );
    astrm.put( sKey::Name, treename_ );
    BufferString str;
    Lithology::undef().fill( str );
    astrm.put( UnitRepository::sKeyLith(), str );
    for ( int idx=0; idx<repo.nrLiths(); idx++ )
    {
	const Lithology& lith = repo.lith( idx );
	lith.fill( str );
	astrm.put( UnitRepository::sKeyLith(), str );
    }
    astrm.newParagraph();
    astrm.put( sKeyUnits );
    UnitRef::Iter it( *this );
    if ( !it.unit() ) return strm.good();
    const UnitRef& firstun = *it.unit(); firstun.fill( str );
    astrm.put( firstun.fullCode(), str );
    ObjectSet<const UnitRef> unitrefs;
    unitrefs += &firstun;
    
    while ( it.next() )
    {
	const UnitRef& un = *it.unit(); un.fill( str );
	astrm.put( un.fullCode(), str );
	unitrefs += &un;
    }
    astrm.newParagraph();

    IOPar uniop( UnitRepository::sKeyProp() );
    for ( int idx=0; idx<unitrefs.size(); idx++ )
    {
	uniop.clear();
	unitrefs[idx]->putTo( uniop );
	uniop.putTo( astrm );
    }
    
    astrm.put( UnitRepository::sKeyBottomLvlID() );
    astrm.put( toString( botLvlID() ) );
    
    astrm.newParagraph();
    return strm.good();
}

};// namespace Strat
