/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : June 2001
-*/
 
static const char* rcsID = "$Id: nlacrdesc.cc,v 1.7 2005-02-08 16:57:12 bert Exp $";

#include "nlacrdesc.h"
#include "posvecdataset.h"
#include "datacoldef.h"
#include "ioman.h"
#include "errh.h"
#include "ptrman.h"
#include "stats.h"

NLACreationDesc& NLACreationDesc::operator =(
	const NLACreationDesc& sd )
{
    if ( this != &sd )
    {
	design = sd.design;
	deepCopy( outids, sd.outids );
	doextraction = sd.doextraction;
	fsid = sd.fsid;
	ratiotst = sd.ratiotst;
	isdirect = sd.isdirect;
    }
    return *this;
}


void NLACreationDesc::clear()
{
    design.clear();
    deepErase(outids);
    fsid = "";
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
    trainvds.data().copyStructureFrom( vdss[0]->data() );
    if ( doextraction && !isdirect )
    {
        for ( int iout=0; iout<nrout; iout++ )
	{
	    BufferString psnm = IOM().nameOf( outids[iout]->buf() );
	    DataColDef* newdcd = new DataColDef( psnm );
	    newdcd->ref_ = *outids[iout];
            trainvds.add( newdcd );
	}
    }
    testvds.data().copyStructureFrom( trainvds.data() );

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
    const int needednrtest = (int)(trainbvs.totalSize() * ratiotst + .5);
    if ( needednrtest < 1  || needednrtest >= trainbvs.totalSize() )
	return 0;

    BinID bid; float vals[ trainbvs.nrVals() ];
    while ( testbvs.totalSize() < needednrtest )
    {
	const int randidx = Stat_getIndex( trainbvs.totalSize() );
	BinIDValueSet::Pos pos = trainbvs.getPos( randidx );
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
    BinID bid; float vals[ totnrvals ];
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
