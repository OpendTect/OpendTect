/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uinlapartserv.h"

#include "binidvalset.h"
#include "datacoldef.h"
#include "datapointset.h"
#include "debug.h"
#include "ioman.h"
#include "ioobj.h"
#include "nlacrdesc.h"
#include "nladataprep.h"
#include "nlamodel.h"
#include "od_helpids.h"
#include "picksettr.h"
#include "posvecdataset.h"
#include "ptrman.h"
#include "sorting.h"
#include "survinfo.h"
#include "wellextractdata.h"

#include "uicombobox.h"
#include "uidatapointset.h"
#include "uigeninput.h"
#include "uihistogramdisplay.h"
#include "uilabel.h"
#include "uimsg.h"
#include "uistatsdisplay.h"
#include "uistrings.h"
#include "uitaskrunner.h"

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
int uiNLAPartServer::evConfirmWrite()		{ return 9; }
uiString uiNLAPartServer::sKeyUsrCancel()	{ return tr("User cancel");  }

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
    delete uidps_;
    delete &storepars_;
}


bool uiNLAPartServer::willDoExtraction() const
{
    return creationDesc().doextraction;
}


const char* uiNLAPartServer::modelName() const
{
    return getModel().name();
}


IOPar& uiNLAPartServer::modelPars() const
{
    return const_cast<NLAModel&>(getModel()).pars();
}


const BufferStringSet& uiNLAPartServer::modelInputs() const
{
    return getModel().design().inputs_;
}


void uiNLAPartServer::getDataPointSets( RefObjectSet<DataPointSet>& dpss ) const
{
    const NLACreationDesc& crdesc = creationDesc();

    TypeSet<MultiID> mids;
    for ( int idx=0; idx<crdesc.outids.size(); idx++ )
    {
	MultiID key;
	key.fromString( crdesc.outids.get(idx).buf() );
	mids.add( key );
    }

    if ( !crdesc.isdirect )
	PickSetTranslator::createDataPointSets( mids, dpss, is2d_ );
    else
    {
	auto* ts = new Well::TrackSampler( mids, dpss, SI().zIsTime() );
	ts->for2d_ = is2d_;
	ts->usePar( crdesc.pars );
	uiTaskRunner uiex( appserv().parent() );
	if ( !TaskRunner::execute(&uiex,*ts) )
	    dpss.erase();

	delete ts;
    }

    for ( int idx=0; idx<dpss.size(); idx++ )
    {
	PosVecDataSet& vds = dpss[idx]->dataSet();
	for ( int iinp=0; iinp<crdesc.design.inputs_.size(); iinp++ )
	{
	    BufferString psnm = crdesc.design.inputs_.get( iinp );
	    if ( IOObj::isKey(psnm) )
		psnm = IOM().nameOf( psnm.buf() );

	    vds.add( new DataColDef(psnm) );
	}
    }
}


