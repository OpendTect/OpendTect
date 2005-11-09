/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Helene Payraudeau
 Date:          September 2005
 RCS:           $Id: emhorizonutils.cc,v 1.6 2005-11-09 16:43:37 cvshelene Exp $
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
    
float HorizonUtils::getZ( const RowCol& rc, const Surface* surface )
{
    float bottomz=-mUdf(float);
    TypeSet<Coord3> coords;
    surface->geometry.getPos( rc, coords );
    for( int idx=0; idx<coords.size(); idx++ )
    {
	float valz = coords[idx].z;
	bottomz = ( !mIsUdf(valz) && valz>bottomz ) ? valz : bottomz;
    }
    return bottomz;
}


float HorizonUtils::getMissingZ( const RowCol& rc, const Surface* surface )
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
	    RowCol rowcol(rc.row-dist, rc.col );
	    firstinlz = getZ( rowcol, surface );
	    if ( firstinlz > -mUdf(float) ) distfirstinlz = dist;
	}
	if ( secondinlz == -mUdf(float) )
	{
	    RowCol rowcol( rc.row+dist, rc.col );
	    secondinlz = getZ( rowcol, surface );
	    if ( secondinlz > -mUdf(float) ) distsecondinlz = dist;
	}
	if ( firstcrlz == -mUdf(float) )
	{
	    RowCol rowcol( rc.row, rc.col-dist );
	    firstcrlz = getZ( rowcol, surface );
	    if ( firstcrlz > -mUdf(float) ) distfirstcrlz = dist;
	}
	if ( secondcrlz == -mUdf(float) )
	{
	    RowCol rowcol( rc.row, rc.col+dist );
	    secondcrlz = getZ( rowcol, surface );
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
    Surface* surface1 = getSurface(*(midset[0]));
    if ( !surface1 ) return;

    Surface* surface2 = 0;
    if ( midset.size() == 2 )
    {
	surface2 = getSurface(*(midset[1]));
	if ( !surface2 ) return;
    }
    
    strm << "\nFetching surface positions ...\n" ;
    ProgressMeter pm( strm );
    
    for ( int idy=hs.start.inl; idy<=hs.stop.inl; idy+=SI().inlStep() )
    {
	for ( int idz=hs.start.crl; idz<=hs.stop.crl; idz+=SI().crlStep() )
	{
	    float topz = getZ( RowCol(idy,idz), surface1 );
	    float botz = surface2 ? getZ( RowCol(idy,idz), surface2 ) : 0;
	    if ( topz == -mUdf(float) )
		topz = getMissingZ( RowCol(idy,idz), surface1 );
	    if ( botz == -mUdf(float) )
		botz = getMissingZ( RowCol(idy,idz), surface2 );

	    if ( topz == -mUdf(float) || botz == -mUdf(float) )
		continue;

	    BinIDValues bidval( BinID(idy,idz) );
	    bidval.value(0) = ( surface2 && botz<topz ? botz : topz ) 
			      + extraz.start;
	    bidval.value(1) = ( surface2 && botz>topz ? botz : topz ) 
			      + extraz.stop;
	    wantedposbivs.add(bidval);
	    ++pm;
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
