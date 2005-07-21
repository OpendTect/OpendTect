/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          May 2001
 RCS:           $Id: uinlapartserv.cc,v 1.28 2005-07-21 11:30:52 cvsbert Exp $
________________________________________________________________________

-*/

#include "uinlapartserv.h"
#include "nlacrdesc.h"
#include "nladataprep.h"
#include "picksettr.h"
#include "welltransl.h"
#include "wellextractdata.h"
#include "posvecdataset.h"
#include "posvecdatasettr.h"
#include "datacoldef.h"
#include "binidvalset.h"
#include "uiexecutor.h"
#include "uiposdataedit.h"
#include "uiioobjsel.h"
#include "uidistribution.h"
#include "uigeninput.h"
#include "uicanvas.h"
#include "uimsg.h"
#include "debug.h"
#include "ioman.h"
#include "ioobj.h"
#include "ptrman.h"
#include "survinfo.h"
#include "ctxtioobj.h"
#include "sorting.h"

const int uiNLAPartServer::evPrepareWrite	= 0;
const int uiNLAPartServer::evPrepareRead	= 1;
const int uiNLAPartServer::evReadFinished	= 2;
const int uiNLAPartServer::evGetInputNames	= 3;
const int uiNLAPartServer::evGetStoredInput	= 4;
const int uiNLAPartServer::evGetData		= 5;
const int uiNLAPartServer::evSaveMisclass	= 6;
const int uiNLAPartServer::evCreateAttrSet	= 7;
const int uiNLAPartServer::evIs2D		= 8;


uiNLAPartServer::uiNLAPartServer( uiApplService& a )
	: uiApplPartServer(a)
	, trainvds(*new PosVecDataSet("Training data"))
	, testvds(*new PosVecDataSet("Test data"))
	, mcvds(*new PosVecDataSet("Misclassified"))
	, storepars(*new IOPar)
{
}


uiNLAPartServer::~uiNLAPartServer()
{
    deepErase( inpnms );
    delete &trainvds;
    delete &testvds;
    delete &mcvds;
    delete &storepars;
}


bool uiNLAPartServer::willDoExtraction() const
{
    return creationDesc().doextraction;
}


const BufferStringSet& uiNLAPartServer::modelInputs() const
{
    return getModel().design().inputs;
}


void uiNLAPartServer::getBinIDValueSets(
				  ObjectSet<BinIDValueSet>& bivsets ) const
{
    const NLACreationDesc& crdesc = creationDesc();

    if ( !crdesc.isdirect )
	PickSetGroupTranslator::createBinIDValueSets( crdesc.outids, bivsets );
    else
    {
	Executor* ex = WellTranslator::createBinIDValueSets( crdesc.outids,
							     crdesc.pars,
							     bivsets );
	if ( !ex ) return;
	uiExecutor uiex( appserv().parent(), *ex );
	if ( !uiex.go() )
	    deepErase( bivsets );
    }
}


