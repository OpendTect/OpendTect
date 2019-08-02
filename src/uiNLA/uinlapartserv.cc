/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          May 2001
________________________________________________________________________

-*/

#include "uinlapartserv.h"

#include "binnedvalueset.h"
#include "ioobjctxt.h"
#include "datacoldef.h"
#include "debug.h"
#include "nlacrdesc.h"
#include "nladataprep.h"
#include "od_helpids.h"
#include "picksetmanager.h"
#include "posvecdataset.h"
#include "posvecdatasettr.h"
#include "datapointset.h"
#include "ptrman.h"
#include "seisioobjinfo.h"
#include "sorting.h"
#include "staticstring.h"
#include "survinfo.h"
#include "varlenarray.h"
#include "wellextractdata.h"

#include "uicombobox.h"
#include "uihistogramdisplay.h"
#include "uistatsdisplay.h"
#include "uitaskrunner.h"
#include "uigeninput.h"
#include "uidatapointset.h"
#include "uilabel.h"
#include "uimsg.h"
#include "uistrings.h"

#include <iostream>

int uiNLAPartServer::evPrepareWrite()		{ return 0; }
int uiNLAPartServer::evPrepareRead()		{ return 1; }
int uiNLAPartServer::evReadFinished()		{ return 2; }
int uiNLAPartServer::evGetInputNames()		{ return 3; }
int uiNLAPartServer::evGetStoredInput()		{ return 4; }
int uiNLAPartServer::evGetData()		{ return 5; }
int uiNLAPartServer::evSaveMisclass()		{ return 6; }
int uiNLAPartServer::evCr2DRandomSet()		{ return 7; }
uiString uiNLAPartServer::sKeyUsrCancel()	{ return tr("User cancel");  }

#define mDPM DPM(DataPackMgr::PointID())


uiNLAPartServer::uiNLAPartServer( uiApplService& a )
    : uiApplPartServer(a)
    , dps_(0)
    , storepars_(*new IOPar)
    , is2d_(false)
    , dpsdispmgr_(0)
{
}


uiNLAPartServer::~uiNLAPartServer()
{
    delete &storepars_;
}


bool uiNLAPartServer::willDoExtraction() const
{
    return creationDesc().doextraction;
}


const char* uiNLAPartServer::modelName() const
{
    const NLAModel* mdl = getModel();
    return mdl ? mdl->name() : 0;
}


#define mGetOutIDs() \
	DBKeySet outids; \
	for ( int idx=0; idx<crdesc.outids.size(); idx++ ) \
	    outids += DBKey( crdesc.outids.get( idx ) );


void uiNLAPartServer::getDataPointSets( ObjectSet<DataPointSet>& dpss ) const
{
    const NLACreationDesc& crdesc = creationDesc();

    if ( !crdesc.isdirect )
    {
	for ( int idesc=0; idesc<crdesc.outids.size(); idesc++ )
	{
	    uiRetVal uirv;
	    const DBKey setid( crdesc.outids.get(idesc) );
	    ConstRefMan<Pick::Set> ps = Pick::SetMGR().fetch( setid, uirv );
	    if ( !ps )
		return;

	    DataPointSet* newdps = new DataPointSet( is2d_ );
	    DataPointSet::DataRow dr;
	    Pick::SetIter psiter( *ps );
	    while ( psiter.next() )
	    {
		dr.pos_.set( psiter.get().pos() );
		newdps->addRow( dr );
	    }
	    psiter.retire();
	    newdps->dataChanged();
	    dpss += newdps;
	}
    }
    else
    {
	mGetOutIDs();
	Well::TrackSampler* ts = new Well::TrackSampler( outids, dpss,
							 SI().zIsTime() );
	ts->for2d_ = is2d_;
	ts->usePar( crdesc.pars );
	uiTaskRunner uiex( appserv().parent() );
	if ( !TaskRunner::execute(&uiex,*ts) )
	    deepUnRef( dpss );
	delete ts;
    }

    for ( int idx=0; idx<dpss.size(); idx++ )
    {
	PosVecDataSet& vds = dpss[idx]->dataSet();
	for ( int iinp=0; iinp<crdesc.design.inputs.size(); iinp++ )
	{
	    BufferString psnm = crdesc.design.inputs.get( iinp );
	    if ( DBKey::isValidString(psnm) )
		psnm = DBKey(psnm).name();
	    vds.add( new DataColDef(psnm) );
	}
    }
}


