/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          December 2004
________________________________________________________________________

-*/



#include "uiscalingattrib.h"
#include "scalingattrib.h"

#include "attribengman.h"
#include "attribdesc.h"
#include "attribdescset.h"
#include "attribparam.h"
#include "attribparamgroup.h"
#include "attribprocessor.h"
#include "attribfactory.h"
#include "binnedvalueset.h"
#include "trckeyzsampling.h"
#include "ioobj.h"
#include "flatposdata.h"
#include "seisbuf.h"
#include "seisdatapack.h"
#include "seistrc.h"
#include "seisioobjinfo.h"
#include "seisbufadapters.h"
#include "survinfo.h"
#include "volstatsattrib.h"

#include "uiattribfactory.h"
#include "uiattrsel.h"
#include "uibutton.h"
#include "uigainanalysisdlg.h"
#include "uigeninput.h"
#include "uimsg.h"
#include "uiseislinesel.h"
#include "uiselsurvranges.h"
#include "uitable.h"
#include "uitaskrunner.h"
#include "od_helpids.h"

using namespace Attrib;

static const int initnrrows = 5;
static const int startcol = 0;
static const int stopcol = 1;
static const int factcol = 2;


static const char* statstypestr[] =
{
    "RMS",
    "Mean",
    "Max",
    "User-defined",
    "Detrend",
    0
};


static const char* scalingtypestr[] =
{
    "Z^n",
    "Window",
    "AGC",
    "Squeeze",
    "Gain Correction",
    0
};

static uiWord sDispName()
{
    return od_static_tr("sDispName","Scaling");
}

mInitAttribUI(uiScalingAttrib,Scaling,sDispName(),sBasicGrp())


uiScalingAttrib::uiScalingAttrib( uiParent* p, bool is2d )
	: uiAttrDescEd(p,is2d, mODHelpKey(mScalingAttribHelpID) )
	, parent_(p)
{
    inpfld = createInpFld( is2d );

    typefld = new uiGenInput( this, uiStrings::sType(),
                              StringListInpSpec(scalingtypestr) );
    typefld->valuechanged.notify( mCB(this,uiScalingAttrib,typeSel) );
    typefld->attach( alignedBelow, inpfld );

    nfld = new uiGenInput( this, toUiString("n"), FloatInpSpec() );
    nfld->attach( alignedBelow, typefld );

    sqrgfld = new uiGenInput( this, tr("Value range (empty=unlimited)"),
				FloatInpIntervalSpec() );
    sqrgfld->attach( alignedBelow, typefld );
    squrgfld = new uiGenInput( this, tr("Untouched range (empty=all)"),
				FloatInpIntervalSpec() );
    squrgfld->attach( alignedBelow, sqrgfld );

    statsfld = new uiGenInput( this, tr("Basis"),
        StringListInpSpec(statstypestr) );
    statsfld->attach( alignedBelow, typefld );
    statsfld->valuechanged.notify( mCB(this,uiScalingAttrib,statsSel) );

    tblgrp = new uiGroup( this );
    tblgrp->attach( alignedBelow, statsfld );
    table = new uiTable( tblgrp, uiTable::Setup()
				.rowdesc(tr("Time Gate","Time Period Window"))
				.rowgrow(true)
				.defrowlbl("")
				.fillcol(true)
				.maxrowhgt(1)
				.selmode(uiTable::Multi),
		                                "Define Gate limits" );

    table->setNrCols( 3 );
    table->setNrRows( initnrrows );
    const uiString zunstr = SI().zUnitString();
    table->setColumnLabel( 0, uiStrings::sStart().withUnit( zunstr ) );
    table->setColumnLabel( 1, uiStrings::sStop().withUnit( zunstr ) );
    table->setColumnLabel( 2, uiStrings::sScale() );
    table->setColumnStretchable( startcol, true );
    table->setColumnStretchable( stopcol, true );
    table->setStretch( 2, 0 );
    table->setToolTip( tr("Right-click to add, insert or remove a gate") );

    // for AGC
    uiString label = tr("Window width").withSurvZUnit();
    // TODO: make default value dependent on survey type
    windowfld = new uiGenInput( this, label, FloatInpSpec(200) );
    windowfld->attach( alignedBelow, typefld );

    lowenergymute = new uiGenInput( this, tr("Low energy mute (%)"),
				    FloatInpSpec() );
    lowenergymute->setValue( 0 );
    lowenergymute->attach( alignedBelow, windowfld );

    // for Gain Correction
    analysebut_ = new uiPushButton( this, uiStrings::sAnalyze(),
				    mCB(this,uiScalingAttrib,analyseCB), false);
    analysebut_->attach( alignedBelow, typefld );

    typeSel(0);
    statsSel(0);

    setHAlignObj( inpfld );
}