class uiPrepNLAData : public uiDialog
{
public:

uiPrepNLAData( uiParent* p, ObjectSet<PosVecDataSet>& vdss )
    : uiDialog(p,uiDialog::Setup("Data preparation",gtTitle(vdss),"0.4.3"))
{
    const BinIDValueSet& bvs = vdss[0]->data();
    bvs.getColumn( bvs.nrVals() - 1, datavals, false );
    sort_array( datavals.arr(), datavals.size() );

    uiGroup* graphgrp = new uiGroup( this, "Graph group" );
    plotfld = new uiDistribPlot( graphgrp );
    varname = vdss[0]->colDef(vdss[0]->nrCols()-1).name_;
    plotfld->setData( datavals.arr(), datavals.size(), varname );
    bsetup.nrptsperclss = plotfld->avgCount();
    plotfld->setAnnotatedNrClasses( bsetup.nrptsperclss );

    uiGroup* datagrp = new uiGroup( this, "Data group" );
    dobalfld = new uiGenInput( datagrp, "Balance data", BoolInpSpec() );
    dobalfld->valuechanged.notify( mCB(this,uiPrepNLAData,doBalChg) );

    nrptspclssfld = new uiGenInput( datagrp, "Data points per class",
				IntInpSpec(bsetup.nrptsperclss) );
    nrptspclssfld->attach( alignedBelow, dobalfld );
    nrptspclssfld->valuechanged.notify( mCB(this,uiPrepNLAData,cutoffChg) );
    percnoisefld = new uiGenInput( datagrp, "Percentage noise when adding",
				   FloatInpSpec(bsetup.noiselvl*100) );
    percnoisefld->attach( alignedBelow, nrptspclssfld );

    rg_.start = datavals[0];
    rg_.stop = datavals[datavals.size()-1];
    valrgfld = new uiGenInput( datagrp, "Data range to use",
	    			FloatInpIntervalSpec(rg_) );
    valrgfld->attach( alignedBelow, percnoisefld );
    valrgfld->valuechanged.notify( mCB(this,uiPrepNLAData,valrgChg) );

    datagrp->attach( centeredBelow, graphgrp );
}

const char* gtTitle( const ObjectSet<PosVecDataSet>& vdss ) const
{
    const PosVecDataSet& pvds = *vdss[0];
    const DataColDef& dcd = pvds.colDef( pvds.nrCols()-1 );
    static BufferString ret;
    ret = "Specify data preparation for '";
    ret += dcd.name_;
    ret += "'";
    return ret.buf();
}

void doBalChg( CallBacker* )
{
    const bool dobal = dobalfld->getBoolValue();
    nrptspclssfld->display( dobal );
    percnoisefld->display( dobal );
    bsetup.nrptsperclss = dobal ? nrptspclssfld->getIntValue() : -1;
    plotfld->setAnnotatedNrClasses( bsetup.nrptsperclss );
}

void cutoffChg( CallBacker* )
{
    bsetup.nrptsperclss = nrptspclssfld->getIntValue();
    plotfld->setAnnotatedNrClasses( bsetup.nrptsperclss );
}

void valrgChg( CallBacker* )
{
    rg_ = valrgfld->getFInterval();
    rg_.sort( true );
    TypeSet<float> newdatavals;
    for ( int idx=0; idx<datavals.size(); idx++ )
    {
	if ( rg_.includes( datavals[idx] ) )
	    newdatavals += datavals[idx];
    }
    plotfld->setData( newdatavals.arr(), newdatavals.size(), varname );
}

#define mErrRet(s) { uiMSG().error(s); return false; }
bool acceptOK( CallBacker* )
{
    dobal_ = dobalfld->getBoolValue();
    if ( dobal_ )
    {
	rg_ = valrgfld->getFInterval();
	rg_.sort();
	bsetup.nrptsperclss = nrptspclssfld->getIntValue();
	if ( bsetup.nrptsperclss < 1 || Values::isUdf(bsetup.nrptsperclss) )
	    mErrRet("Please enter a valid number of points per class")
	bsetup.noiselvl = percnoisefld->getfValue();
	if ( Values::isUdf(bsetup.noiselvl) )
	    bsetup.noiselvl = 0;
	if ( bsetup.noiselvl > 100 || bsetup.noiselvl < -1e-6 )
	    mErrRet("Please enter a valid number of points per class")
	bsetup.noiselvl *= 0.01;
    }

    bsetup.nrclasses = plotfld->nrClasses();
    return true;
}

    uiDistribPlot*	plotfld;
    uiGenInput*		dobalfld;
    uiGenInput*		valrgfld;
    uiGenInput*		nrptspclssfld;
    uiGenInput*		percnoisefld;

    TypeSet<float>	datavals;
    BufferString	varname;

    bool		dobal_;
    Interval<float>	rg_;
    NLADataPreparer::BalanceSetup bsetup;
};


