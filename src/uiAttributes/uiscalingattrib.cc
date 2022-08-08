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
#include "binidvalset.h"
#include "trckeyzsampling.h"
#include "ioman.h"
#include "ioobj.h"
#include "linekey.h"
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
#include "uicombobox.h"
#include "uigainanalysisdlg.h"
#include "uigeninput.h"
#include "uimsg.h"
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

mInitAttribUI(uiScalingAttrib,Scaling,"Scaling",sKeyBasicGrp())


uiScalingAttrib::uiScalingAttrib( uiParent* p, bool is2d )
	: uiAttrDescEd(p,is2d, mODHelpKey(mScalingAttribHelpID) )
	, parent_(p)
{
    inpfld = createInpFld( is2d );

    typefld = new uiGenInput( this, uiStrings::sType(),
                              StringListInpSpec(scalingtypestr) );
    typefld->valuechanged.notify( mCB(this,uiScalingAttrib,typeSel) );
    typefld->attach( alignedBelow, inpfld );

    nfld = new uiGenInput( this, tr("n"), FloatInpSpec() );
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
    const uiString zunstr = SI().getUiZUnitString();
    table->setColumnLabel( 0, uiStrings::sStart().withUnit( zunstr ) );
    table->setColumnLabel( 1, uiStrings::sStop().withUnit( zunstr ) );
    table->setColumnLabel( 2, uiStrings::sScale() );
    table->setColumnStretchable( startcol, true );
    table->setColumnStretchable( stopcol, true );
    table->setStretch( 2, 0 );
    table->setToolTip( tr("Right-click to add, insert or remove a gate") );

    // for AGC
    uiString label = tr("Window width %1").arg(SI().getUiZUnitString( true ));
    // TODO: make default value dependent on survey type
    windowfld = new uiGenInput( this, label, FloatInpSpec(200) );
    windowfld->attach( alignedBelow, typefld );

    lowenergymute = new uiGenInput( this, tr("Low energy mute (%)"),
				    FloatInpSpec() );
    lowenergymute->setValue( 0 );
    lowenergymute->attach( alignedBelow, windowfld );

    // for Gain Correction
    analysebut_ = new uiPushButton( this, tr("Analyze"),
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


bool uiScalingAttrib::getInput( Desc& desc )
{
    fillInp( inpfld, desc, 0 );
    return true;
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


bool uiScalingAttrib::areUIParsOK()
{
    const int typeval = typefld->getIntValue();
    if ( typeval == 3 )
    {
	if ( sqrgfld->isUndef(0) && sqrgfld->isUndef(1) )
	{
	    errmsg_ = tr("Please fill in at least one value range limit");
	    return false;
	}
    }

    return true;
}


class uiSelectPositionDlg : public uiDialog
{ mODTextTranslationClass(uiSelectPositionDlg);
public:

    enum DataType		{ Stored2D=0, Stored3D=1, DataPack2D=2,
				  DataPack3D=3 };

uiSelectPositionDlg( uiParent* p,const DataPack::FullID& dpfid )
    : uiDialog(p,uiDialog::Setup(uiStrings::phrSelect(
	      uiStrings::sData().toLower()),tr("For gain analysis"),mNoHelpKey))
    , attribnm_(0)
    , linesfld_(0)
    , subvolfld_(0)
    , dpfid_(dpfid)
{
    const DataPack::MgrID dpmid = dpfid.mgrID();
    if ( dpmid!=DataPackMgr::FlatID() && dpmid!=DataPackMgr::SeisID() )
    {
	pErrMsg( "Only Flat & Cube DataPacks supported" );
	return;
    }

    const bool is2d = dpmid==DataPackMgr::FlatID();
    createSelFields( is2d ? DataPack2D : DataPack3D );
}

uiSelectPositionDlg( uiParent* p,const MultiID& mid, bool is2d,const char* anm )
    : uiDialog(p,uiDialog::Setup(uiStrings::phrSelect(
	      uiStrings::sData().toLower()),tr("For gain analysis"),mNoHelpKey))
    , attribnm_(anm)
    , linesfld_(0)
    , subvolfld_(0)
    , mid_(mid)
{
    createSelFields( is2d ? Stored2D : Stored3D );
}

int nrTrcs()
{ return nrtrcfld_->getIntValue(); }

LineKey lineKey() const
{ return LineKey( linesfld_->box()->text(), attribnm_ ); }

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

    if ( type==uiSelectPositionDlg::Stored2D )
    {
	SeisIOObjInfo seisinfo( mid_ );
	BufferStringSet linenames;
	seisinfo.getLineNames( linenames );
	linesfld_ = new uiLabeledComboBox(this, tr("Gain Analyisis on line:"));
	for ( int idx=0; idx<linenames.size(); idx++ )
	    linesfld_->box()->addItem( toUiString(linenames.get(idx)) );

	linesfld_->attach( alignedBelow, nrtrcfld_ );
    }
    else if ( type==uiSelectPositionDlg::DataPack2D )
    {
	auto fdp = DPM( dpfid_.mgrID() ).get<FlatDataPack>( dpfid_.packID() );
	if ( !fdp )
	    return;

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
	    auto cdp = DPM( dpfid_.mgrID()).get<RegularSeisDataPack>(
							    dpfid_.packID() );
	    if ( !cdp )
		return;

	    cs = cdp->sampling();
	}
	else if ( type==uiSelectPositionDlg::Stored3D )
	{
	    SeisIOObjInfo seisinfo( mid_ );
	    seisinfo.getRanges( cs );
	}

	subvolfld_->setSampling( cs );
    }
}


bool acceptOK( CallBacker* ) override
{
    if ( !nrtrcfld_->dataInpSpec()->isInsideLimits() )
    {
	uiMSG().error(tr("Number of traces specified is "
			 "more than in the dataset"));
	return false;
    }

    return true;
}

    BufferString	attribnm_;
    DataPack::FullID	dpfid_;
    MultiID		mid_;

    uiGenInput*		nrtrcfld_;
    uiSelSubvol*	subvolfld_;
    uiLabeledComboBox*	linesfld_;
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
    TypeSet<SelSpec> attribspecs;
    SelSpec sp( 0, attribid );
    sp.set( desc );
    attribspecs += sp;
    aem->setAttribSet( descset );
    aem->setAttribSpecs( attribspecs );

    TrcKeyZSampling cs( false );
    const bool isinpindp = !dpfids_.isEmpty();
    TypeSet<TrcKey> trckeys;
    int nrtrcs = 0;
    if ( !isinpindp )
    {
	LineKey lk( inpdesccp->getStoredID(true) );
	PtrMan<IOObj> ioobj = IOM().get( MultiID(lk.lineName()) );
	if ( !ioobj )
	    return uiMSG().error( tr("Select a valid input") );

	uiSelectPositionDlg subseldlg( this, ioobj->key(), is2D(),
				       lk.attrName() );
	if ( !subseldlg.go() )
	    return;

	if ( is2D() )
	{
	    SeisIOObjInfo seisinfo( ioobj );
	    StepInterval<int> trcrg;
	    StepInterval<float> zrg;
	    const Pos::GeomID geomid = Survey::GM().getGeomID(
				    subseldlg.lineKey().lineName().buf() );
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

	if ( dpfid.mgrID()==DataPackMgr::SeisID() )
	    cs = subseldlg.subVol();
	else
	{
	    auto fdp = DPM(dpfid.mgrID()).get<FlatDataPack>( dpfid.packID() );
	    if ( !fdp )
	    {
		pErrMsg( "No FlatDataPack found" );
		return;
	    }

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
	return uiMSG().error(tr("Number of traces cannot be zero or negative"));

    cs.hsamp_.getRandomSet( nrtrcs, trckeys );
    aem->setTrcKeyZSampling( cs );

    BinIDValueSet bidvals( 0, false );
    for ( int idx=0; idx<trckeys.size(); idx++ )
       bidvals.add( trckeys[idx].position() );

    uiString errmsg;
    SeisTrcBuf bufs( true );
    Interval<float> zrg( cs.zsamp_ );
    PtrMan<Processor> proc =
	aem->createTrcSelOutput( errmsg, bidvals, bufs, 0, &zrg );

    if ( !proc )
    {
	uiMSG().error( errmsg );
	return;
    }

    uiTaskRunner dlg( parent_ );
    if ( !TaskRunner::execute( &dlg, *proc ) )
	return;

    uiGainAnalysisDlg analdlg( this, bufs, zvals_, scalefactors_);
    analdlg.go();
}