void uiScalingAttrib::typeSel( CallBacker* )
{
    const int typeval = typefld->getIntValue();
    nfld->display( typeval==0 );

    statsfld->display( typeval==1 );
    tblgrp->display( typeval==1 );

    windowfld->display( typeval==2);
    lowenergymute->display( typeval==2);

    sqrgfld->display( typeval==3 );
    squrgfld->display( typeval==3 );

    analysebut_->display( typeval==4 );
}


void uiScalingAttrib::statsSel( CallBacker* )
{
    const int statstype = statsfld->getIntValue();
    table->hideColumn( 2, statstype!=3 );
}


bool uiScalingAttrib::setParameters( const Desc& desc )
{
    if ( desc.attribName() != Scaling::attribName() )
	return false;

    mIfGetEnum( Scaling::scalingTypeStr(), scalingtype,
	        typefld->setValue(scalingtype) );
    mIfGetFloat( Scaling::powervalStr(), powerval, nfld->setValue(powerval) );
    mIfGetEnum( Scaling::statsTypeStr(), statstype,
				statsfld->setValue(statstype) );
    mIfGetFloat( Scaling::widthStr(), wndwidthval,
		 windowfld->setValue(wndwidthval) );
    mIfGetFloat( Scaling::mutefractionStr(), mutefactor,
				lowenergymute->setValue(mutefactor*100));

    const Attrib::ValParam* vp = desc.getValParam(Scaling::sqrangeStr());
    if ( vp )
	sqrgfld->setValue( Interval<float>(vp->getFValue(0),vp->getFValue(1)) );
    vp = desc.getValParam(Scaling::squntouchedStr());
    if ( vp )
	squrgfld->setValue( Interval<float>(vp->getFValue(0),vp->getFValue(1)));

    int nrtgs = 0;
    if ( desc.getParam(Scaling::gateStr()) )
    {
	mDescGetConstParamGroup(ZGateParam,gateset,desc,Scaling::gateStr())
	nrtgs = gateset->size();
    }

    table->clearTable();
    while ( nrtgs > table->nrRows() )
	table->insertRows( 0, 1 );
    while ( nrtgs < table->nrRows() && table->nrRows() > initnrrows )
	table->removeRow( 0 );
    if ( desc.getParam(Scaling::gateStr()) )
    {
	mDescGetConstParamGroup(ZGateParam,gateset,desc,Scaling::gateStr());

	zvals_.erase();
	for ( int idx=0; idx<gateset->size(); idx++ )
	{
	    const ValParam& param = (ValParam&)(*gateset)[idx];

	    if ( typefld->getIntValue() == 4 )
	    {
		if ( idx==0 )
		    zvals_.addIfNew( param.getFValue(0) );
		zvals_.addIfNew( param.getFValue(1) );
	    }
	    else
	    {
		table->setValue( RowCol(idx,startcol), param.getFValue(0) );
		table->setValue( RowCol(idx,stopcol), param.getFValue(1) );
	    }
	}
    }
    if ( desc.getParam(Scaling::factorStr()) )
    {
	mDescGetConstParamGroup(ValParam,factorset,desc,Scaling::factorStr());
	scalefactors_.erase();

	for ( int idx=0; idx< factorset->size(); idx++ )
	{
	    const ValParam& param = (ValParam&)(*factorset)[idx];
	    if ( typefld->getIntValue() == 4 )
		scalefactors_ += param.getFValue(0);
	    else
		table->setValue( RowCol(idx,factcol), param.getFValue(0) );
	}
    }

    typeSel(0);
    statsSel(0);
    return true;
}


bool uiScalingAttrib::setInput( const Desc& desc )
{
    putInp( inpfld, desc, 0 );
    return true;
}


