/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          May 2001
 RCS:           $Id: uinlapartserv.cc,v 1.18 2005-05-04 15:19:59 cvsbert Exp $
________________________________________________________________________

-*/

#include "uinlapartserv.h"
#include "nlacrdesc.h"
#include "picksettr.h"
#include "welltransl.h"
#include "wellextractdata.h"
#include "posvecdataset.h"
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
#include "featset.h"
#include "featsettr.h"
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


static void addFSToSets( ObjectSet<BinIDValueSet>& bivsets, FeatureSet& fs )
{
    const int nrdescs = fs.descs().size();
    BinIDValueSet* bvs = new BinIDValueSet( nrdescs + 1, true );
    bivsets += bvs;
    float vals[nrdescs + 1];
    for ( int idx=0; idx<fs.size(); idx++ )
    {
	const FeatureVec& vec = *fs[idx];
	vals[0] = vec.fvPos().ver;
	for ( int iv=0; iv<vec.size(); iv++ )
	    vals[iv+1] = vec[iv];
	bvs->add( vec.fvPos(), vals );
    }
    fs.erase();
}


static void putBivSetToFS( BinIDValueSet& bvs, FeatureSet& fs )
{
    BinIDValueSet::Pos pos;
    const int nrdescs = fs.descs().size();
    float vals[nrdescs+1];
    BinID binid;
    while ( bvs.next(pos) )
    {
	bvs.get( pos, binid, vals );
        FVPos fvp(binid.inl, binid.crl, vals[0]);
	FeatureVec* vec = new FeatureVec( fvp );
	for ( int idx=1; idx<=nrdescs; idx++ )
	    (*vec) += vals[idx];
	fs += vec;
    }
}

class uiBalanceDataDlg : public uiDialog
{
public:

uiBalanceDataDlg( uiParent* p, ObjectSet<PosVecDataSet>& vdss )
    : uiDialog(p,uiDialog::Setup("Data balancing",gtTitle(vdss),"0.4.3"))
{
    const BinIDValueSet& bvs = vdss[0]->data();
    BinIDValueSet::Pos pos;
    const int valnr = bvs.nrVals() - 1;
    TypeSet<float> datavals;
    while ( bvs.next(pos) )
    {
	const float* vals = bvs.getVals( pos );
	float val = vals[valnr];
	if ( !mIsUndefined(val) )
	    datavals += val;
    }
    sort_array( datavals.arr(), datavals.size() );

    uiGroup* graphgrp = new uiGroup( this, "Graph group" );
    plotfld = new uiDistribPlot( graphgrp );
    const char* varname = vdss[0]->colDef(vdss[0]->nrCols()-1).name_;
    plotfld->setData( datavals.arr(), datavals.size(), varname );

    uiGroup* datagrp = new uiGroup( this, "Data group" );
    dobalfld = new uiGenInput( datagrp, "Balance data", BoolInpSpec() );
    dobalfld->valuechanged.notify( mCB(this,uiBalanceDataDlg,doBalChg) );
    rg_.start = datavals[0];
    rg_.stop = datavals[datavals.size()-1];
    valrgfld = new uiGenInput( datagrp, "Data range to use",
	    			FloatInpIntervalSpec(rg_) );
    valrgfld->attach( alignedBelow, dobalfld );
    nrptsperclss_ = plotfld->avgCount();
    nrptspclssfld = new uiGenInput( datagrp,
				"Target Number of data points per class",
				IntInpSpec(nrptsperclss_) );
    nrptspclssfld->attach( alignedBelow, valrgfld );

    datagrp->attach( centeredBelow, graphgrp );
}

const char* gtTitle( const ObjectSet<PosVecDataSet>& vdss ) const
{
    const PosVecDataSet& pvds = *vdss[0];
    const DataColDef& dcd = pvds.colDef( pvds.nrCols()-1 );
    static BufferString ret;
    ret = "Specify balancing for '";
    ret += dcd.name_;
    ret += "'";
    return ret.buf();
}

void doBalChg( CallBacker* )
{
    dobal_ = dobalfld->getBoolValue();
    valrgfld->display( dobal_ );
    nrptspclssfld->display( dobal_ );
}

bool acceptOK( CallBacker* )
{
    dobal_ = dobalfld->getBoolValue();
    if ( dobal_ )
    {
	rg_ = valrgfld->getFInterval();
	nrptsperclss_ = nrptspclssfld->getIntValue();
    }

    return true;
}

    uiDistribPlot*	plotfld;
    uiGenInput*		dobalfld;
    uiGenInput*		valrgfld;
    uiGenInput*		nrptspclssfld;

    bool		dobal_;
    Interval<float>	rg_;
    int			nrptsperclss_;
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
		    vals[lastidx] = ivec >= ressz ? mUndefValue : res[ivec];
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
    vdss += &trainvds; vdss += &testvds;
    uiPosDataEdit dlg( appserv().parent(), vdss );
    dlg.saveData.notify( mCB(this,uiNLAPartServer,writeSets) );
    if ( dlg.go() )
    {
	bool allok = true;
	if ( crdesc.isdirect )
	{
	    uiBalanceDataDlg bddlg( appserv().parent(), vdss );
	    allok = bddlg.go();
	    if ( allok )
	    {
		//TODO do the actual balancing to trainvds.data()
	    }
	}

	if ( allok )
	    return 0;
    }

    trainvds.data().empty(); testvds.data().empty();
    return "User cancel";
}


#define mErrRet(s) { return; }

void uiNLAPartServer::writeSets( CallBacker* cb )
{
    mDynamicCastGet(uiPosDataEdit*,dlg,cb)
    if ( !dlg ) { pErrMsg("Huh"); return; }

    const int tblidx = dlg->saveTableNo();
    PosVecDataSet savevds; savevds.copyStructureFrom( trainvds );
    for ( int idx=0; idx<2; idx++ )
    {
	if ( tblidx >= 0 && idx != tblidx )
	    continue;
	dlg->getTableData( idx, savevds.data() );
    }
    if ( savevds.data().isEmpty() )
	{ uiMSG().error( "Empty data set" ); return; }

    CtxtIOObj ctio( FeatureSetTranslatorGroup::ioContext() );
    ctio.ctxt.forread = false;
    uiIOObjSelDlg seldlg( appserv().parent(), ctio );
    if ( !seldlg.go() )
	return;
    ctio.setObj( seldlg.ioObj()->clone() );

    FeatureSet fswrite( savevds );
    fswrite.pars() = storepars;
    fswrite.pars().set( sKey::Type, "MVA Data" );
    ctio.ioobj->pars().setYN( FeatureSetTranslator::sKeyDoVert, true );

    bool isok = fswrite.put( ctio.ioobj );
    ctio.setObj( 0 );
    if ( !isok )
	{ uiMSG().error( "Cannot write data set" ); return; }

    fswrite.close();
}