class uiPrepNLAData : public uiDialog
{ mODTextTranslationClass(uiPrepNLAData)
public:

uiPrepNLAData( uiParent* p, const DataPointSet& dps )
    : uiDialog(p,Setup(uiStrings::phrData(tr("preparation")),gtTitle(dps),
		       mODHelpKey(mPrepNLADataHelpID)))
    , statsfld_(0)
{
    const BinIDValueSet& bvs = dps.dataSet().data();
    bvs.getColumn( bvs.nrVals() - 1, datavals, false );
    if ( datavals.isEmpty() )
    {
	setCtrlStyle( uiDialog::CloseOnly );
	new uiLabel( this, uiStrings::phrCannotFind(uiStrings::phrJoinStrings(
		  uiStrings::sLog(),uiStrings::sData(),uiStrings::sValue(2))) );
	return;
    }
    sort_array( datavals.arr(), datavals.size() );

    auto* graphgrp = new uiGroup( this, "Graph group" );
    uiStatsDisplay::Setup su; su.withtext(false);
    statsfld_ = new uiStatsDisplay( graphgrp, su );
    statsfld_->setData( datavals.arr(), datavals.size() );
    bsetup_.nrptsperclss = statsfld_->funcDisp()->nrClasses() > 0
		? statsfld_->funcDisp()->nrInpVals() /
		  statsfld_->funcDisp()->nrClasses() : 1;
    statsfld_->setMarkValue( mCast(float,bsetup_.nrptsperclss), false );

    auto* datagrp = new uiGroup( this, "Data group" );
    dobalfld = new uiGenInput( datagrp, tr("Balance data"), BoolInpSpec(true) );
    dobalfld->valueChanged.notify( mCB(this,uiPrepNLAData,doBalChg) );

    nrptspclssfld = new uiGenInput( datagrp, uiStrings::phrData(tr(
			"points per class")),IntInpSpec(bsetup_.nrptsperclss) );
    nrptspclssfld->attach( alignedBelow, dobalfld );
    nrptspclssfld->valueChanged.notify( mCB(this,uiPrepNLAData,cutoffChg) );
    percnoisefld = new uiGenInput( datagrp, tr("Percentage noise when adding"),
				   FloatInpSpec(bsetup_.noiselvl*100) );
    percnoisefld->attach( alignedBelow, nrptspclssfld );

    rg_.start_ = datavals[0];
    rg_.stop_ = datavals[datavals.size()-1];
    valrgfld = new uiGenInput( datagrp, uiStrings::phrData(tr("range to use")),
				FloatInpIntervalSpec(rg_) );
    valrgfld->attach( alignedBelow, percnoisefld );
    valrgfld->valueChanged.notify( mCB(this,uiPrepNLAData,valrgChg) );

    datagrp->attach( centeredBelow, graphgrp );
}

uiString gtTitle( const DataPointSet& dps ) const
{
    const PosVecDataSet& pvds = dps.dataSet();
    const DataColDef& dcd = pvds.colDef( pvds.nrCols()-1 );

    const uiString ret =
			tr("Specify data preparation for '%1'").arg(dcd.name_);

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

bool acceptOK( CallBacker* ) override
{
    if ( !statsfld_ ) return true;
    dobal_ = dobalfld->getBoolValue();
    if ( dobal_ )
    {
	rg_ = valrgfld->getFInterval();
	rg_.sort();
	bsetup_.nrptsperclss = nrptspclssfld->getIntValue();
	if ( bsetup_.nrptsperclss < 1 || mIsUdf(bsetup_.nrptsperclss) )
	    mErrRet(tr("Please enter a valid number of points per class"))
	bsetup_.noiselvl = percnoisefld->getFValue();
	if ( mIsUdf(bsetup_.noiselvl) )
	    bsetup_.noiselvl = 0;
	if ( bsetup_.noiselvl > 100 || bsetup_.noiselvl < -1e-6 )
	    mErrRet(tr("Please enter a valid noise level"))
	bsetup_.noiselvl *= 0.01f;
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


bool uiNLAPartServer::extractDirectData( RefObjectSet<DataPointSet>& dpss )
{
    const NLACreationDesc& crdesc = creationDesc();
    if ( dpss.size() != crdesc.outids.size() )
    {
	if ( DBG::isOn() )
	    DBG::message( "uiNLAPartServer::extractDirectData: "
			  "Nr DataPointSets Sets != Nr. well IDs" );
	return false;
    }

    TypeSet<MultiID> mids;
    for ( int idx=0; idx<crdesc.outids.size(); idx++ )
    {
	MultiID key;
	key.fromString( crdesc.outids.get(idx).buf() );
	mids.add( key );
    }

    Well::LogDataExtracter lde( mids, dpss, SI().zIsTime() );
    lde.usePar( crdesc.pars );
    uiTaskRunner uiex( appserv().parent() );
    return TaskRunner::execute( &uiex, lde );
}


class uiLithCodeMan : public uiDialog
{ mODTextTranslationClass(uiLithCodeMan)
public:

uiLithCodeMan( uiParent* p, const TypeSet<int>& codes, BufferStringSet& usels,
		const char* lognm )
	: uiDialog(p,Setup(uiStrings::phrManage(uiStrings::sCode(2)),
			   uiStrings::phrSpecify(tr("how to handle codes")),
			   mODHelpKey(mLithCodeManHelpID)))
	, usrsels(usels)
{
    BufferStringSet opts;
    opts.add( "Use" ).add( "Merge into" ).add( "Drop" );
    uiLabeledComboBox* prevoptlcb = 0;
    for ( int icode=0; icode<codes.size(); icode++ )
    {
	const int curcode = codes[icode];
	uiString txt = uiStrings::phrJoinStrings(uiStrings::sCode(),
							   toUiString(curcode));
	auto* optlcb = new uiLabeledComboBox( this, opts, txt );
	uiComboBox* optbox = optlcb->box();
	BufferString nm( lognm ); nm += " ["; nm += curcode; nm += "]";
	auto* nmfld = new uiGenInput( this, uiStrings::sName(), nm );
	auto* codelcb = new uiLabeledComboBox( this, uiStrings::
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


bool acceptOK( CallBacker* ) override
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
					const RefObjectSet<DataPointSet>& dpss,
					const int firstgooddps )
{
    const int valnr = dpss[firstgooddps]->dataSet().data().nrVals() - 1;
    const char* valnm = dpss[firstgooddps]->dataSet().colDef(valnr).name_;

    // Discover the litho codes
    LithCodeData lcd;
    for ( int iset=firstgooddps; iset<dpss.size(); iset++ )
    {
	const BinIDValueSet& bvs = dpss[iset]->dataSet().data();
	BinIDValueSet::SPos pos;
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

    return uiStrings::sEmptyString();
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
    BinIDValueSet::SPos pos;
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
    delete uidps_;
    uidps_ = new uiDataPointSet( appserv().parent(), *dps(), su, dpsdispmgr_ );
    uidps_->setCtrlStyle( uiDialog::RunAndClose );
    uidps_->storePars() = storepars_;
    BufferStringSet bss;
    bss.add( NLACreationDesc::DataTypeNames()[0] );
    bss.add( NLACreationDesc::DataTypeNames()[1] );
    uidps_->setGroupNames( bss );
    uidps_->setGroupType( "Data Set" );
    const bool res = uidps_->go();
    deleteAndNullPtr( uidps_ );
    return res;
}


#undef mErrRet
#define mErrRet(rv) \
{ if ( dps_ ) dps_ = nullptr; return rv; }


RefMan<DataPointSet> uiNLAPartServer::gtDps() const
{
    uiNLAPartServer& self = *const_cast<uiNLAPartServer*>( this );
    if ( dps_ && dps_->is2D() != is2d_ )
	self.dps_ = nullptr;

    if ( !dps_ )
    {
	self.dps_ = new DataPointSet( is2d_ );
	self.dps_->setName( "<NLA train/test data>" );
	mDPM.add( self.dps_ );
    }

    return dps_;
}


uiString uiNLAPartServer::prepareInputData( RefObjectSet<DataPointSet>& dpss )
{
    const NLACreationDesc& crdesc = creationDesc();

    if ( crdesc.doextraction && crdesc.isdirect )
    {
       if ( !extractDirectData(dpss) )
	    mErrRet(uiStrings::sEmptyString())

	if ( crdesc.design.classification_ )
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
						crdesc.design.outputs_ );
	    outps.erase();
	    const int newnrvals = vds.data().nrVals();
	    for ( int idx=orgnrvals; idx<newnrvals; idx++ )
		outps.add( vds.colDef(idx).ref_ );
	}
    }

    dps()->setEmpty();
    uiString res = crdesc.prepareData( dpss, *dps() );
    if ( res.isSet() ) mErrRet(res)

    // allow user to view and edit data
    if ( !doDPSDlg() )
	mErrRet(sKeyUsrCancel())

    bool allok = true;
    if ( crdesc.isdirect && !crdesc.design.classification_ )
    {
	uiPrepNLAData pddlg( appserv().parent(), *dps() );
	allok = pddlg.go();
	if ( allok )
	{
	    BinIDValueSet& bivset = dps()->dataSet().data();
	    const int targetcol = bivset.nrVals() - 1;
	    NLADataPreparer dp( bivset, targetcol );
	    dp.removeUndefs(); dp.limitRange( pddlg.rg_ );
	    if ( pddlg.dobal_ )
		dp.balance( pddlg.bsetup_ );

	    dps()->dataChanged();
	}
    }

    if ( allok )
	return uiStrings::sEmptyString();

    mErrRet(sKeyUsrCancel())
}
