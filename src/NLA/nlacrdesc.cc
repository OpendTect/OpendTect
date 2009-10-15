/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : June 2001
-*/
 
static const char* rcsID = "$Id: nlacrdesc.cc,v 1.21 2009-10-15 10:07:13 cvsbert Exp $";

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

const char** NLACreationDesc::DataTypeNames()
{
    static const char* datatyps[] = { "Training data", "Test data",
		"Misclassified training data", "Misclassified test data", 0 };
    return datatyps;
}


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
					  DataPointSet& dps ) const
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
	const_cast<NLACreationDesc*>(this)->pars.merge( vds.pars() );
    }
    if ( totnrvec < 1 )
	return "No data vectors found";

    // If not direct prediction, add a ColumnDef for each output node
    dps.dataSet().copyStructureFrom( dpss[0]->dataSet() );
    const int orgnrcols = dps.dataSet().nrCols();
    int nrcols = orgnrcols;
    if ( doextraction && !isdirect )
    {
        for ( int iout=0; iout<nrout; iout++ )
	{
	    BufferString psnm = LineKey::defKey2DispName( outids.get(iout) );
            dps.dataSet().add( new DataColDef( psnm, *outids[iout] ) );
	}
	nrcols = dps.nrCols();
    }

    // Get the data into train and test set
    Stats::RandGen::init();
    const bool extractrand = ratiotst > -0.001;
    const float tstratio = ratiotst < 0 ? -ratiotst : ratiotst;
    const int lasttrain = (int)((1-ratiotst)*totnrvec + .5);
    for ( int idps=0; idps<dpss.size(); idps++ )
    {
	const DataPointSet& curdps = *dpss[idps];
	for ( DataPointSet::RowID irow=0; irow<curdps.size(); irow++ )
	{
	    DataPointSet::DataRow dr( curdps.dataRow(irow) );
	    for ( int idx=0; idx<nrout; idx++ )
		dr.data_ += idx == idps ? 1 : 0;

	    const bool istest = extractrand ? Stats::RandGen::get() < ratiotst
					     : irow > lasttrain;
	    dr.setGroup( istest ? 2 : 1 );
	    dps.addRow( dr );
	}
    }

    dps.dataChanged();
    return 0;
}