bool uiScalingAttrib::getParameters( Desc& desc )
{
    if ( desc.attribName() != Scaling::attribName() )
	return false;

    mSetEnum( Scaling::scalingTypeStr(), typefld->getIntValue() );
    mSetFloat( Scaling::powervalStr(), nfld->getFValue() );
    mSetEnum( Scaling::statsTypeStr(), statsfld->getIntValue() );
    mSetFloat( Scaling::widthStr(), windowfld->getFValue() );
    mSetFloat( Scaling::mutefractionStr(), lowenergymute->getFValue()/100 );
    mSetFloatInterval( Scaling::sqrangeStr(), sqrgfld->getFInterval() );
    mSetFloatInterval( Scaling::squntouchedStr(), squrgfld->getFInterval() );

    TypeSet<ZGate> tgs;
    TypeSet<float> factors;

    for ( int idx=0; idx<table->nrRows(); idx++ )
    {
	int start = table->getIntValue( RowCol(idx,startcol) );
	int stop = table->getIntValue( RowCol(idx,stopcol) );
	if ( mIsUdf(start) && mIsUdf(stop) ) continue;

	tgs += ZGate( mCast(float,start), mCast(float,stop) );

	if ( statsfld->getIntValue() == 3 )
	{
	    const char* factstr = table->text( RowCol(idx,factcol) );
	    factors += factstr && *factstr ? toFloat(factstr) : 1;
	}
    }

    TrcKeyZSampling cs;
    if ( typefld->getIntValue() == 4 )
    {
	tgs.erase();
	factors.erase();
	for ( int idx=0; idx<zvals_.size()-1; idx++ )
	{
	    float zstart = zvals_[idx];
	    float zstop = zvals_[idx+1];

	    tgs += ZGate( zstart, zstop );
	    if ( scalefactors_.validIdx(idx) )
		factors += scalefactors_[idx];
	}

	factors += scalefactors_.last();
    }

    mDescGetParamGroup(ZGateParam,gateset,desc,Scaling::gateStr())
    gateset->setSize( tgs.size() );
    for ( int idx=0; idx<tgs.size(); idx++ )
    {
	ZGateParam& zgparam = (ZGateParam&)(*gateset)[idx];
	zgparam.setValue( tgs[idx] );
    }

    mDescGetParamGroup(ValParam,factorset,desc,Scaling::factorStr())
    factorset->setSize( factors.size() );
    for ( int idx=0; idx<factors.size(); idx++ )
    {
	ValParam& valparam = (ValParam&)(*factorset)[idx];
	valparam.setValue(factors[idx] );
    }

    return true;
}


uiRetVal uiScalingAttrib::getInput( Desc& desc )
{
    return fillInp( inpfld, desc, 0 );
}


void uiScalingAttrib::getEvalParams( TypeSet<EvalParam>& params ) const
{
    const int typeval = typefld->getIntValue();
    if ( typeval == 0 )
	params += EvalParam( "n", Scaling::powervalStr() );
    else if ( typeval == 2 )
    {
	params += EvalParam( "Width", Scaling::widthStr() );
	params += EvalParam( "Mute fraction", Scaling::mutefractionStr() );
    }
    else if ( typeval == 3 )
	params += EvalParam( "Untouched range", Scaling::squntouchedStr() );
}


uiRetVal uiScalingAttrib::areUIParsOK()
{
    const int typeval = typefld->getIntValue();
    if ( typeval == 3 )
    {
	if ( sqrgfld->isUndef(0) && sqrgfld->isUndef(1) )
	    return uiRetVal(
		    tr("Please fill in at least one value range limit") );
    }

    return uiRetVal::OK();
}


class uiSelectPositionDlg : public uiDialog
{ mODTextTranslationClass(uiSelectPositionDlg);
public:

    enum DataType		{ Stored2D=0, Stored3D=1, DataPack2D=2,
				  DataPack3D=3 };

uiSelectPositionDlg( uiParent* p,const DataPack::FullID& dpfid )
    : uiDialog(p,uiDialog::Setup(uiStrings::phrSelect(
	      uiStrings::sData().toLower()),tr("For gain analysis"),mNoHelpKey))
    , linesfld_(0)
    , subvolfld_(0)
    , dpfid_(dpfid)
{
    const DataPack::MgrID dpmid = dpfid_.mgrID();
    if ( dpmid!=DataPackMgr::FlatID() && dpmid!=DataPackMgr::SeisID() )
	{ pErrMsg( "Only Flat & Cube DataPacks supported" ); return; }

    const bool is2d = dpmid == DataPackMgr::FlatID();
    createSelFields( is2d ? DataPack2D : DataPack3D );
}

uiSelectPositionDlg( uiParent* p,const DBKey& mid, bool is2d )
    : uiDialog(p,uiDialog::Setup(uiStrings::phrSelect(
	      uiStrings::sData().toLower()),tr("For gain analysis"),mNoHelpKey))
    , linesfld_(0)
    , subvolfld_(0)
    , mid_(mid)
{
    createSelFields( is2d ? Stored2D : Stored3D );
}

int nrTrcs()
{ return nrtrcfld_->getIntValue(); }

Pos::GeomID geomID() const
{ return linesfld_->getInputGeomID(); }

TrcKeyZSampling subVol() const
{
    TrcKeyZSampling cs;
    if ( subvolfld_ )
	cs = subvolfld_->getSampling();
    return cs;
}


protected:

void createSelFields( DataType type )
{
    IntInpSpec nrtrcinpspec( 50 );
    nrtrcfld_ = new uiGenInput( this, tr("Nr of Traces for Examination"),
				nrtrcinpspec );

    const DataPack::FullID::MgrID mgrid = dpfid_.mgrID();
    const DataPack::FullID::PackID dpid = dpfid_.packID();
    if ( type==uiSelectPositionDlg::Stored2D )
    {
	linesfld_ = new uiSeis2DLineNameSel( this, true );
	linesfld_->setDataSet( mid_ );
	linesfld_->attach( alignedBelow, nrtrcfld_ );
    }
    else if ( type==uiSelectPositionDlg::DataPack2D )
    {
        auto fdp = DPM(mgrid).get<FlatDataPack>( dpid );
	Interval<int> trcrglimits( 0, fdp->size(true) );
	nrtrcinpspec.setLimits( trcrglimits );
	nrtrcfld_->setValue( fdp->size(true) );
    }
    else
    {
	subvolfld_ = new uiSelSubvol( this, false );
	subvolfld_->attach( alignedBelow, nrtrcfld_ );
	TrcKeyZSampling cs;
	if ( type==uiSelectPositionDlg::DataPack3D )
	{
            auto cdp = DPM(mgrid).get<RegularSeisDataPack>(dpid);
	    cs = TrcKeyZSampling( cdp->subSel() );
	}
	else if ( type==uiSelectPositionDlg::Stored3D )
	{
	    SeisIOObjInfo seisinfo( mid_ );
	    seisinfo.getRanges( cs );
	}

	subvolfld_->setSampling( cs );
    }
}


bool acceptOK()
{
    if ( !nrtrcfld_->dataInpSpec()->isInsideLimits() )
    {
	uiMSG().error(tr("Number of traces specified is "
			 "more than in the dataset"));
	return false;
    }

    return true;
}

    DataPack::FullID	dpfid_;
    DBKey		mid_;

    uiGenInput*		nrtrcfld_;
    uiSelSubvol*	subvolfld_;
    uiSeis2DLineNameSel*	linesfld_;
};


