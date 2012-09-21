/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          May 2001
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id$";

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
#include "datapointset.h"
#include "ptrman.h"
#include "seisioobjinfo.h"
#include "sorting.h"
#include "survinfo.h"
#include "varlenarray.h"
#include "wellextractdata.h"
#include "welltransl.h"

#include "uicombobox.h"
#include "uihistogramdisplay.h"
#include "uistatsdisplay.h"
#include "uitaskrunner.h"
#include "uigeninput.h"
#include "uidatapointset.h"
#include "uilabel.h"
#include "uimsg.h"

#include <iostream>

int uiNLAPartServer::evPrepareWrite()		{ return 0; }
int uiNLAPartServer::evPrepareRead()		{ return 1; }
int uiNLAPartServer::evReadFinished()		{ return 2; }
int uiNLAPartServer::evGetInputNames()		{ return 3; }
int uiNLAPartServer::evGetStoredInput()		{ return 4; }
int uiNLAPartServer::evGetData()		{ return 5; }
int uiNLAPartServer::evSaveMisclass()		{ return 6; }
int uiNLAPartServer::evCreateAttrSet()		{ return 7; }
int uiNLAPartServer::evCr2DRandomSet()		{ return 8; }
const char* uiNLAPartServer::sKeyUsrCancel()	{ return "User cancel";  }

#define mDPM DPM(DataPackMgr::PointID())


uiNLAPartServer::uiNLAPartServer( uiApplService& a )
	: uiApplPartServer(a)
	, uidps_(0)
	, dps_(0)
	, storepars_(*new IOPar)
	, is2d_(false)
	, dpsdispmgr_(0)
{
}


uiNLAPartServer::~uiNLAPartServer()
{
    deepErase( inpnms_ );
    if ( dps_ )
	mDPM.release( dps_->id() );
    delete &storepars_;
}


bool uiNLAPartServer::willDoExtraction() const
{
    return creationDesc().doextraction;
}


const BufferStringSet& uiNLAPartServer::modelInputs() const
{
    return getModel().design().inputs;
}


void uiNLAPartServer::getDataPointSets( ObjectSet<DataPointSet>& dpss ) const
{
    const NLACreationDesc& crdesc = creationDesc();

    if ( !crdesc.isdirect )
	PickSetTranslator::createDataPointSets( crdesc.outids, dpss, is2d_ );
    else
    {
	PtrMan<Executor> ex = WellTranslator::createDataPointSets(
				crdesc.outids, crdesc.pars, is2d_ , dpss,
	       			SI().zIsTime() );
	if ( !ex ) return;
	uiTaskRunner uiex( appserv().parent() );
	if ( !uiex.execute(*ex) )
	    deepErase( dpss );
    }

    for ( int idx=0; idx<dpss.size(); idx++ )
    {
	PosVecDataSet& vds = dpss[idx]->dataSet();
	for ( int iinp=0; iinp<crdesc.design.inputs.size(); iinp++ )
	{
	    BufferString psnm = SeisIOObjInfo::defKey2DispName(
		    			crdesc.design.inputs.get(iinp) );
	    vds.add( new DataColDef(psnm) );
	}
    }
}


