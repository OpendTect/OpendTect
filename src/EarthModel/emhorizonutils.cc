/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Helene Payraudeau
 Date:          September 2005
 RCS:           $Id: emhorizonutils.cc,v 1.5 2005-10-12 20:35:33 cvskris Exp $
________________________________________________________________________

-*/

#include "emhorizonutils.h"

#include "cubesampling.h"
#include "progressmeter.h"
#include "binidvalset.h"

#include "emmanager.h"
#include "emsurface.h"
#include "emsurfacegeometry.h"
#include "emsurfaceauxdata.h"
#include "parametricsurface.h"
#include "survinfo.h"

namespace EM
{
    
float HorizonUtils::getZ( const BinID& bid, 
			  const ObjectSet<BinIDValueSet>& bidvalset)
{
    float bottomz=-mUdf(float);
    for( int idx=0; idx<bidvalset.size(); idx++ )
    {
	BinIDValueSet::Pos pos = bidvalset[idx]->findFirst(bid);
	float valz = bidvalset[idx]->valid(bid) ?
		     bidvalset[idx]->getVals(pos)[0] : mUdf(float);
	bottomz = ( !mIsUdf(valz) && valz>bottomz ) ? valz : bottomz;
    }
    return bottomz;
}


float HorizonUtils::getMissingZ( const BinID& bid,
				 const ObjectSet<BinIDValueSet>& bidvalset)
{
    int dist = 1;
    int distfirstinlz = 0;
    int distsecondinlz = 0;
    int distfirstcrlz = 0;
    int distsecondcrlz = 0;
    float neededz = -mUdf(float);
    float firstinlz = -mUdf(float);
    float secondinlz = -mUdf(float);
    float firstcrlz = -mUdf(float);
    float secondcrlz = -mUdf(float);
    
    while ( dist<150 )
    {
	if ( firstinlz == -mUdf(float) )
	{
	    BinID binid = BinID( bid.inl-dist, bid.crl );
	    firstinlz = getZ( binid, bidvalset );
	    if ( firstinlz > -mUdf(float) ) distfirstinlz = dist;
	}
	if ( secondinlz == -mUdf(float) )
	{
	    BinID binid = BinID( bid.inl+dist, bid.crl );
	    secondinlz = getZ( binid, bidvalset );
	    if ( secondinlz > -mUdf(float) ) distsecondinlz = dist;
	}
	if ( firstcrlz == -mUdf(float) )
	{
	    BinID binid = BinID( bid.inl, bid.crl-dist );
	    firstcrlz = getZ( binid, bidvalset );
	    if ( firstcrlz > -mUdf(float) ) distfirstcrlz = dist;
	}
	if ( secondcrlz == -mUdf(float) )
	{
	    BinID binid = BinID( bid.inl, bid.crl+dist );
	    secondcrlz = getZ( binid, bidvalset );
	    if ( secondcrlz > -mUdf(float) ) distsecondcrlz = dist;
	}

	if ( (distfirstcrlz && distsecondcrlz) || 
	     (distfirstinlz && distsecondinlz) )
	    break;
	else
	    dist++;
    }

    if ( distfirstcrlz && distsecondcrlz )
    {
	neededz = firstcrlz + ( ( secondcrlz - firstcrlz) * distfirstcrlz / 
				( distfirstcrlz + distsecondcrlz ) );
    }
    else if ( distfirstinlz && distsecondinlz )
    {
	neededz = firstinlz + ( ( secondinlz - firstinlz) * distfirstinlz / 
				( distfirstinlz + distsecondinlz ) );
    }

    return neededz;
}


Surface* HorizonUtils::getSurface( const MultiID& id )
{
    EMManager& em = EMM();
    const ObjectID objid = em.getObjectID(id);
    EMObject* obj = em.getObject( objid );
    mDynamicCastGet(Surface*,surface,obj)
    return surface;
}


void HorizonUtils::getPositions( std::ostream& strm, const MultiID& id,
				 ObjectSet<BinIDValueSet>& data )
{
    Surface* surface = getSurface(id);
    if ( !surface ) return;

    strm << "\nFetching surface positions ...\n" ;
    ProgressMeter pm( strm );
    deepErase( data );
    const int nrsect = surface->geometry.nrSections();
    for ( int sectionidx=0; sectionidx<nrsect; sectionidx++ )
    {
	const SectionID sectionid = 
	    			surface->geometry.sectionID( sectionidx );
	const Geometry::ParametricSurface* psurf = 
	    			surface->geometry.getSurface(sectionid);

	BinIDValueSet& res = *new BinIDValueSet( 1, false );
	data += &res;

	const int nrnodes = psurf->nrKnots();
	for ( int idy=0; idy<nrnodes; idy++ )
	{
	    const Coord3 crd = psurf->getKnot( psurf->getKnotRowCol(idy) );
	    const BinID bid = SI().transform(crd);
	    res.add( bid, crd.z );
	    ++pm;
	}
    }

    pm.finish();
    strm << "Done!" << std::endl;
}


void HorizonUtils::getWantedPositions( std::ostream& strm, 
				       ObjectSet<MultiID>& midset,
				       BinIDValueSet& wantedposbivs, 
				       const HorSampling& hs,
				       const Interval<float>& extraz )
{
    ObjectSet<BinIDValueSet> bivs;
    getPositions( strm, *(midset[0]), bivs );
    ObjectSet<BinIDValueSet> bivs2;
    
    if ( midset.size() == 2 )
	getPositions( strm, *(midset[1]), bivs2 );
    
    for ( int idy=hs.start.inl; idy<=hs.stop.inl; idy+=SI().inlStep() )
    {
	for ( int idz=hs.start.crl; idz<=hs.stop.crl; idz+=SI().crlStep() )
	{
	    float topz = getZ( BinID(idy,idz), bivs );
	    float botz = bivs2.size() ? getZ( BinID(idy,idz), bivs2 ) : 0;
	    if ( topz == -mUdf(float) )
		topz = getMissingZ( BinID(idy,idz), bivs );
	    if ( botz == -mUdf(float) )
		botz = getMissingZ( BinID(idy,idz), bivs2 );

	    if ( topz == -mUdf(float) || botz == -mUdf(float) )
		continue;

	    BinIDValues bidval( BinID(idy,idz) );
	    bidval.value(0) = ( bivs2.size() && botz<topz ? botz : topz ) 
			      + extraz.start;
	    bidval.value(1) = ( bivs2.size() && botz>topz ? botz : topz ) 
			      + extraz.stop;
	    wantedposbivs.add(bidval);
	}
    }
}


void HorizonUtils::addSurfaceData( const MultiID& id, 
				   const BufferStringSet& attrnms,
				   const ObjectSet<BinIDValueSet>& data )
{
    EMManager& em = EMM();
    const ObjectID objid = em.getObjectID(id);
    EMObject* obj = em.getObject( objid );
    mDynamicCastGet(Surface*,surface,obj)
    if ( !surface ) return;

    surface->auxdata.removeAll();
    for ( int idx=0; idx<attrnms.size(); idx++ )
	surface->auxdata.addAuxData( attrnms.get(idx) );

    for ( int sectionidx=0; sectionidx<data.size(); sectionidx++ )
    {
	const SectionID sectionid = 
	    			surface->geometry.sectionID( sectionidx );
	BinIDValueSet& bivs = *data[sectionidx];

	PosID posid( objid, sectionid );
	BinIDValueSet::Pos pos;
	BinID bid; TypeSet<float> vals;
	while ( bivs.next(pos) )
	{
	    bivs.get( pos, bid, vals );
	    const SubID subid = RowCol(bid.inl,bid.crl).getSerialized();
	    posid.setSubID( subid );
	    for ( int validx=1; validx<vals.size(); validx++ )
		surface->auxdata.setAuxDataVal( validx-1, posid, vals[validx] );
	}
    }
}

};//namespace
