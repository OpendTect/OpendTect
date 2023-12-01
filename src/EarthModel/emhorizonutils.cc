/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "emhorizonutils.h"

#include "binidvalset.h"
#include "trckeyzsampling.h"
#include "datapointset.h"

#include "emioobjinfo.h"
#include "emmanager.h"
#include "emhorizon3d.h"
#include "emhorizon2d.h"
#include "emsurfaceauxdata.h"
#include "emsurfacetr.h"
#include "iodir.h"
#include "iodirentry.h"
#include "ioobj.h"
#include "posprovider.h"
#include "progressmeterimpl.h"
#include "survinfo.h"
#include "od_ostream.h"

#define mMaxSampInterpol	150;


namespace EM
{

HorizonSelInfo::HorizonSelInfo( const MultiID& key )
    : key_(key)
{}


HorizonSelInfo::~HorizonSelInfo()
{}


void HorizonSelInfo::getAll( ObjectSet<HorizonSelInfo>& set, bool is2d )
{
    IOObjContext ctxt = EM::Horizon::ioContext( is2d, true );
    IODir iodir( ctxt.getSelKey() );
    IODirEntryList entries( iodir, ctxt );
    for ( int idx=0; idx<entries.size(); idx++ )
    {
	const IOObj* ioobj = entries[idx]->ioobj_;
	if ( !ioobj || ioobj->translator() != "dGB" )
	    continue;

	HorizonSelInfo* info = new HorizonSelInfo( ioobj->key() );
	info->name_ = ioobj->name();
	set += info;

	uiString errmsg;
	EM::IOObjInfo eminfo( ioobj->key() );
	eminfo.getSurfaceData( info->iodata_, errmsg );
    }
}



// HorizonUtils
float HorizonUtils::getZ( const RowCol& rc, const Horizon* horizon )
{
    return horizon ? horizon->getZ( TrcKey(rc) ) : mUdf(float);
}


float HorizonUtils::getMissingZ( const RowCol& rc, const Horizon* horizon,
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
	    firstinlz = getZ( rowcol, horizon );
	    if ( firstinlz > -mUdf(float) ) distfirstinlz = dist;
	}
	if ( secondinlz == -mUdf(float) )
	{
	    RowCol rowcol( rc.row()+dist, rc.col() );
	    secondinlz = getZ( rowcol, horizon );
	    if ( secondinlz > -mUdf(float) ) distsecondinlz = dist;
	}
	if ( firstcrlz == -mUdf(float) )
	{
	    RowCol rowcol( rc.row(), rc.col()-dist );
	    firstcrlz = getZ( rowcol, horizon );
	    if ( firstcrlz > -mUdf(float) ) distfirstcrlz = dist;
	}
	if ( secondcrlz == -mUdf(float) )
	{
	    RowCol rowcol( rc.row(), rc.col()+dist );
	    secondcrlz = getZ( rowcol, horizon );
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


Horizon* HorizonUtils::getHorizon( const MultiID& id )
{
    EMManager& em = EMM();
    const ObjectID objid = em.getObjectID(id);
    EMObject* obj = em.getObject( objid );
    mDynamicCastGet(Horizon*,horizon,obj)
    return horizon;
}


void HorizonUtils::getPositions( od_ostream& strm, const MultiID& id,
				 ObjectSet<BinIDValueSet>& data )
{
    Horizon* horizon = getHorizon(id);
    if ( !horizon ) return;

    strm << "\nFetching horizon positions ...\n" ;
    TextStreamProgressMeter pm( strm );
    deepErase( data );

    auto* res = new BinIDValueSet( 1, false );
    data += res;

    PtrMan<EMObjectIterator> iterator = horizon->createIterator();
    while ( iterator )
    {
	const EM::PosID pid = iterator->next();
	if ( !pid.objectID().isValid() )
	    break;

	const Coord3 crd = horizon->getPos( pid );
	const BinID bid = SI().transform(crd);
	res->add( bid, (float) crd.z );
	++pm;
    }

    pm.setFinished();
    strm << "Fetching horizon positions Done!\n\n" << od_endl;
}


void HorizonUtils::getExactCoords( od_ostream& strm, const MultiID& id,
			   Pos::GeomID geomid, const TrcKeySampling& hsamp,
			   ObjectSet<DataPointSet>& data )
{
    Horizon* horizon = getHorizon(id);
    if ( !horizon ) return;

    mDynamicCastGet(Horizon2D*,hor2d,horizon);

    strm << "\nFetching horizon positions ...\n" ;
    TextStreamProgressMeter pm( strm );

    DataPointSet* res = 0;
    if ( hor2d && geomid != Survey::GeometryManager::cUndefGeomID() )
    {
	TypeSet<DataPointSet::DataRow> pts;
	BufferStringSet nms;
	res = new DataPointSet( pts, nms, true );
	data += res;
	for ( int idx=hsamp.start_.crl(); idx<=hsamp.stop_.crl(); idx++ )
	{
	    Coord3 coords = hor2d->getCoord( TrcKey(geomid,idx) );
	    DataPointSet::Pos newpos( coords );
	    DataPointSet::DataRow dtrow( newpos );
	    res->addRow( dtrow );
	}
    }
    else
    {
	PtrMan<EMObjectIterator> iterator = horizon->createIterator();
	while ( iterator )
	{
	    const EM::PosID pid = iterator->next();
	    if ( !pid.isValid() )
		break;

	    TypeSet<DataPointSet::DataRow> pts;
	    BufferStringSet nms;
	    res = new DataPointSet( pts, nms, true );
	    data += res;

	    const Coord3 crd = horizon->getPos( pid );
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
				       ObjectSet<MultiID>& midset,
				       BinIDValueSet& wantedposbivs,
				       const TrcKeySampling& hs,
				       const Interval<float>& extraz,
				       int nrinterpsamp, int mainhoridx,
				       float extrawidth,
				       Pos::Provider* provider )
{
    Horizon* horizon1 = getHorizon(*(midset[0]));
    if ( !horizon1 ) return;

    Horizon* horizon2 = 0;
    if ( midset.size() == 2 )
    {
	horizon2 = getHorizon(*(midset[1]));
	if ( !horizon2 ) return;
    }

    strm << "\nFetching horizon positions ...\n" ;
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
	    if ( !getZInterval( idi, idc, horizon1, horizon2, topz, botz,
				nrinterpsamp, mainhoridx, lastzinter,
				extrawidth ) )
		continue;

	    BinID bid( idi,idc );
	    if ( provider )
	    {
		if ( !provider->includes( SI().transform(bid)) )
		    continue;
	    }

	    vals[0] = ( horizon2 && botz<topz ? botz : topz ) + extraz.start;
	    vals[1] = ( horizon2 && botz>topz ? botz : topz ) + extraz.stop;
	    wantedposbivs.add( bid, vals );
	    nrpos = mCast( int, wantedposbivs.totalSize() );
	    meanzinter = ( meanzinter*( nrpos -1 ) + lastzinter) / nrpos;
	    ++pm;
	}
    }
}


bool HorizonUtils::getZInterval( int idi, int idc,
				 Horizon* horizon1, Horizon* horizon2,
				 float& topz, float& botz,
				 int nrinterpsamp, int mainhoridx,
				 float& lastzinterval, float extrawidth )
{
    topz = getZ( RowCol(idi,idc), horizon1 );
    botz = horizon2 ? getZ( RowCol(idi,idc), horizon2 ) : 0;

    bool is1interp, is2interp;
    is1interp = is2interp = false;
    bool is1main = ( horizon2 && mainhoridx==1 ) ? true : false;

    if ( fabs(topz) != mUdf(float) && fabs(botz) != mUdf(float) )
	lastzinterval = botz - topz;

    if ( topz == -mUdf(float) && nrinterpsamp )
    {
	topz = getMissingZ( RowCol(idi,idc), horizon1, nrinterpsamp );
	is1interp = true;
    }
    if ( botz == -mUdf(float) && nrinterpsamp)
    {
	botz = getMissingZ( RowCol(idi,idc), horizon2, nrinterpsamp );
	is2interp = true;
    }

    if ( topz == -mUdf(float) || botz == -mUdf(float) )
    {
	if ( !horizon2 || mIsZero(extrawidth,0.01)
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

    if ( horizon2 && isintersect )
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


void HorizonUtils::addHorizonData( const MultiID& id,
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
	const BinIDValueSet& bivs = *data[sectionidx];
	PosID posid( objid );
	BinIDValueSet::SPos pos;
	BinID bid; TypeSet<float> vals;
	while ( bivs.next(pos) )
	{
	    bivs.get( pos, bid, vals );
	    for ( int validx=1; validx<vals.size(); validx++ )
		horizon->auxdata.setAuxDataVal( validx-1, bid, vals[validx] );
	}
    }
}


#define mIsEmptyErr( cond, surfid )\
    if ( cond )\
    {\
	strm << "\n Cannot get Positions for horizon ID"<<surfid<<" \n";\
	return;\
    }

void HorizonUtils::getWantedPos2D( od_ostream& strm,
				   ObjectSet<MultiID>& midset,
				   DataPointSet* dtps,
				   const TrcKeySampling& horsamp,
				   const Interval<float>& extraz,
				   Pos::GeomID geomid )
{
    RefObjectSet<DataPointSet> possurf0;
    RefObjectSet<DataPointSet> possurf1;
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

} // namespace EM