const char* uiNLAPartServer::prepareInputData(
		const ObjectSet<PosVecDataSet>& inpvdss )
{
    const NLACreationDesc& crdesc = creationDesc();

    if ( crdesc.doextraction && crdesc.isdirect )
    {
	// Direct prediction: we need to fetch the well data
	if ( inpvdss.size() != crdesc.outids.size() )
	{
	    if ( DBG::isOn() )
		DBG::message( "uiNLAPartServer::prepareInputData: "
			      "Nr BinIDValue Sets != Nr. well IDs" );
	    return 0;
	}

	// Put the positions in new BinIDValueSets
	ObjectSet<BinIDValueSet> bivsets;
	for ( int idx=0; idx<inpvdss.size(); idx++ )
	{
	    BinIDValueSet* newbvs = new BinIDValueSet( 1, true );
	    bivsets += newbvs;
	    newbvs->append( inpvdss[idx]->data() );
	}

	// Fetch the well data
	Well::LogDataExtracter lde( crdesc.outids, bivsets );
	lde.usePar( crdesc.pars );
	uiExecutor uiex( appserv().parent(), lde );
	if ( uiex.go() )
	{
	    // Add a column to the input data
	    const BufferString outnm = crdesc.design.outputs.get(0);
	    for ( int idx=0; idx<inpvdss.size(); idx++ )
	    {
		PosVecDataSet& vds = *inpvdss[idx];
		DataColDef* newdcd = new DataColDef( outnm );
		newdcd->ref_ = outnm;
		vds.add( newdcd );
		TypeSet<float>& res = *lde.results()[idx];
		const int ressz = res.size();

		BinIDValueSet::Pos pos;
		const int lastidx = vds.data().nrVals() - 1;
		BinID bid; float vals[lastidx+1];
		int ivec = 0;
		while ( vds.data().next(pos) )
		{
		    vds.data().get( pos, bid, vals );
		    vals[lastidx] = ivec >= ressz ? mUdf(float) : res[ivec];
		    vds.data().set( pos, vals );
		    ivec++;
		}
	    }
	}

	deepErase( bivsets );
    }

    const char* res = crdesc.prepareData( inpvdss, trainvds, testvds );
    if ( res ) return res;

    // allow user to view and edit data
    ObjectSet<PosVecDataSet> vdss;
    vdss += &trainvds;
    if ( !testvds.data().isEmpty() )
	vdss += &testvds;
    uiPosDataEdit dlg( appserv().parent(), vdss, 0, uiPosDataEdit::AllOnly );
    dlg.saveData.notify( mCB(this,uiNLAPartServer,writeSets) );
    if ( dlg.go() )
    {
	if ( vdss.size() < 2 )
	    vdss += &testvds;
	bool allok = true;
	if ( crdesc.isdirect )
	{
	    uiPrepNLAData pddlg( appserv().parent(), vdss );
	    allok = pddlg.go();
	    if ( allok )
	    {
		const int targetcol = trainvds.data().nrVals() - 1;
		NLADataPreparer dp( trainvds.data(), targetcol );
		dp.removeUndefs();
		dp.limitRange( pddlg.rg_ );
		if ( pddlg.dobal_ )
		    dp.balance( pddlg.bsetup );
	    }
	}

	if ( allok )
	    return 0;
    }

    trainvds.data().empty(); testvds.data().empty();
    return "User cancel";
}


void uiNLAPartServer::writeSets( CallBacker* cb )
{
    // Almost identical to uiAttribCrossPlot::saveData
    // Couldn't think of a common place to put it
    mDynamicCastGet(uiPosDataEdit*,dlg,cb)
    if ( !dlg ) { pErrMsg("Huh"); return; }

    CtxtIOObj ctio( PosVecDataSetTranslatorGroup::ioContext() );
    ctio.ctxt.forread = false;
    ctio.ctxt.parconstraints.set( sKey::Type, "MVA Data" );
    ctio.ctxt.includeconstraints = true;
    uiIOObjSelDlg seldlg( appserv().parent(), ctio );
    if ( !seldlg.go() )
	return;
    ctio.setObj( seldlg.ioObj()->clone() );
    dlg->stdSave( *ctio.ioobj, false, &storepars );
    delete ctio.ioobj;
}
