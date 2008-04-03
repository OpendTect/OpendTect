/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : June 2001
-*/
 
static const char* rcsID = "$Id: nlacrdesc.cc,v 1.14 2008-04-03 11:18:47 cvsbert Exp $";

#include "nlacrdesc.h"

#include "datacoldef.h"
#include "errh.h"
#include "ioman.h"
#include "linekey.h"
#include "posvecdataset.h"
#include "datapointset.h"
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


const char* NLACreationDesc::prepareData( const ObjectSet<DataPointSet>& dpss,
					  DataPointSet& traindps,
					  DataPointSet& testdps ) const
{
    const int nrout = dpss.size();
    if ( !nrout )
	{ return "Internal: No input BinIDValueSets to transfer data from"; }

    // For direct prediction, the sets are ready. If not, add a ColumnDef
    // for each output node
    traindps.dataSet().copyStructureFrom( dpss[0]->dataSet() );
    if ( doextraction && !isdirect )
    {
        for ( int iout=0; iout<nrout; iout++ )
	{
	    BufferString psnm = LineKey::defKey2DispName( outids.get(iout) );
            traindps.dataSet().add( new DataColDef( psnm, *outids[iout] ) );
	}
    }
    testdps.dataSet().copyStructureFrom( traindps.dataSet() );

    int totnrvec = 0;
    for ( int idps=0; idps<dpss.size(); idps++ )
	totnrvec += dpss[idps]->size();

    // Get the data into train and test set
    Stats::RandGen::init();
    const bool extractrand = ratiotst > -0.001;
    const float tstratio = ratiotst < 0 ? -ratiotst : ratiotst;
    const int lasttrain = (int)((1-ratiotst)*totnrvec + .5);
    for ( int idps=0; idps<dpss.size(); idps++ )
    {
	const DataPointSet& dps = *dpss[idps];
	for ( DataPointSet::RowID irow=0; irow<dps.size(); irow++ )
	{
	    DataPointSet::DataRow dr( dps.dataRow(irow) );
	    if ( !isdirect )
	    {
		for ( int idx=0; idx<nrout; idx++ )
		    dr.data_ += idx == idps ? 1 : 0;
	    }

	    const bool istrain = extractrand ? Stats::RandGen::get() < ratiotst
					     : irow > lasttrain;
	    if ( istrain )
		traindps.addRow( dr );
	    else
		testdps.addRow( dr );
	}
    }

    traindps.dataChanged(); testdps.dataChanged();
    return 0;
}