class uiPrepNLAData : public uiDialog
{ mODTextTranslationClass(uiPrepNLAData);
public:

uiPrepNLAData( uiParent* p, const DataPointSet& dps )
    : uiDialog(p,uiDialog::Setup(tr("Data Preparation"),
	       gtUiTitle(dps), mODHelpKey(mPrepNLADataHelpID)))
    , statsfld_(0)
{
    const BinnedValueSet& bvs = dps.dataSet().data();
    bvs.getColumn( bvs.nrVals() - 1, datavals, false );
    if ( datavals.isEmpty() )
    {
	setCtrlStyle( uiDialog::CloseOnly );
	new uiLabel( this, uiStrings::phrCannotFind(tr("Log Data Values")) );
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
    statsfld_->setMarkValue( mCast(float,bsetup_.nrptsperclss), false );

    uiGroup* datagrp = new uiGroup( this, "Data group" );
    dobalfld = new uiGenInput( datagrp, tr("Balance data"), BoolInpSpec(true) );
    dobalfld->valuechanged.notify( mCB(this,uiPrepNLAData,doBalChg) );

    nrptspclssfld = new uiGenInput( datagrp, uiStrings::phrData(tr(
			"points per class")),IntInpSpec(bsetup_.nrptsperclss) );
    nrptspclssfld->attach( alignedBelow, dobalfld );
    nrptspclssfld->valuechanged.notify( mCB(this,uiPrepNLAData,cutoffChg) );
    percnoisefld = new uiGenInput( datagrp, tr("Percentage noise when adding"),
				   FloatInpSpec(bsetup_.noiselvl*100) );
    percnoisefld->attach( alignedBelow, nrptspclssfld );

    rg_.start = datavals[0];
    rg_.stop = datavals[datavals.size()-1];
    valrgfld = new uiGenInput( datagrp, uiStrings::phrData(tr("range to use")),
				FloatInpIntervalSpec(rg_) );
    valrgfld->attach( alignedBelow, percnoisefld );
    valrgfld->valuechanged.notify( mCB(this,uiPrepNLAData,valrgChg) );

    datagrp->attach( centeredBelow, graphgrp );
}

const char* gtTitle( const DataPointSet& dps ) const
{
    const PosVecDataSet& pvds = dps.dataSet();
    const DataColDef& dcd = pvds.colDef( pvds.nrCols()-1 );
    mDeclStaticString( ret );
    ret = "Specify data preparation for '";
    ret += dcd.name_;
    ret += "'";
    return ret.buf();
}


const uiString gtUiTitle( const DataPointSet& dps ) const
 {
    const PosVecDataSet& pvds = dps.dataSet();
    const DataColDef& dcd = pvds.colDef( pvds.nrCols()-1 );
    static uiString ret;
    ret = uiStrings::phrSpecify(od_static_tr("dtUiTitle",
				  "data preparation for '%1'").arg(dcd.name_));
    return ret;
}


void doBalChg( CallBacker* )
{
    const bool dobal = dobalfld->getBoolValue();
    nrptspclssfld->display( dobal );
    percnoisefld->display( dobal );
    bsetup_.nrptsperclss = dobal ? nrptspclssfld->getIntValue() : -1;
    statsfld_->setMarkValue( mCast(float,bsetup_.nrptsperclss), false );
}

void cutoffChg( CallBacker* )
{
    bsetup_.nrptsperclss = nrptspclssfld->getIntValue();
    statsfld_->setMarkValue( mCast(float,bsetup_.nrptsperclss), false );
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

bool acceptOK()
{
    if ( !statsfld_ ) return true;
    dobal_ = dobalfld->getBoolValue();
    if ( dobal_ )
    {
	rg_ = valrgfld->getFInterval();
	rg_.sort();
	bsetup_.nrptsperclss = nrptspclssfld->getIntValue();
	if ( bsetup_.nrptsperclss < 1 || mIsUdf(bsetup_.nrptsperclss) )
	    mErrRet(uiStrings::phrEnter(tr(
					"a valid number of points per class")))
	bsetup_.noiselvl = percnoisefld->getFValue();
	if ( mIsUdf(bsetup_.noiselvl) )
	    bsetup_.noiselvl = 0;
	if ( bsetup_.noiselvl > 100 || bsetup_.noiselvl < -1e-6 )
	    mErrRet(uiStrings::phrEnter(tr("a valid noise level")))
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

    mGetOutIDs();
    Well::LogDataExtracter lde( outids, dpss, SI().zIsTime() );
    lde.usePar( crdesc.pars );
    uiTaskRunner uiex( appserv().parent() );
    return TaskRunner::execute( &uiex, lde );
}


class uiLithCodeMan : public uiDialog
{ mODTextTranslationClass(uiLithCodeMan)
public:

uiLithCodeMan( uiParent* p, const TypeSet<int>& codes, BufferStringSet& usels,
		const char* lognm )
	: uiDialog(p,uiDialog::Setup(uiStrings::phrManage(uiStrings::sCode(2)),
				     uiStrings::phrSpecify(tr(
				     "how to handle codes")),
				     mODHelpKey(mLithCodeManHelpID)))
	, usrsels(usels)
{
    uiStringSet opts;
    opts.add( uiStrings::sUse() );
    opts.add( tr("Merge into") );
    opts.add( tr("Drop") );
    uiLabeledComboBox* prevoptlcb = 0;
    for ( int icode=0; icode<codes.size(); icode++ )
    {
	const int curcode = codes[icode];
	uiString txt = toUiString("%1 %2").arg(uiStrings::sCode())
					    .arg(curcode);
	uiLabeledComboBox* optlcb = new uiLabeledComboBox( this, opts, txt );
	uiComboBox* optbox = optlcb->box();
	BufferString nm( lognm ); nm += " ["; nm += curcode; nm += "]";
	uiGenInput* nmfld = new uiGenInput( this, uiStrings::sName(), nm );
	uiLabeledComboBox* codelcb = new uiLabeledComboBox( this, uiStrings::
								       sCode());
	for ( int ic=0; ic<codes.size(); ic++ )
	{
	    if ( ic == icode ) continue;
	    uiString s = toUiString(codes[ic]);
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


bool acceptOK()
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


const uiString uiNLAPartServer::convertToClasses(
					const ObjectSet<DataPointSet>& dpss,
					const int firstgooddps )
{
    const int valnr = dpss[firstgooddps]->dataSet().data().nrVals() - 1;
    const char* valnm = dpss[firstgooddps]->dataSet().colDef(valnr).name_;

    // Discover the litho codes
    LithCodeData lcd;
    for ( int iset=firstgooddps; iset<dpss.size(); iset++ )
    {
	const BinnedValueSet& bvs = dpss[iset]->dataSet().data();
	BinnedValueSet::SPos pos;
	while( bvs.next(pos) )
	{
	    const float val = bvs.getVals(pos)[valnr];
	    if ( mIsUdf(val) ) continue;
	    const int code = mNINT32(val);
	    if ( !lcd.codes.isPresent(code) )
		lcd.codes += code;
	}
    }

    if ( lcd.codes.size() < 2 )
	return tr("Only one lithology found - need at least 2");
    else if ( lcd.codes.size() > 20 )
	return tr("More than 20 lithologies found - please group lithologies");

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

    return uiString::empty();
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
    BinnedValueSet& bvs = vds.data();
    BinnedValueSet::SPos pos;
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
	    vals[valnr+icode+1] = mCast( float, icode == codeidx ? 1 : 0 );
    }
}


bool uiNLAPartServer::doDPSDlg()
{
    uiDataPointSet::Setup su( uiStrings::sInputData(), true );
    su.isconst(false).allowretrieve(false).canaddrow(false);
    uiDataPointSet uidps( appserv().parent(), *dps(), su, dpsdispmgr_ );
    uidps.setCtrlStyle( uiDialog::RunAndClose );
    uidps.storePars() = storepars_;
    BufferStringSet bss;
    bss.add( NLACreationDesc::DataTypeNames()[0] );
    bss.add( NLACreationDesc::DataTypeNames()[1] );
    uidps.setGroupNames( bss );
    uidps.setGroupType( "Data Set" );
    return uidps.go();
}


#undef mErrRet
#define mErrRet(rv) \
{ if ( dps_ ) { dps_ = 0; } return rv; }


RefMan<DataPointSet> uiNLAPartServer::gtDps() const
{
    uiNLAPartServer& self = *const_cast<uiNLAPartServer*>( this );
    if ( dps_ && dps_->is2D() != is2d_ )
    {
	self.dps_ = 0;
    }

    if ( !dps_ )
    {
	self.dps_ = new DataPointSet( is2d_ );
	self.dps_->setName( "<NLA train/test data>" );
	mDPM.add( self.dps_ );
    }

    return dps_;
}


uiString uiNLAPartServer::prepareInputData( ObjectSet<DataPointSet>& dpss )
{
    const NLACreationDesc& crdesc = creationDesc();

    if ( crdesc.doextraction && crdesc.isdirect )
    {
       if ( !extractDirectData(dpss) )
	    mErrRet(uiString::empty())

	if ( crdesc.design.classification )
	{
	    int firstgooddps = -1;
	    for ( int iset=0; iset<dpss.size(); iset++ )
	    {
		if ( !dpss[iset]->isEmpty() )
		    { firstgooddps = iset; break; }
	    }
	    if ( firstgooddps == -1 )
		mErrRet(uiStrings::sNoValidData())

	    const PosVecDataSet& vds = dpss[firstgooddps]->dataSet();
	    const int orgnrvals = vds.nrCols();
	    const uiString res = convertToClasses( dpss, firstgooddps );
	    if ( !res.isEmpty() ) mErrRet(res)
	    // change design output nodes to new nodes
	    BufferStringSet& outps = const_cast<BufferStringSet&>(
						crdesc.design.outputs );
	    outps.erase();
	    const int newnrvals = vds.data().nrVals();
	    for ( int idx=orgnrvals; idx<newnrvals; idx++ )
		outps.add( vds.colDef(idx).ref_ );
	}
    }

    dps()->setEmpty();
    uiString res = crdesc.prepareData( dpss, *dps() );
    if ( !res.isEmpty() )
	mErrRet(res)

    // allow user to view and edit data
    if ( !doDPSDlg() )
	mErrRet(sKeyUsrCancel())

    bool allok = true;
    if ( crdesc.isdirect && !crdesc.design.classification )
    {
	uiPrepNLAData pddlg( appserv().parent(), *dps() );
	allok = pddlg.go();
	if ( allok )
	{
	    BinnedValueSet& bivset = dps()->dataSet().data();
	    const int targetcol = bivset.nrVals() - 1;
	    NLADataPreparer dp( bivset, targetcol );
	    dp.removeUndefs(); dp.limitRange( pddlg.rg_ );
	    if ( pddlg.dobal_ )
	    {
		dp.balance( pddlg.bsetup_ );
		dps()->dataChanged();
	    }
	}
    }

    if ( allok )
	return uiString::empty();

    mErrRet(sKeyUsrCancel())
}
