/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : June 2001
-*/
 
static const char* rcsID = "$Id: nlacrdesc.cc,v 1.16 2008-04-11 13:22:25 cvsbert Exp $";

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
    const int nrcols = dps.nrCols();
    TypeSet<int> tbl( nrcols, -1 );
    for ( int idx=0; idx<2; idx++ )
    {
	const DataColDef::MatchLevel ml = idx	? DataColDef::Exact
	    					: DataColDef::Start;
	for ( DataPointSet::ColID colid=0; colid<nrcols; colid++ )
	{
	    if ( tbl[colid] >= 0 ) continue;

	    const DataColDef& dpscd = dps.colDef( colid );
	    for ( int vdscol=0; vdscol<vds.nrCols(); vdscol++ )
	    {
		if ( dpscd.compare(vds.colDef(vdscol),true) )
		    { tbl[colid] = vdscol; break; }
	    }
	}
    }

    for ( int idx=0; idx<tbl.size(); idx++ )
    {
	if ( tbl[idx] < 0 )
	{
	    errmsg = "Input data contains no column '";
	    errmsg += dps.colName( idx ); errmsg += "'";
	    return false;
	}
    }

    const BinIDValueSet& bvs = vds.data();
    BinIDValueSet::Pos pos;
    DataPointSet::DataRow dr; dr.data_.setSize( nrcols );
    while ( bvs.next(pos) )
    {
	const float* vals = bvs.getVals( pos );
	for ( DataPointSet::ColID colid=0; colid<nrcols; colid++ )
	    dr.data_[ colid ] = vals[ tbl[colid] ];
	dr.pos_.binid_ = bvs.getBinID( pos );
	dr.pos_.z_ = vals[0];
	dps.addRow( dr );
    }

    dps.dataChanged();
    return true;
}


const char* NLACreationDesc::prepareData( const ObjectSet<DataPointSet>& dpss,
					  DataPointSet& traindps,
					  DataPointSet& testdps ) const
{
    const int nrout = dpss.size();
    if ( !nrout )
	{ return "Internal: No input DataPointSets to transfer data from"; }
    int totnrvec = 0;
    for ( int idps=0; idps<dpss.size(); idps++ )
	totnrvec += dpss[idps]->size();
    if ( totnrvec < 1 )
	return "No data vectors found";

    BufferString dpsnmadd;
    if ( !doextraction )
    {
	PtrMan<IOObj> ioobj = IOM().get( vdsid );
	if ( !ioobj )
	    return "Cannot find training data set specified";
	static BufferString errmsg; PosVecDataSet vds;
	if ( !vds.getFrom(ioobj->fullUserExpr(true),errmsg) )
	    return errmsg.buf();
	if ( vds.pars().isEmpty() || vds.data().isEmpty() )
	    return "Invalid input data set specified";
	DataPointSet& dps = const_cast<DataPointSet&>( *dpss[0] );
	if ( !transferData2DPS(vds,dps,errmsg) )
	    return errmsg.buf();
	dpsnmadd += " (data from '";
	dpsnmadd += ioobj->name(); dpsnmadd += "')";
    }

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
