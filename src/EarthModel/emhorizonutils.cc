/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Helene Payraudeau
 Date:          September 2005
________________________________________________________________________

-*/
static const char* rcsID = "$Id: emhorizonutils.cc,v 1.25 2010/12/08 11:52:33 cvsnageswara Exp $";

#include "emhorizonutils.h"

#include "binidvalset.h"
#include "cubesampling.h"
#include "datapointset.h"

#include "emmanager.h"
#include "emhorizon3d.h"
#include "emhorizon2d.h"
#include "emsurfacegeometry.h"
#include "emsurfaceauxdata.h"
#include "parametricsurface.h"
#include "posprovider.h"
#include "progressmeter.h"
#include "survinfo.h"
#include "surv2dgeom.h"

#define mMaxSampInterpol	150;


namespace EM
{
    
float HorizonUtils::getZ( const RowCol& rc, const Surface* surface )
{
    const SubID subid = rc.toInt64();

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


void HorizonUtils::getExactCoords( std::ostream& strm, const MultiID& id,
				   const PosInfo::GeomID& geomid,
				   const HorSampling& hsamp,
				   ObjectSet<DataPointSet>& data )
{
    Surface* surface = getSurface(id);
    if ( !surface ) return;

    mDynamicCastGet(Horizon2D*,hor2d,surface);

    strm << "\nFetching surface positions ...\n" ;
    TextStreamProgressMeter pm( strm );
    deepErase( data );

    DataPointSet* res = 0;
    if ( hor2d && geomid.isOK() )
    {
	TypeSet<DataPointSet::DataRow> pts;
	BufferStringSet nms;
	res = new DataPointSet( pts, nms, true );
	data += res;
	SectionID sid = 0; 		//multiple sections not used here
	for ( int idx=hsamp.start.crl; idx<=hsamp.stop.crl; idx++ )
	{
	    Coord3 coords = hor2d->getPos( sid, geomid, idx);
	    DataPointSet::Pos newpos( coords );
	    DataPointSet::DataRow dtrow( newpos );
	    res->addRow( dtrow );
	}
    }
    else
    {
	PtrMan<EMObjectIterator> iterator = surface->createIterator(-1);
	SectionID sid = -1;
	//multiple sections not used!!
	while ( iterator )
	{
	    const EM::PosID pid = iterator->next();
	    if ( pid.objectID()==-1 )
		break;

	    if ( pid.sectionID() != sid )
	    {
		TypeSet<DataPointSet::DataRow> pts;
		BufferStringSet nms;
		res = new DataPointSet( pts, nms, true );
		data += res;
		sid = pid.sectionID();
	    }

	    const Coord3 crd = surface->getPos( pid );
	    DataPointSet::Pos newpos( crd );
	    DataPointSet::DataRow dtrow( newpos );
	    res->addRow( dtrow );
	    ++pm;
	}
    }

    if ( res ) res->dataChanged();
    
    pm.setFinished();
    strm << "Done!" << std::endl;
}


void HorizonUtils::getWantedPositions( std::ostream& strm, 
				       ObjectSet<MultiID>& midset,
				       BinIDValueSet& wantedposbivs, 
				       const HorSampling& hs,
				       const Interval<float>& extraz,
				       int nrinterpsamp, int mainhoridx,
				       float extrawidth,
				       Pos::Provider* provider )
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
	    
	    BinID bid( idi,idc );
	    if ( provider )
	    {
		if ( !provider->includes( SI().transform(bid)) )
		    continue;
	    }

	    BinIDValues bidval( bid );
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
	horizon->auxdata.addAuxData( attrnms.get(idx).buf() );

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
	    const SubID subid = RowCol(bid.inl,bid.crl).toInt64();
	    posid.setSubID( subid );
	    for ( int validx=1; validx<vals.size(); validx++ )
		horizon->auxdata.setAuxDataVal( validx-1, posid, vals[validx] );
	}
    }
}


#define mIsEmptyErr( cond, surfid )\
    if ( cond )\
    {\
	strm << "\n Cannot get Positions for surface ID"<<surfid<<" \n";\
	return;\
    }

void HorizonUtils::getWantedPos2D( std::ostream& strm,
				   ObjectSet<MultiID>& midset, 
				   DataPointSet* dtps,
				   const HorSampling& horsamp,
				   const Interval<float>& extraz,
       				   const PosInfo::GeomID& geomid )
{
    ObjectSet<DataPointSet> possurf0;
    ObjectSet<DataPointSet> possurf1;
    getExactCoords( strm, *(midset[0]), geomid, horsamp, possurf0 );
    bool use2hor = midset.size() == 2;

    if ( use2hor )
	getExactCoords( strm, *(midset[1]), geomid, horsamp, possurf1 );

    mIsEmptyErr( possurf0.isEmpty(), *(midset[0]) )
    mIsEmptyErr( use2hor && possurf1.isEmpty(), *(midset[1]) )

    //Remark: multiple sections for the same horizon not fully used here;
    //	  loop over the different sections but use only the first Z 
    //	  found for each BinID
    for ( int secsurf0=0; secsurf0<possurf0.size(); secsurf0++ )
    {
	for (int ptsurf0=0; ptsurf0<possurf0[secsurf0]->size(); ptsurf0++)
	{
	    const Coord coordsurf0 = possurf0[secsurf0]->coord( ptsurf0 );
	    if ( use2hor )
	    {
		for ( int secsurf1=0; secsurf1<possurf1.size(); secsurf1++ )
		{
		    DataPointSet::RowID rid = possurf1[secsurf1]->
						findFirst( coordsurf0 );
		    if ( rid > -1 )
		    {
			const float z0 = possurf0[secsurf0]->z( ptsurf0 );
			const float z1 = possurf1[secsurf1]->z( rid );
			const float ztop = (z0>z1 ? z1 : z0) + extraz.start;
			const float zbot = (z0>z1 ? z0 : z1) + extraz.stop;
			DataPointSet::Pos pos( coordsurf0, ztop );
			DataPointSet::DataRow dtrow( pos );
			dtrow.data_ += zbot;
			dtps->addRow( dtrow );
			break;
		    }
		}
	    }
	    else
	    {
		const float z0 = possurf0[secsurf0]->z( ptsurf0 );
		const float ztop = z0 + extraz.start;
		const float zbot = z0 + extraz.stop;
		DataPointSet::Pos pos( coordsurf0, ztop );
		DataPointSet::DataRow dtrow( pos );
		dtrow.data_ += zbot;
		dtps->addRow( dtrow );
	    }
	}	
    }
    
    dtps->dataChanged();
}


};//namespace
