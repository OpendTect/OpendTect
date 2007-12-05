/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Helene Payraudeau
 Date:          September 2005
 RCS:           $Id: emhorizonutils.cc,v 1.12 2007-12-05 21:44:20 cvskris Exp $
________________________________________________________________________

-*/

#include "emhorizonutils.h"

#include "cubesampling.h"
#include "progressmeter.h"
#include "binidvalset.h"

#include "emmanager.h"
#include "emhorizon3d.h"
#include "emsurfacegeometry.h"
#include "emsurfaceauxdata.h"
#include "parametricsurface.h"
#include "survinfo.h"

#define mMaxSampInterpol	150;

namespace EM
{
    
float HorizonUtils::getZ( const RowCol& rc, const Surface* surface )
{
    const SubID subid = rc.getSerialized();

    float bottomz=-mUdf(float);
    for ( int idx=surface->nrSections()-1; idx>=0; idx-- )
    {
	const EM::SectionID sid = surface->sectionID( idx );
	const float valz = surface->getPos( sid, subid ).z; 
	bottomz = ( !mIsUdf(valz) && valz>bottomz ) ? valz : bottomz;
    }

    return bottomz;
}


float HorizonUtils::getMissingZ( const RowCol& rc, const Surface* surface,
				 int nrptinterp )
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
    
    while ( dist<nrptinterp )
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
    TextStreamProgressMeter pm( strm );
    deepErase( data );

    PtrMan<EMObjectIterator> iterator = surface->createIterator(-1);
    SectionID sid = -1;
    BinIDValueSet* res = 0;
    while ( iterator )
    {
	const EM::PosID pid = iterator->next();
	if ( pid.objectID()==-1 )
	    break;

	if ( pid.sectionID() != sid )
	{	
	    res = new BinIDValueSet( 1, false );
	    data += res;
	    sid = pid.sectionID();
	}

	const Coord3 crd = surface->getPos( pid );
	const BinID bid = SI().transform(crd);
	res->add( bid, crd.z );
	++pm;
    }

    pm.setFinished();
    strm << "Done!" << std::endl;
}


void HorizonUtils::getWantedPositions( std::ostream& strm, 
				       ObjectSet<MultiID>& midset,
				       BinIDValueSet& wantedposbivs, 
				       const HorSampling& hs,
				       const Interval<float>& extraz,
				       int nrinterpsamp, int mainhoridx,
				       float extrawidth )
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
    TextStreamProgressMeter pm( strm );
   
    if ( mIsUdf(nrinterpsamp) )
	nrinterpsamp = mMaxSampInterpol;
    float meanzinter = 0;
    int nrpos;
    float topz, botz, lastzinter;
    for ( int idi=hs.start.inl; idi<=hs.stop.inl; idi+=SI().inlStep() )
    {
	for ( int idc=hs.start.crl; idc<=hs.stop.crl; idc+=SI().crlStep() )
	{
	    lastzinter = meanzinter;
	    if ( !getZInterval( idi, idc, surface1, surface2, topz, botz, 
				nrinterpsamp, mainhoridx, lastzinter, 
				extrawidth ) )
		continue;
	    
	    BinIDValues bidval( BinID(idi,idc) );
	    bidval.value(0) = ( surface2 && botz<topz ? botz : topz ) 
			      + extraz.start;
	    bidval.value(1) = ( surface2 && botz>topz ? botz : topz ) 
			      + extraz.stop;
	    wantedposbivs.add(bidval);
	    nrpos = wantedposbivs.totalSize();
	    meanzinter = ( meanzinter*( nrpos -1 ) + lastzinter) / nrpos;
	    ++pm;
	}
    }
}


bool HorizonUtils::getZInterval( int idi, int idc, 
				 Surface* surface1, Surface* surface2, 
				 float& topz, float& botz, 
				 int nrinterpsamp, int mainhoridx,
				 float& lastzinterval, float extrawidth )
{
    topz = getZ( RowCol(idi,idc), surface1 );
    botz = surface2 ? getZ( RowCol(idi,idc), surface2 ) : 0;
    
    bool is1interp, is2interp;
    is1interp = is2interp = false;
    bool is1main = ( surface2 && mainhoridx==1 ) ? true : false;

    if ( fabs(topz) != mUdf(float) && fabs(botz) != mUdf(float) )
	lastzinterval = botz - topz;
    
    if ( topz == -mUdf(float) && nrinterpsamp )
    {
	topz = getMissingZ( RowCol(idi,idc), surface1, nrinterpsamp );
	is1interp = true;
    }
    if ( botz == -mUdf(float) && nrinterpsamp)
    {
	botz = getMissingZ( RowCol(idi,idc), surface2, nrinterpsamp );
	is2interp = true;
    }

    if ( topz == -mUdf(float) || botz == -mUdf(float) )
    {
	if ( !surface2 || mIsZero(extrawidth,0.01) 
	     || ( extrawidth && topz == -mUdf(float) && is1main )
	     || ( extrawidth && botz == -mUdf(float) && !is1main ) )
	    return false;
	
	if ( topz == -mUdf(float) )
	    topz = botz - extrawidth;
	else if ( botz == -mUdf(float) )
	    botz = topz + extrawidth;
    }

    bool isintersect = ( lastzinterval >= 0 && ( botz - topz ) < 0 ) ||
     		       ( lastzinterval <= 0 && ( botz - topz ) > 0 );

    if ( surface2 && isintersect )
	return SolveIntersect( topz, botz, nrinterpsamp, is1main, extrawidth, 
			       is1interp, is2interp );

    return true;
}


bool HorizonUtils::SolveIntersect( float& topz, float& botz, int nrinterpsamp,
				   int is1main, float extrawidth,
				   bool is1interp, bool is2interp )
{
    bool bothinterp = is1interp && is2interp;
    if ( is1main && (  bothinterp || !is1interp ) )
	botz = topz + extrawidth;
    else if ( !is1main && (  bothinterp || !is2interp ) )
	topz = botz - extrawidth;
    else
	return false;

    return true;
}


void HorizonUtils::addSurfaceData( const MultiID& id, 
				   const BufferStringSet& attrnms,
				   const ObjectSet<BinIDValueSet>& data )
{
    EMManager& em = EMM();
    const ObjectID objid = em.getObjectID(id);
    EMObject* obj = em.getObject( objid );
    mDynamicCastGet(Horizon3D*,horizon,obj)
    if ( !horizon ) return;

    horizon->auxdata.removeAll();
    for ( int idx=0; idx<attrnms.size(); idx++ )
	horizon->auxdata.addAuxData( attrnms.get(idx) );

    for ( int sectionidx=0; sectionidx<data.size(); sectionidx++ )
    {
	const SectionID sectionid = horizon->sectionID( sectionidx );
	const BinIDValueSet& bivs = *data[sectionidx];

	PosID posid( objid, sectionid );
	BinIDValueSet::Pos pos;
	BinID bid; TypeSet<float> vals;
	while ( bivs.next(pos) )
	{
	    bivs.get( pos, bid, vals );
	    const SubID subid = RowCol(bid.inl,bid.crl).getSerialized();
	    posid.setSubID( subid );
	    for ( int validx=1; validx<vals.size(); validx++ )
		horizon->auxdata.setAuxDataVal( validx-1, posid, vals[validx] );
	}
    }
}

};//namespace