class uiPrepNLAData : public uiDialog
{
public:

uiPrepNLAData( uiParent* p, const DataPointSet& dps )
    : uiDialog(p,uiDialog::Setup("Data preparation",gtTitle(dps),"dgb:102.2.3"))
    , statsfld_(0)
{
    const BinIDValueSet& bvs = dps.dataSet().data();
    bvs.getColumn( bvs.nrVals() - 1, datavals, false );
    if ( datavals.isEmpty() )
    {
	setCtrlStyle( uiDialog::LeaveOnly );
	new uiLabel( this, "No valid log data values extracted" );
	return;
    }
    sort_array( datavals.arr(), datavals.size() );

    uiGroup* graphgrp = new uiGroup( this, "Graph group" );
    uiStatsDisplay::Setup su; su.withtext(false);
    statsfld_ = new uiStatsDisplay( graphgrp, su );
    statsfld_->setData( datavals.arr(), datavals.size() );
    bsetup_.nrptsperclss = statsfld_->funcDisp()->nrClasses() > 0
		? statsfld_->funcDisp()->nrInpVals() / 
		  statsfld_->funcDisp()->nrClasses() : 1;
    statsfld_->setMarkValue( bsetup_.nrptsperclss, false );

    uiGroup* datagrp = new uiGroup( this, "Data group" );
    dobalfld = new uiGenInput( datagrp, "Balance data", BoolInpSpec(true) );
    dobalfld->valuechanged.notify( mCB(this,uiPrepNLAData,doBalChg) );

    nrptspclssfld = new uiGenInput( datagrp, "Data points per class",
				IntInpSpec(bsetup_.nrptsperclss) );
    nrptspclssfld->attach( alignedBelow, dobalfld );
    nrptspclssfld->valuechanged.notify( mCB(this,uiPrepNLAData,cutoffChg) );
    percnoisefld = new uiGenInput( datagrp, "Percentage noise when adding",
				   FloatInpSpec(bsetup_.noiselvl*100) );
    percnoisefld->attach( alignedBelow, nrptspclssfld );

    rg_.start = datavals[0];
    rg_.stop = datavals[datavals.size()-1];
    valrgfld = new uiGenInput( datagrp, "Data range to use",
	    			FloatInpIntervalSpec(rg_) );
    valrgfld->attach( alignedBelow, percnoisefld );
    valrgfld->valuechanged.notify( mCB(this,uiPrepNLAData,valrgChg) );

    datagrp->attach( centeredBelow, graphgrp );
}

const char* gtTitle( const DataPointSet& dps ) const
{
    const PosVecDataSet& pvds = dps.dataSet();
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
    bsetup_.nrptsperclss = dobal ? nrptspclssfld->getIntValue() : -1;
    statsfld_->setMarkValue( bsetup_.nrptsperclss, false );
}

void cutoffChg( CallBacker* )
{
    bsetup_.nrptsperclss = nrptspclssfld->getIntValue();
    statsfld_->setMarkValue( bsetup_.nrptsperclss, false );
}

void valrgChg( CallBacker* )
{
    rg_ = valrgfld->getFInterval();
    rg_.sort( true );
    TypeSet<float> newdatavals;
    for ( int idx=0; idx<datavals.size(); idx++ )
    {
	if ( rg_.includes( datavals[idx],false ) )
	    newdatavals += datavals[idx];
    }
    statsfld_->setData( newdatavals.arr(), newdatavals.size() );
}

#define mErrRet(s) { uiMSG().error(s); return false; }

bool acceptOK( CallBacker* )
{
    if ( !statsfld_ ) return true;
    dobal_ = dobalfld->getBoolValue();
    if ( dobal_ )
    {
	rg_ = valrgfld->getFInterval();
	rg_.sort();
	bsetup_.nrptsperclss = nrptspclssfld->getIntValue();
	if ( bsetup_.nrptsperclss < 1 || mIsUdf(bsetup_.nrptsperclss) )
	    mErrRet("Please enter a valid number of points per class")
	bsetup_.noiselvl = percnoisefld->getfValue();
	if ( mIsUdf(bsetup_.noiselvl) )
	    bsetup_.noiselvl = 0;
	if ( bsetup_.noiselvl > 100 || bsetup_.noiselvl < -1e-6 )
	    mErrRet("Please enter a valid noise level")
	bsetup_.noiselvl *= 0.01;
    }

    bsetup_.nrclasses = statsfld_->funcDisp()->nrClasses();
    return true;
}

    uiStatsDisplay*	statsfld_;
    uiGenInput*		dobalfld;
    uiGenInput*		valrgfld;
    uiGenInput*		nrptspclssfld;
    uiGenInput*		percnoisefld;

    TypeSet<float>	datavals;

    bool		dobal_;
    Interval<float>	rg_;
    NLADataPreparer::BalanceSetup bsetup_;
};


bool uiNLAPartServer::extractDirectData( ObjectSet<DataPointSet>& dpss )
{
    const NLACreationDesc& crdesc = creationDesc();
    if ( dpss.size() != crdesc.outids.size() )
    {
	if ( DBG::isOn() )
	    DBG::message( "uiNLAPartServer::extractDirectData: "
			  "Nr DataPointSets Sets != Nr. well IDs" );
	return false;
    }

    Well::LogDataExtracter lde( crdesc.outids, dpss, SI().zIsTime() );
    lde.usePar( crdesc.pars );
    uiTaskRunner uiex( appserv().parent() );
    return uiex.execute(lde);
}