void uiScalingAttrib::analyseCB( CallBacker* )
{
    Attrib::Desc* inpdesc = !ads_ ? getInputDescFromDP( inpfld )
				  : ads_->getDesc( inpfld->attribID() );
    Attrib::Desc* inpdesccp = new Attrib::Desc( *inpdesc );
    Attrib::Desc* voldesc = PF().createDescCopy( VolStats::attribName() );
    if ( !inpdesccp || !voldesc )
	return;

    PtrMan<Attrib::DescSet> descset =
	ads_ ? ads_->optimizeClone( inpfld->attribID() )
	     : new DescSet( is2D() );
    if ( !descset )
	return;

    Attrib::Desc& desc = *voldesc;
    desc.setInput( 0, inpdesccp );
    Interval<float> timegate(-28,28);
    mSetFloatInterval( VolStats::gateStr(), timegate );
    mSetBinID( VolStats::stepoutStr(), BinID(descset->is2D() ? 0 : 5,5) );
    mSetEnum( VolStats::shapeStr(), 0 );
    mSetBool( VolStats::steeringStr(), false );
    mSetInt( VolStats::nrtrcsStr(), 1 );
    mSetInt( VolStats::optstackstepStr(), 8 );
    desc.selectOutput( 8 );
    desc.setUserRef( "Examine-GainCorrection" );
    desc.updateParams();

    inpdesccp->setDescSet( descset );
    descset->addDesc( inpdesccp );
    Attrib::DescID attribid = descset->addDesc( voldesc );
    PtrMan<Attrib::EngineMan> aem = new Attrib::EngineMan;
    SelSpecList attribspecs;
    SelSpec sp( 0, attribid );
    sp.set( desc );
    attribspecs += sp;
    aem->setAttribSet( descset );
    aem->setAttribSpecs( attribspecs );

    TrcKeyZSampling cs( false );
    const bool isinpindp = dpfids_.size();
    TypeSet<TrcKey> trckeys;
    int nrtrcs = 0;
    if ( !isinpindp )
    {
	PtrMan<IOObj> ioobj = DBKey(inpdesccp->getStoredID(true)).getIOObj();
	if ( !ioobj )
	    { uiMSG().error( tr("Select a valid input") ); return; }

	uiSelectPositionDlg subseldlg( this, ioobj->key(), is2D() );
	if ( !subseldlg.go() )
	    return;

	if ( is2D() )
	{
	    SeisIOObjInfo seisinfo( ioobj );
	    StepInterval<int> trcrg;
	    StepInterval<float> zrg;
	    const Pos::GeomID geomid = subseldlg.geomID();
	    seisinfo.getRanges( geomid, trcrg, zrg );
	    cs.hsamp_.setCrlRange( trcrg );
	    cs.hsamp_.setInlRange( Interval<int>(0,0) );
	    cs.zsamp_ = zrg;
	    aem->setGeomID( geomid );
	}
	else
	    cs = subseldlg.subVol();
	nrtrcs = subseldlg.nrTrcs();
    }
    else
    {
	DataPack::FullID dpfid;
	getInputDPID( inpfld, dpfid );
	uiSelectPositionDlg subseldlg( this, dpfid );
	if ( !subseldlg.go() )
	    return;
	if ( dpfid.mgrID() == DataPackMgr::SeisID() )
	    cs = subseldlg.subVol();
	else
	{
            auto fdp = DPM(dpfid.mgrID()).get<FlatDataPack>( dpfid.packID() );

	    if ( !fdp )
		{ pErrMsg( "No FlatDataPack found" ); return; }

	    StepInterval<double> dtrcrg = fdp->posData().range( true );
	    Interval<int> trcrg( mCast(int,dtrcrg.start),
				 mCast(int,dtrcrg.stop) );
	    cs.hsamp_.setCrlRange( trcrg );
	    cs.hsamp_.setInlRange( Interval<int>(0,0) );

	    StepInterval<double> dzrg = fdp->posData().range( false );
	    StepInterval<float> zrg( mCast(float,dzrg.start),
				     mCast(float,dzrg.stop),
				     mCast(float,dzrg.step) );
	    cs.zsamp_ = zrg;
	}
	nrtrcs = subseldlg.nrTrcs();
    }

    if ( nrtrcs <= 0 )
    {
	uiMSG().error(tr("Number of traces cannot be zero or negative"));
	return;
    }

    cs.hsamp_.getRandomSet( nrtrcs, trckeys );
    aem->setSubSel( Survey::FullSubSel(cs) );

    BinnedValueSet bidvals( 0, false );
    for ( int idx=0; idx<trckeys.size(); idx++ )
       bidvals.add( trckeys[idx].binID() );

    uiRetVal uirv;
    SeisTrcBuf bufs( true );
    Interval<float> zrg( cs.zsamp_ );
    PtrMan<Processor> proc =
	aem->createTrcSelOutput( uirv, bidvals, bufs, 0, &zrg );

    if ( !proc )
	{ uiMSG().error( uirv ); return; }

    uiTaskRunner dlg( parent_ );
    if ( !TaskRunner::execute( &dlg, *proc ) )
	return;

    uiGainAnalysisDlg analdlg( this, bufs, zvals_, scalefactors_);
    analdlg.go();
}
