/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          May 2001
 RCS:           $Id: uinlapartserv.cc,v 1.42 2007-10-22 07:06:19 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uinlapartserv.h"

#include "binidvalset.h"
#include "ctxtioobj.h"
#include "datacoldef.h"
#include "debug.h"
#include "ioman.h"
#include "ioobj.h"
#include "keystrs.h"
#include "nlacrdesc.h"
#include "nladataprep.h"
#include "picksettr.h"
#include "posvecdataset.h"
#include "posvecdatasettr.h"
#include "ptrman.h"
#include "sorting.h"
#include "survinfo.h"
#include "varlenarray.h"
#include "wellextractdata.h"
#include "welltransl.h"

#include "uicombobox.h"
#include "uidistribution.h"
#include "uiexecutor.h"
#include "uigeninput.h"
#include "uiioobjsel.h"
#include "uimsg.h"
#include "uiposdataedit.h"


const int uiNLAPartServer::evPrepareWrite	= 0;
const int uiNLAPartServer::evPrepareRead	= 1;
const int uiNLAPartServer::evReadFinished	= 2;
const int uiNLAPartServer::evGetInputNames	= 3;
const int uiNLAPartServer::evGetStoredInput	= 4;
const int uiNLAPartServer::evGetData		= 5;
const int uiNLAPartServer::evSaveMisclass	= 6;
const int uiNLAPartServer::evCreateAttrSet	= 7;
const char* uiNLAPartServer::sKeyUsrCancel	= "User cancel";


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
	PickSetTranslator::createBinIDValueSets( crdesc.outids, bivsets );
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
    dobalfld = new uiGenInput( datagrp, "Balance data", BoolInpSpec(true) );
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
	if ( bsetup.nrptsperclss < 1 || mIsUdf(bsetup.nrptsperclss) )
	    mErrRet("Please enter a valid number of points per class")
	bsetup.noiselvl = percnoisefld->getfValue();
	if ( mIsUdf(bsetup.noiselvl) )
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