class uiLithCodeMan : public uiDialog
{
public:

uiLithCodeMan( uiParent* p, const TypeSet<int>& codes, BufferStringSet& usels,
       		const char* lognm )
    	: uiDialog(p,uiDialog::Setup("Manage codes",
				     "Specify how to handle codes",
				     "dgb:102.2.2"))
	, usrsels(usels)
{
    BufferStringSet opts;
    opts.add( "Use" ).add( "Merge into" ).add( "Drop" );
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
    usrsels.erase();
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
					const ObjectSet<DataPointSet>& dpss,
					const int firstgooddps )
{
    const int valnr = dpss[firstgooddps]->dataSet().data().nrVals() - 1;
    const char* valnm = dpss[firstgooddps]->dataSet().colDef(valnr).name_;

    // Discover the litho codes
    LithCodeData lcd;
    for ( int iset=firstgooddps; iset<dpss.size(); iset++ )
    {
	const BinIDValueSet& bvs = dpss[iset]->dataSet().data();
	BinIDValueSet::Pos pos;
	while( bvs.next(pos) )
	{
	    const float val = bvs.getVals(pos)[valnr];
	    if ( mIsUdf(val) ) continue;
	    const int code = mNINT32(val);
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
	return sKeyUsrCancel();

    lcd.useUserSels( usels );
    for ( int iset=0; iset<dpss.size(); iset++ )
    {
	PosVecDataSet& vds = const_cast<PosVecDataSet&>(dpss[iset]->dataSet());
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
	    ptrtbl += codes.indexOf( toInt(det+1) );
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

	const int code = mNINT32(val);
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


bool uiNLAPartServer::doDPSDlg()
{
    uiDataPointSet::Setup su( "Input data", true );
    su.isconst(false).allowretrieve(false).canaddrow(false);
    uidps_ = new uiDataPointSet( appserv().parent(), dps(), su, dpsdispmgr_ );
    uidps_->setCtrlStyle( uiDialog::DoAndStay );
    uidps_->storePars() = storepars_;
    BufferStringSet bss;
    bss.add( NLACreationDesc::DataTypeNames()[0] );
    bss.add( NLACreationDesc::DataTypeNames()[1] );
    uidps_->setGroupNames( bss );
    uidps_->setGroupType( "Data Set" );
    uidps_->setDeleteOnClose( true );
    return uidps_->go();
}


#undef mErrRet
#define mErrRet(rv) \
{ if ( dps_ ) { mDPM.release( dps_->id() ); dps_ = 0; } return rv; }


DataPointSet& uiNLAPartServer::gtDps() const
{
    uiNLAPartServer& self = *const_cast<uiNLAPartServer*>( this );
    if ( dps_ && dps_->is2D() != is2d_ )
    {
	mDPM.release( dps_->id() );
	self.dps_ = 0;
    }

    if ( !dps_ )
    {
	self.dps_ = new DataPointSet( is2d_ );
	self.dps_->setName( "<NLA train/test data>" );
	mDPM.add( dps_ );
	mDPM.obtain( dps_->id() );
    }

    return *dps_;
}


const char* uiNLAPartServer::prepareInputData( ObjectSet<DataPointSet>& dpss )
{
    const NLACreationDesc& crdesc = creationDesc();

    if ( crdesc.doextraction && crdesc.isdirect )
    {
       if ( !extractDirectData(dpss) )
	    mErrRet(0)

	if ( crdesc.design.classification )
	{
	    int firstgooddps = -1;
	    for ( int iset=0; iset<dpss.size(); iset++ )
	    {
		if ( !dpss[iset]->isEmpty() )
		    { firstgooddps = iset; break; }
	    }
	    if ( firstgooddps == -1 )
		mErrRet("No valid data found")

	    const PosVecDataSet& vds = dpss[firstgooddps]->dataSet();
	    const int orgnrvals = vds.nrCols();
	    const char* res = convertToClasses( dpss, firstgooddps );
	    if ( res ) mErrRet(res)

	    // change design output nodes to new nodes
	    BufferStringSet& outps = const_cast<BufferStringSet&>(
		    				crdesc.design.outputs );
	    outps.erase();
	    const int newnrvals = vds.data().nrVals();
	    for ( int idx=orgnrvals; idx<newnrvals; idx++ )
		outps.add( vds.colDef(idx).ref_ );
	}
    }

    dps().setEmpty();
    const char* res = crdesc.prepareData( dpss, dps() );
    if ( res ) mErrRet(res)

    // allow user to view and edit data
    if ( !doDPSDlg() )
	mErrRet(sKeyUsrCancel())

    bool allok = true;
    if ( crdesc.isdirect && !crdesc.design.classification )
    {
	uiPrepNLAData pddlg( appserv().parent(), dps() );
	allok = pddlg.go();
	if ( allok )
	{
	    BinIDValueSet& bivset = dps().dataSet().data();
	    const int targetcol = bivset.nrVals() - 1;
	    NLADataPreparer dp( bivset, targetcol );
	    dp.removeUndefs(); dp.limitRange( pddlg.rg_ );
	    if ( pddlg.dobal_ )
	    {
		dp.balance( pddlg.bsetup_ );
		dps().dataChanged();
	    }
	}
    }

    if ( allok )
	return 0;

    mErrRet(sKeyUsrCancel())
}
