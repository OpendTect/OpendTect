/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : June 2001
-*/


#include "nlacrdesc.h"
#include "nlamodel.h"

#include "datacoldef.h"
#include "ioobj.h"
#include "posvecdataset.h"
#include "datapointset.h"
#include "ptrman.h"
#include "statrand.h"
#include "uistrings.h"

static const char* nladatatyps[] = { "Training data", "Test data",
		"Misclassified training data", "Misclassified test data", 0 };


bool isEmpty( const NLAModel* mdl )
{
    return !mdl || mdl->isEmpty();
}


const char** NLACreationDesc::DataTypeNames()
{
    return nladatatyps;
}


NLACreationDesc& NLACreationDesc::operator =(
	const NLACreationDesc& sd )
{
    if ( this != &sd )
    {
	design = sd.design;
	outids = sd.outids;
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
    outids.setEmpty();
    vdsid.setInvalid();
    doextraction = true;
    isdirect = false;
}


static bool haveColNmMatch( BufferString& colnm, const char* inpnodenm )
{
    const bool isstored = *inpnodenm == '[';
    if ( isstored )
	{ inpnodenm++; if ( !*inpnodenm ) return false; }
    BufferString nodenm( inpnodenm );
    if ( isstored && nodenm[nodenm.size()-1] == ']' )
	nodenm[nodenm.size()-1] = '\0';

    if ( colnm == nodenm )
	return true;

    if ( DBKey::isValidString(colnm) )
	colnm = DBKey(colnm).name();
    if ( DBKey::isValidString(nodenm) )
	nodenm = DBKey(nodenm).name();
    return colnm == nodenm;
}


static bool isPresentInDesgn( const NLADesign& des, const char* inpcolnm )
{
    const bool isstored = *inpcolnm == '[';
    if ( isstored )
	{ inpcolnm++; if ( !*inpcolnm ) return false; }
    BufferString colnm( inpcolnm );
    if ( isstored && colnm[colnm.size()-1] == ']' )
	colnm[colnm.size()-1] = '\0';

    for ( int idx=0; idx<des.inputs.size(); idx++ )
	if ( haveColNmMatch( colnm, des.inputs.get(idx).buf() ) )
	    return true;
    for ( int idx=0; idx<des.outputs.size(); idx++ )
	if ( haveColNmMatch( colnm, des.outputs.get(idx).buf() ) )
	    return true;

    return false;
}


uiString NLACreationDesc::prepareData(const ObjectSet<DataPointSet>& dpss,
					  DataPointSet& dps ) const
{
    int nrout = dpss.size();
    int totnrvec = 0;

    if ( doextraction )
    {
	if ( nrout < 1 )
	{
	    return tr("Internal: No input DataPointSets to "
		      "transfer data from");
	}
	for ( int idps=0; idps<dpss.size(); idps++ )
	    totnrvec += dpss[idps]->size();
    }
    else
    {
	PtrMan<IOObj> ioobj = vdsid.getIOObj();
	if ( !ioobj )
	    return tr("Cannot find training data set specified");

	uiString errmsg;
	PosVecDataSet vds;
	if ( !vds.getFrom(ioobj->fullUserExpr(true),errmsg) )
	    return  errmsg ;
	if ( vds.pars().isEmpty() || vds.data().isEmpty() )
	    return tr("Invalid input data set specified");

	ObjectSet<DataPointSet>& ncdpss
			= const_cast<ObjectSet<DataPointSet>&>( dpss );
	const bool is2d = dpss[0]->is2D();
	const bool ismini = dpss[0]->isMinimal();
	deepUnRef( ncdpss );
	ncdpss += new DataPointSet( vds, is2d, ismini );
	nrout = 1;
	totnrvec = dpss[0]->size();
	const_cast<NLACreationDesc*>(this)->pars.merge( vds.pars() );
    }
    if ( totnrvec < 1 )
	return tr("No data vectors found");

    dps.setEmpty();
    const DataPointSet& dps0 = *dpss[0];
    int nrcols = dps0.nrCols();
    BoolTypeSet isincl;
    for ( int icol=0; icol<nrcols; icol++ )
    {
	const DataColDef& cd( dps0.colDef(icol) );
	bool issel = isPresentInDesgn( design, cd.name_ );
	if ( !issel )
	    issel = isPresentInDesgn( design, cd.ref_ );
	isincl += issel;
	if ( issel )
	    dps.dataSet().add( new DataColDef(cd) );
    }

    // If not direct prediction, add a ColumnDef for each output node
    const bool addcols = doextraction && !isdirect;
    if ( addcols )
    {
        for ( int iout=0; iout<nrout; iout++ )
	{
	    BufferString psnm = outids.get( iout );
	    if ( DBKey::isValidString(psnm) )
		psnm = DBKey(psnm).name();
            dps.dataSet().add( new DataColDef( psnm, *outids[iout] ) );
	}
    }

    // Get the data into train and test set
    // All the following to support non-random test set extraction
    const bool extractrand = ratiotst > -0.001;
    const float tstratio = ratiotst < 0 ? -ratiotst : ratiotst;
    int lasttrain = 0;
    if ( !extractrand )
    {
	int totnrvecs = 0;
	for ( int idps=0; idps<dpss.size(); idps++ )
	    totnrvecs += dpss[idps]->size();
	lasttrain = (int)((1-tstratio)*totnrvecs + .5);
    }

    int itotal = 0;
    for ( int idps=0; idps<dpss.size(); idps++ )
    {
	const DataPointSet& curdps = *dpss[idps];
	for ( DataPointSet::RowID irow=0; irow<curdps.size(); irow++ )
	{
	    DataPointSet::DataRow inpdr( curdps.dataRow(irow) );
	    DataPointSet::DataRow outdr( inpdr );
	    outdr.data_.erase();
	    for ( int icol=0; icol<nrcols; icol++ )
	    {
		if ( isincl[icol] )
		    outdr.data_ += inpdr.data_[icol];
	    }
	    if ( addcols )
	    {
		for ( int iout=0; iout<nrout; iout++ )
		    outdr.data_ += mCast( float, iout == idps ? 1 : 0 );
	    }

	    const bool istest = extractrand ? Stats::randGen().get() < tstratio
					     : itotal > lasttrain;
	    outdr.setGroup( istest ? 2 : 1 );
	    dps.addRow( outdr );
	    itotal++;
	}
    }

    dps.dataChanged();
    return uiString::empty();
}
