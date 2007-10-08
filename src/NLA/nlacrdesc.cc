/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : June 2001
-*/
 
static const char* rcsID = "$Id: nlacrdesc.cc,v 1.13 2007-10-08 13:40:00 cvsbert Exp $";

#include "nlacrdesc.h"

#include "datacoldef.h"
#include "errh.h"
#include "ioman.h"
#include "linekey.h"
#include "posvecdataset.h"
#include "ptrman.h"
#include "statrand.h"

NLACreationDesc& NLACreationDesc::operator =(
	const NLACreationDesc& sd )
{
    if ( this != &sd )
    {
	design = sd.design;
	deepCopy( outids, sd.outids );
	doextraction = sd.doextraction;
	vdsid = sd.vdsid;
	ratiotst = sd.ratiotst;
	isdirect = sd.isdirect;
    }
    return *this;
}


void NLACreationDesc::clear()
{
    design.clear();
    deepErase(outids);
    vdsid = "";
    doextraction = true;
    isdirect = false;
}


const char* NLACreationDesc::prepareData( const ObjectSet<PosVecDataSet>& vdss,
					  PosVecDataSet& trainvds,
					  PosVecDataSet& testvds ) const
{
    trainvds.empty(); testvds.empty();
    const char* res = 0;
    const int nrout = vdss.size();
    if ( !nrout )
	{ return "Internal: No input BinIDValueSets to transfer data from"; }

    // For direct prediction, the sets are ready. If not, add a ColumnDef
    // for each output node
    trainvds.copyStructureFrom( *vdss[0] );
    if ( doextraction && !isdirect )
    {
        for ( int iout=0; iout<nrout; iout++ )
	{
	    BufferString psnm = LineKey::defKey2DispName( outids.get(iout) );
            trainvds.add( new DataColDef( psnm, *outids[iout] ) );
	}
    }
    testvds.copyStructureFrom( trainvds );

    // Get the data into train and test set
    for ( int idx=0; idx<vdss.size(); idx++ )
    {
        if ( !addBVSData(vdss[idx]->data(),trainvds.data(),idx) )
        {
            BufferString msg( "No values collected for '" );
            msg += IOM().nameOf( *outids[idx] );
            msg += "'";
            UsrMsg( msg );
        }
    }

    if ( res && *res ) return res;

    BinIDValueSet& trainbvs = trainvds.data();
    BinIDValueSet& testbvs = testvds.data();
    const bool extractrand = ratiotst > -0.001;
    const float tstratio = ratiotst < 0 ? -ratiotst : ratiotst;
    const int needednrtest = (int)(trainbvs.totalSize() * tstratio + .5);
    if ( needednrtest < 1  || needednrtest >= trainbvs.totalSize() )
	return 0;

    BinID bid;
    ArrPtrMan<float> vals = new float [trainbvs.nrVals()];
    const int totsz = trainbvs.totalSize();
    int botidx = totsz - 1;
    while ( testbvs.totalSize() < needednrtest )
    {
	const int useidx = extractrand ? Stats::RandGen::getIndex( totsz )
				       : botidx--;
	BinIDValueSet::Pos pos = trainbvs.getPos( useidx );
	trainbvs.get( pos, bid, vals );
	trainbvs.remove( pos );
	testbvs.add( bid, vals );
    }

    return res;
}


int NLACreationDesc::addBVSData( const BinIDValueSet& bvs,
				 BinIDValueSet& bvsout, int iout ) const
{
    int nradded = 0;
    BinIDValueSet::Pos pos;
    const int totnrvals = bvsout.nrVals();
    const int outnrvals = totnrvals - bvs.nrVals();
    BinID bid;
    ArrPtrMan<float> vals = new float [totnrvals];
    while ( bvs.next( pos ) )
    {
	bvs.get( pos, bid, vals );
	if ( !isdirect )
	{
	    for ( int idx=0; idx<outnrvals; idx++ )
		vals[totnrvals-outnrvals+idx] = idx == iout ? 1 : 0;
	}
	bvsout.add( bid, vals );
	nradded++;
    }
    return nradded;
}