bool uiNLAPartServer::extractDirectData( const ObjectSet<PosVecDataSet>& vdss )
{
    const NLACreationDesc& crdesc = creationDesc();
    if ( vdss.size() != crdesc.outids.size() )
    {
	if ( DBG::isOn() )
	    DBG::message( "uiNLAPartServer::extractDirectData: "
			  "Nr BinIDValue Sets != Nr. well IDs" );
	return false;
    }

    // Put the positions in new BinIDValueSets
    ObjectSet<BinIDValueSet> bivsets;
    for ( int idx=0; idx<vdss.size(); idx++ )
    {
	BinIDValueSet* newbvs = new BinIDValueSet( 1, true );
	bivsets += newbvs;
	newbvs->append( vdss[idx]->data() );
    }

    // Fetch the well data
    Well::LogDataExtracter lde( crdesc.outids, bivsets );
    lde.usePar( crdesc.pars );
    uiExecutor uiex( appserv().parent(), lde );
    if ( uiex.go() )
    {
	// Add a column to the input data
	const BufferString outnm = crdesc.design.outputs.get(0);
	for ( int idx=0; idx<vdss.size(); idx++ )
	{
	    PosVecDataSet& vds = const_cast<PosVecDataSet&>(*vdss[idx]);
	    DataColDef* newdcd = new DataColDef( outnm );
	    newdcd->ref_ = outnm;
	    vds.add( newdcd );
	    const TypeSet<float>& res = *lde.results()[idx];
	    const int ressz = res.size();

	    BinIDValueSet::Pos pos;
	    const int lastidx = vds.data().nrVals() - 1;
	    BinID bid;
	    mVariableLengthArr( float, vals, lastidx+1 );
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
    return true;
}


class uiLithCodeMan : public uiDialog
{
public:

uiLithCodeMan( uiParent* p, const TypeSet<int>& codes, BufferStringSet& usels,
       		const char* lognm )
    	: uiDialog(p,uiDialog::Setup("Manage codes",
				     "Specify how to handle codes",
				     "0.4.6"))
	, usrsels(usels)
{
    BufferStringSet opts;
    opts.add( "Use" ); opts.add( "Merge into" ); opts.add( "Drop" );
    uiLabeledComboBox* prevoptlcb = 0;
    for ( int icode=0; icode<codes.size(); icode++ )
    {
	const int curcode = codes[icode];
	BufferString txt( "Code '" );
	txt += curcode; txt += "'";
	uiLabeledComboBox* optlcb = new uiLabeledComboBox( this, opts, txt );
	uiComboBox* optbox = optlcb->box();
	BufferString nm( lognm ); nm += " ["; nm += curcode; nm += "]";
	uiGenInput* nmfld = new uiGenInput( this, "Name", nm );
	uiLabeledComboBox* codelcb = new uiLabeledComboBox( this, "Code" );
	for ( int ic=0; ic<codes.size(); ic++ )
	{
	    if ( ic == curcode ) continue;
	    BufferString s; s+= codes[ic];
	    codelcb->box()->addItem( s );
	}

	nmfld->attach( rightOf, optlcb );
	codelcb->attach( rightOf, optlcb );
	if ( prevoptlcb )
	    optlcb->attach( alignedBelow, prevoptlcb );
	prevoptlcb = optlcb;

	optflds += optbox;
	nmflds += nmfld;
	mrgcodeflds += codelcb;
	optbox->selectionChanged.notify( mCB(this,uiLithCodeMan,selChg) );
    }

    selChg( 0 );
}


void selChg( CallBacker* sender )
{
    for ( int idx=0; idx<optflds.size(); idx++ )
    {
	uiComboBox* cb = optflds[idx];
	if ( sender && sender != cb )
	    continue;
	const int opt = cb->currentItem();
	nmflds[idx]->display( opt == 0 );
	mrgcodeflds[idx]->display( opt == 1 );
    }
}


bool acceptOK( CallBacker* )
{
    usrsels.deepErase();
    for ( int idx=0; idx<optflds.size(); idx++ )
    {
	uiComboBox* cb = optflds[idx];
	const int opt = cb->currentItem();
	BufferString txt( opt == 0 ? "U" : (opt == 1 ? "M" : "R") );
	if ( opt == 0 )
	    txt += nmflds[idx]->text();
	else if ( opt == 1 )
	    txt += mrgcodeflds[idx]->box()->text();
	usrsels.add( txt );
    }
    return true;
}

    BufferStringSet&		usrsels;
    ObjectSet<uiComboBox>	optflds;
    ObjectSet<uiGenInput>	nmflds;
    ObjectSet<uiLabeledComboBox> mrgcodeflds;

};


const char* uiNLAPartServer::convertToClasses(
					const ObjectSet<PosVecDataSet>& vdss,
					const int firstgoodvds )
{
    const int valnr = vdss[firstgoodvds]->data().nrVals() - 1;
    const char* valnm = vdss[firstgoodvds]->colDef(valnr).name_;

    // Discover the litho codes
    LithCodeData lcd;
    for ( int iset=firstgoodvds; iset<vdss.size(); iset++ )
    {
	const BinIDValueSet& bvs = vdss[iset]->data();
	BinIDValueSet::Pos pos;
	while( bvs.next(pos) )
	{
	    const float val = bvs.getVals(pos)[valnr];
	    if ( mIsUdf(val) ) continue;
	    const int code = mNINT(val);
	    if ( lcd.codes.indexOf(code) < 0 )
		lcd.codes += code;
	}
    }

    if ( lcd.codes.size() < 2 )
	return "Only one lithology found - need at least 2";
    else if ( lcd.codes.size() > 20 )
	return "More than 20 lithologies found - please group lithologies";

    sort( lcd.codes );
    BufferStringSet usels;
    uiLithCodeMan dlg( appserv().parent(), lcd.codes, usels, valnm );
    if ( !dlg.go() )
	return sKeyUsrCancel;

    lcd.useUserSels( usels );
    for ( int iset=0; iset<vdss.size(); iset++ )
    {
	PosVecDataSet& vds = const_cast<PosVecDataSet&>( *vdss[iset] );
	lcd.addCols( vds, valnm );
	if ( !vds.data().isEmpty() )
	    lcd.fillCols( vds, valnr );
    }

    return 0;
}


void uiNLAPartServer::LithCodeData::useUserSels( const BufferStringSet& usels )
{
    for ( int icode=0; icode<codes.size(); icode++ )
    {
	const char* det = usels.get( icode ).buf();
	if ( *det == 'R' )
	    ptrtbl += -1;
	else if ( *det == 'M' )
	    ptrtbl += codes.indexOf( atoi(det+1) );
	else
	{
	    ptrtbl += icode;
	    usedcodes += codes[icode];
	    usednames.add( det + 1 );
	}
    }

    // Handle indirect references. Blunt but safe approach.
    for ( int idx=0; idx<ptrtbl.size()-2; idx++ )
    for ( int iptr=0; iptr<ptrtbl.size(); iptr++ )
    {
	int& pointedto = ptrtbl[iptr];
	if ( pointedto != -1 )
	    pointedto = ptrtbl[pointedto]; // will often be the same
    }
}


void uiNLAPartServer::LithCodeData::addCols( PosVecDataSet& vds,
					     const char* valnm )
{
    for ( int icode=0; icode<usedcodes.size(); icode++ )
    {
	const int curcode = usedcodes[icode];
	const char* givennm = usednames.get(icode).buf();

	BufferString lithstr = valnm; lithstr += "=";
	bool firstcode = true;
	for ( int idx=0; idx<ptrtbl.size(); idx++ )
	{
	    int pointingto = ptrtbl[idx];
	    if ( pointingto == -1 ) continue;
	    if ( codes[pointingto] == curcode )
	    {
		if ( firstcode )
		    firstcode = false;
		else
		    lithstr += ",";
		lithstr += codes[idx];
	    }
	}
	BufferString refnm( givennm ); refnm += " [";
	refnm += lithstr; refnm += "]";

	DataColDef* dcd = new DataColDef( givennm, refnm, 0 );
	vds.add( dcd );
    }
}


void uiNLAPartServer::LithCodeData::fillCols( PosVecDataSet& vds,
					      const int valnr )
{
    BinIDValueSet& bvs = vds.data();
    BinIDValueSet::Pos pos;
    while ( bvs.next(pos) )
    {
	float* vals = bvs.getVals( pos );
	const float val = vals[valnr];
	if ( mIsUdf(val) ) continue;

	const int code = mNINT(val);
	int codeidx = codes.indexOf( code );
	if ( codeidx >= 0 )
	    codeidx = ptrtbl[codeidx];
	if ( codeidx < 0 ) continue;

	codeidx = usedcodes.indexOf( codes[codeidx] );
	if ( codeidx < 0 )
	    { pErrMsg("Logic error somewhere"); continue; }

	for ( int icode=0; icode<usedcodes.size(); icode++ )
	    vals[valnr+icode+1] = icode == codeidx ? 1 : 0;
    }
}


const char* uiNLAPartServer::prepareInputData(
		const ObjectSet<PosVecDataSet>& inpvdss )
{
    const NLACreationDesc& crdesc = creationDesc();

    const bool directextraction = crdesc.doextraction && crdesc.isdirect;
    if ( directextraction )
    {
       if ( !extractDirectData(inpvdss) )
	    return 0;

	if ( crdesc.design.classification )
	{
	    int firstgoodvds = -1;
	    for ( int iset=0; iset<inpvdss.size(); iset++ )
	    {
		if ( !inpvdss[iset]->data().isEmpty() )
		    { firstgoodvds = iset; break; }
	    }
	    if ( firstgoodvds == -1 )
		return "No valid data found";

	    const int orgnrvals = inpvdss[firstgoodvds]->data().nrVals();
	    const char* res = convertToClasses( inpvdss, firstgoodvds );
	    if ( res )
		return res;

	    // change design output nodes to new nodes
	    BufferStringSet& outps = const_cast<BufferStringSet&>(
		    				crdesc.design.outputs );
	    outps.deepErase();
	    const int newnrvals = inpvdss[firstgoodvds]->data().nrVals();
	    for ( int idx=orgnrvals; idx<newnrvals; idx++ )
		outps.add( inpvdss[firstgoodvds]->colDef(idx).ref_ );
	}
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
	if ( crdesc.isdirect && !crdesc.design.classification )
	{
	    uiPrepNLAData pddlg( appserv().parent(), vdss );
	    allok = pddlg.go();
	    if ( allok )
	    {
		const int targetcol = trainvds.data().nrVals() - 1;
		NLADataPreparer dptrain( trainvds.data(), targetcol );
		dptrain.removeUndefs(); dptrain.limitRange( pddlg.rg_ );
		if ( pddlg.dobal_ )
		    dptrain.balance( pddlg.bsetup );
		if ( !testvds.data().isEmpty() )
		{
		    NLADataPreparer dptest( testvds.data(), targetcol );
		    dptest.removeUndefs(); dptest.limitRange( pddlg.rg_ );
		    if ( pddlg.dobal_ )
			dptest.balance( pddlg.bsetup );
		}
	    }
	}

	if ( allok )
	    return 0;
    }

    trainvds.data().empty(); testvds.data().empty();
    return sKeyUsrCancel;
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
