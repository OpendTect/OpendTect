/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : June 2001
-*/
 
static const char* rcsID = "$Id: nlacrdesc.cc,v 1.17 2008-04-15 12:50:30 cvsbert Exp $";

#include "nlacrdesc.h"

#include "datacoldef.h"
#include "errh.h"
#include "ioman.h"
#include "ioobj.h"
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


static bool transferData2DPS( const PosVecDataSet& vds, DataPointSet& dps,
			      BufferString& errmsg )
{
    return true;
}


const char* NLACreationDesc::prepareData( const ObjectSet<DataPointSet>& dpss,
					  DataPointSet& traindps,
					  DataPointSet& testdps ) const
{
    int nrout = dpss.size();
    BufferString dpsnmadd;
    int totnrvec = 0;

    if ( doextraction )
    {
	if ( nrout < 1 )
	    { return "Internal: No input DataPointSets to transfer data from"; }
	for ( int idps=0; idps<dpss.size(); idps++ )
	    totnrvec += dpss[idps]->size();
    }
    else
    {
	PtrMan<IOObj> ioobj = IOM().get( vdsid );
	if ( !ioobj )
	    return "Cannot find training data set specified";
	static BufferString errmsg; PosVecDataSet vds;
	if ( !vds.getFrom(ioobj->fullUserExpr(true),errmsg) )
	    return errmsg.buf();
	if ( vds.pars().isEmpty() || vds.data().isEmpty() )
	    return "Invalid input data set specified";

	ObjectSet<DataPointSet>& ncdpss
	    		= const_cast<ObjectSet<DataPointSet>&>( dpss );
	const bool is2d = dpss[0]->is2D();
	const bool ismini = dpss[0]->isMinimal();
	deepErase( ncdpss );
	ncdpss += new DataPointSet( vds, is2d, ismini );
	dpsnmadd += " (data from '";
	dpsnmadd += ioobj->name(); dpsnmadd += "')";
	nrout = 1;
	totnrvec = dpss[0]->size();
    }
    if ( totnrvec < 1 )
	return "No data vectors found";

    BufferString setnm( "NN Training data" ); setnm += dpsnmadd;
    traindps.setName( setnm );
    setnm = "NN Test data"; setnm += dpsnmadd;
    testdps.setName( setnm );
    
    // If not direct prediction, add a ColumnDef for each output node
    traindps.dataSet().copyStructureFrom( dpss[0]->dataSet() );
    const int orgnrcols = traindps.dataSet().nrCols();
    int nrcols = orgnrcols;
    if ( doextraction && !isdirect )
    {
        for ( int iout=0; iout<nrout; iout++ )
	{
	    BufferString psnm = LineKey::defKey2DispName( outids.get(iout) );
            traindps.dataSet().add( new DataColDef( psnm, *outids[iout] ) );
	}
	nrcols = traindps.nrCols();
    }
    testdps.dataSet().copyStructureFrom( traindps.dataSet() );

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
	    if ( nrcols != orgnrcols )
	    {
		for ( int idx=0; idx<nrout; idx++ )
		    dr.data_ += idx == idps ? 1 : 0;
	    }

	    const bool istest = extractrand ? Stats::RandGen::get() < ratiotst
					     : irow > lasttrain;
	    if ( istest )
		testdps.addRow( dr );
	    else
		traindps.addRow( dr );
	}
    }

    traindps.dataChanged(); testdps.dataChanged();
    return 0;
}
