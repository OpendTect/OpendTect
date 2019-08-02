/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Helene Payraudeau
 Date:          September 2005
________________________________________________________________________

-*/

#include "emhorizonutils.h"

#include "binnedvalueset.h"
#include "binidvalue.h"
#include "trckeysampling.h"
#include "datapointset.h"

#include "emmanager.h"
#include "emhorizon3d.h"
#include "emhorizon2d.h"
#include "emsurfacegeometry.h"
#include "emsurfaceauxdata.h"
#include "parametricsurface.h"
#include "posprovider.h"
#include "progressmeterimpl.h"
#include "survinfo.h"
#include "posinfo2dsurv.h"
#include "od_ostream.h"

#define mMaxSampInterpol	150;


namespace EM
{

float HorizonUtils::getZ( const RowCol& rc, const Surface* surface )
{
    const PosID posid = PosID::getFromRowCol(rc);

    float bottomz=-mUdf(float);
    const float valz = (float) surface->getPos( posid ).z_;
    bottomz = ( !mIsUdf(valz) && valz>bottomz ) ? valz : bottomz;
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
	    RowCol rowcol(rc.row()-dist, rc.col() );
	    firstinlz = getZ( rowcol, surface );
	    if ( firstinlz > -mUdf(float) ) distfirstinlz = dist;
	}
	if ( secondinlz == -mUdf(float) )
	{
	    RowCol rowcol( rc.row()+dist, rc.col() );
	    secondinlz = getZ( rowcol, surface );
	    if ( secondinlz > -mUdf(float) ) distsecondinlz = dist;
	}
	if ( firstcrlz == -mUdf(float) )
	{
	    RowCol rowcol( rc.row(), rc.col()-dist );
	    firstcrlz = getZ( rowcol, surface );
	    if ( firstcrlz > -mUdf(float) ) distfirstcrlz = dist;
	}
	if ( secondcrlz == -mUdf(float) )
	{
	    RowCol rowcol( rc.row(), rc.col()+dist );
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


Surface* HorizonUtils::getSurface( const DBKey& id )
{
    Object* obj = Hor3DMan().getObject( id );
    mDynamicCastGet(Surface*,surface,obj)
    return surface;
}


void HorizonUtils::getPositions( od_ostream& strm, const DBKey& id,
				 ObjectSet<BinnedValueSet>& data )
{
    Surface* surface = getSurface(id);
    if ( !surface ) return;

    strm << "\nFetching surface positions ...\n" ;
    TextStreamProgressMeter pm( strm );
    deepErase( data );

    PtrMan<ObjectIterator> iterator = surface->createIterator();
    BinnedValueSet* res = new BinnedValueSet( 1, false );
    data += res;
    while ( iterator )
    {
	const EM::PosID pid = iterator->next();
	if ( pid.isInvalid() )
	    break;

	const Coord3 crd = surface->getPos( pid );
	const BinID bid = SI().transform(crd.getXY());
	res->add( bid, (float) crd.z_ );
	++pm;
    }

    pm.setFinished();
    strm << "Done!" << od_endl;
}


void HorizonUtils::getExactCoords( od_ostream& strm, const DBKey& id,
			   Pos::GeomID geomid, const TrcKeySampling& hsamp,
			   ObjectSet<DataPointSet>& data )
{
    Surface* surface = getSurface(id);
    if ( !surface ) return;

    mDynamicCastGet(Horizon2D*,hor2d,surface);

    strm << "\nFetching surface positions ...\n" ;
    TextStreamProgressMeter pm( strm );
    deepUnRef( data );

    DataPointSet* res = 0;
    if ( hor2d && geomid.isValid() )
    {
	TypeSet<DataPointSet::DataRow> pts;
	BufferStringSet nms;
	res = new DataPointSet( pts, nms, true );
	data += res;
	for ( int idx=hsamp.start_.crl(); idx<=hsamp.stop_.crl(); idx++ )
	{
	    Coord3 coords = hor2d->getPos( geomid, idx);
	    DataPointSet::Pos newpos( coords );
	    DataPointSet::DataRow dtrow( newpos );
	    res->addRow( dtrow );
	}
    }
    else
    {
	PtrMan<ObjectIterator> iterator = surface->createIterator();
	//multiple sections not used!!
	TypeSet<DataPointSet::DataRow> pts;
	BufferStringSet nms;
	res = new DataPointSet( pts, nms, true );
	data += res;
	while ( iterator )
	{
	    const EM::PosID pid = iterator->next();
	    if ( pid.isInvalid() )
		break;

	    const Coord3 crd = surface->getPos( pid );
	    DataPointSet::Pos newpos( crd );
	    DataPointSet::DataRow dtrow( newpos );
	    res->addRow( dtrow );
	    ++pm;
	}
    }

    if ( res ) res->dataChanged();

    pm.setFinished();
    strm << "Done!" << od_endl;
}


void HorizonUtils::getWantedPositions( od_ostream& strm,
				       DBKeySet& dbkeyset,
				       BinnedValueSet& wantedposbivs,
				       const TrcKeySampling& hs,
				       const Interval<float>& extraz,
				       int nrinterpsamp, int mainhoridx,
				       float extrawidth,
				       Pos::Provider* provider )
{
    Surface* surface1 = getSurface( dbkeyset[0] );
    if ( !surface1 )
	return;

    Surface* surface2 = 0;
    if ( dbkeyset.size() == 2 )
    {
	surface2 = getSurface( dbkeyset[1] );
	if ( !surface2 ) return;
    }

    strm << "\nFetching surface positions ...\n" ;
    TextStreamProgressMeter pm( strm );

    if ( mIsUdf(nrinterpsamp) )
	nrinterpsamp = mMaxSampInterpol;
    float meanzinter = 0;
    int nrpos;
    float topz, botz, lastzinter;
    float vals[2];
    for ( int idi=hs.start_.inl(); idi<=hs.stop_.inl(); idi+=SI().inlStep() )
    {
	for ( int idc=hs.start_.crl();idc<=hs.stop_.crl();idc+=SI().crlStep() )
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

	    vals[0] = ( surface2 && botz<topz ? botz : topz ) + extraz.start;
	    vals[1] = ( surface2 && botz>topz ? botz : topz ) + extraz.stop;
	    wantedposbivs.add( bid, vals );
	    nrpos = mCast( int, wantedposbivs.totalSize() );
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


void HorizonUtils::addSurfaceData( const DBKey& id,
				   const BufferStringSet& attrnms,
				   const ObjectSet<BinnedValueSet>& data )
{
    Object* obj = Hor3DMan().getObject( id );
    mDynamicCastGet(Horizon3D*,horizon,obj)
    if ( !horizon )
	return;

    horizon->auxdata.removeAll();
    for ( int idx=0; idx<attrnms.size(); idx++ )
	horizon->auxdata.addAuxData( attrnms.get(idx).buf() );

    const BinnedValueSet& bivs = *data[0];

    BinnedValueSet::SPos pos;
    BinID bid; TypeSet<float> vals;
    while ( bivs.next(pos) )
    {
	bivs.get( pos, bid, vals );
	const PosID posid = PosID::getFromRowCol( bid );
	for ( int validx=1; validx<vals.size(); validx++ )
	    horizon->auxdata.setAuxDataVal( validx-1, posid, vals[validx] );
    }
}


#define mIsEmptyErr( cond, surfid )\
    if ( cond )\
    {\
	strm << "\n Cannot get Positions for surface ID"<<surfid<<" \n";\
	return;\
    }

void HorizonUtils::getWantedPos2D( od_ostream& strm,
				   DBKeySet& dbkeyset,
				   DataPointSet* dtps,
				   const TrcKeySampling& horsamp,
				   const Interval<float>& extraz,
				   Pos::GeomID geomid )
{
    ObjectSet<DataPointSet> possurf0;
    ObjectSet<DataPointSet> possurf1;
    getExactCoords( strm, dbkeyset[0], geomid, horsamp, possurf0 );
    bool use2hor = dbkeyset.size() == 2;

    if ( use2hor )
	getExactCoords( strm, dbkeyset[1], geomid, horsamp, possurf1 );

    mIsEmptyErr( possurf0.isEmpty(), dbkeyset[0] )
    mIsEmptyErr( use2hor && possurf1.isEmpty(), dbkeyset[1] )

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

} // namespace EM
