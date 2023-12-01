/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiattrtrcselout.h"
#include "attribdesc.h"
#include "attribdescset.h"
#include "attribengman.h"
#include "attriboutput.h"
#include "emioobjinfo.h"
#include "emmanager.h"
#include "emsurfacetr.h"
#include "ioman.h"
#include "ioobj.h"
#include "iopar.h"
#include "keystrs.h"
#include "linekey.h"
#include "multiid.h"
#include "ptrman.h"
#include "seis2ddata.h"
#include "seisselection.h"
#include "seistrctr.h"
#include "survinfo.h"

#include "uiattrsel.h"
#include "uiseissubsel.h"
#include "uibutton.h"
#include "uigeninput.h"
#include "uiioobjsel.h"
#include "uiiosurface.h"
#include "uilabel.h"
#include "uimsg.h"
#include "uiseissel.h"
#include "uibatchjobdispatchersel.h"
#include "od_helpids.h"


using namespace Attrib;

uiAttrTrcSelOut::uiAttrTrcSelOut( uiParent* p, const DescSet& ad,
				  const NLAModel* n, const MultiID& mid,
				  bool usesinglehor )
    : uiAttrEMOut( p, ad, n, mid, "Create Horizon delimited cube output" )
    , usesinglehor_(usesinglehor)
    , extraztopfld_(0)
    , extrazbotfld_(0)
    , gatefld_(0)
    , mainhorfld_(0)
    , widthfld_(0)
    , addwidthfld_(0)
    , interpfld_(0)
    , nrsampfld_(0)
    , xparsdlg_(0)
    , is2d_(ad.is2D())
{
    setCtrlStyle( RunAndClose );
    setHelpKey( usesinglehor_
	? mODHelpKey(mAttrTrcSelOutSliceHelpID)
	: mODHelpKey(mAttrTrcSelOutBetweenHelpID) );

    if ( usesinglehor_ )
	createSingleHorUI();
    else
	createTwoHorUI();

    pargrp_->setHAlignObj( outpfld_ );
    objSel(0);
    if ( usesinglehor_ && !ads_->is2D() )
	interpSel(0);

    if ( usesinglehor_ || ads_->is2D() )
	cubeBoundsSel(0);

    batchjobfld_->jobSpec().pars_.set( IOPar::compKey(sKey::Output(),
				sKey::Type()), Output::tskey() );
}


void uiAttrTrcSelOut::createSingleHorUI()
{
    objfld_ = new uiHorizonSel( pargrp_, is2d_, true,
			      uiStrings::phrCalculate(tr("along Horizon")) );
    objfld_->attach( alignedBelow, attrfld_ );
    objfld_->selectionDone.notify( mCB(this,uiAttrTrcSelOut,objSel) );

    createSubSelFld( pargrp_ );
    createZIntervalFld( pargrp_ );
    createOutsideValFld( pargrp_ );
    if ( !ads_->is2D() )
    {
	createInterpFld( pargrp_ );
	createNrSampFld( pargrp_ );
    }

    createCubeBoundsFlds( pargrp_ );
    createOutputFld( pargrp_ );
}


void uiAttrTrcSelOut::createTwoHorUI()
{
    xparsdlg_ = new uiDialog( pargrp_, uiDialog::Setup(
				tr("Set Extra Options"),mNoDlgTitle,
				mODHelpKey(mAttrTrcSelOutBetweenHelpID)) );
    xparsdlg_->postFinalize().notify( mCB(this,uiAttrTrcSelOut,extraDlgDone) );

    uiIOObjSel::Setup su( tr("Calculate between top Horizon") );
    su.filldef(false);
    objfld_ = new uiHorizonSel( pargrp_, is2d_, true, su );
    objfld_->attach( alignedBelow, attrfld_ );

    su.seltxt( tr("and bottom Horizon") );
    obj2fld_ = new uiHorizonSel( pargrp_, is2d_, true, su );
    obj2fld_->setInput( MultiID("") );
    obj2fld_->attach( alignedBelow, objfld_ );
    obj2fld_->selectionDone.notify( mCB(this,uiAttrTrcSelOut,objSel) );

    createExtraZTopFld( pargrp_ );
    createExtraZBotFld( pargrp_ );
    createSubSelFld( pargrp_ );
    createOutsideValFld( pargrp_ );
    if ( !ads_->is2D() )
    {
	createInterpFld( xparsdlg_ );
	createNrSampFld( xparsdlg_ );
	createAddWidthFld( xparsdlg_ );
	createWidthFld( xparsdlg_ );
	createMainHorFld( xparsdlg_ );
    }

    createCubeBoundsFlds( ads_->is2D() ? (uiParent*) pargrp_
				      : (uiParent*) xparsdlg_ );
    createOutputFld( pargrp_ );

    if ( !ads_->is2D() )
    {
	CallBack cb = mCB(this,uiAttrTrcSelOut,extraParsCB);
	uiPushButton* extrabut =
		new uiPushButton( pargrp_, tr("Extra options"), cb, false );
	extrabut->attach( alignedBelow, outpfld_ );
    }
}


uiAttrTrcSelOut::~uiAttrTrcSelOut()
{
}


void uiAttrTrcSelOut::createZIntervalFld( uiParent* prnt )
{
    const uiString gatelabel = tr("Z Interval required around Horizons");
    gatefld_ = new uiGenInput( prnt, gatelabel,
			FloatInpIntervalSpec().setName("Z Interval Start",0)
					      .setName("Z Interval Stop",1) );
    gatefld_->setValues(0.f, 0.f);
    gatefld_->attach( alignedBelow, seissubselfld_ );
    uiLabel* lbl = new uiLabel( prnt, SI().getUiZUnitString() );
    lbl->attach( rightOf, (uiObject*)gatefld_ );
}


void uiAttrTrcSelOut::createExtraZTopFld( uiParent* prnt )
{
    extraztopfld_ = new uiGenInput( prnt, tr("plus"), FloatInpSpec(0) );
    extraztopfld_->setElemSzPol(uiObject::Small);
    extraztopfld_->attach( rightOf, objfld_ );
    uiLabel* toplbl = new uiLabel( prnt, SI().getUiZUnitString() );
    toplbl->attach( rightOf, extraztopfld_ );
}


void uiAttrTrcSelOut::createExtraZBotFld( uiParent* prnt )
{
    extrazbotfld_ = new uiGenInput( prnt, tr("plus"), FloatInpSpec(0) );
    extrazbotfld_->setElemSzPol(uiObject::Small);
    extrazbotfld_->attach( rightOf, obj2fld_ );
    uiLabel* botlbl = new uiLabel( prnt, SI().getUiZUnitString() );
    botlbl->attach( rightOf, extrazbotfld_ );
}


void uiAttrTrcSelOut::createSubSelFld( uiParent* prnt )
{
    seissubselfld_ = uiSeisSubSel::get( prnt,
	    Seis::SelSetup(ads_->is2D()).onlyrange(false).multiline(true)
				       .withoutz(true).withstep(false) );
    seissubselfld_->attach( alignedBelow, usesinglehor_ ? (uiGroup*)objfld_
						       : (uiGroup*)obj2fld_ );
    mDynamicCastGet( uiSeis2DSubSel* , seis2dsubsel, seissubselfld_ );
    if ( seis2dsubsel )
	seis2dsubsel->selChange.notify(mCB(this,uiAttrTrcSelOut,lineSel));
}


void uiAttrTrcSelOut::createOutsideValFld( uiParent* prnt )
{
    const uiString outsidevallabel = tr("Value outside computed area");
    outsidevalfld_ = new uiGenInput( prnt, outsidevallabel, FloatInpSpec() );
    outsidevalfld_->attach( alignedBelow, usesinglehor_ ? (uiGroup*)gatefld_
						   : (uiGroup*)seissubselfld_ );
    outsidevalfld_->setValue(0);

    uiPushButton* undefbut =
	new uiPushButton( prnt, tr("Set to Undefined"), true );
    undefbut->attach( rightTo, outsidevalfld_ );
    undefbut->activated.notify( mCB(this,uiAttrTrcSelOut,undefCB) );
}


void uiAttrTrcSelOut::undefCB( CallBacker* )
{
    outsidevalfld_->setText( sKey::FloatUdf() );
}


void uiAttrTrcSelOut::createInterpFld( uiParent* prnt )
{
    const uiString interplbl = tr("Interpolate Horizons");
    const uiString flbl = tr("Full");
    const uiString plbl = tr("Partial");
    interpfld_ = new uiGenInput( prnt, interplbl, BoolInpSpec(true,flbl,plbl) );
    interpfld_->setWithCheck( true );
    interpfld_->setChecked( true );
    interpfld_->valueChanged.notify( mCB(this,uiAttrTrcSelOut,interpSel) );
    interpfld_->checked.notify( mCB(this,uiAttrTrcSelOut,interpSel) );
    if ( usesinglehor_ )
	interpfld_->attach( alignedBelow, outsidevalfld_ );
}


void uiAttrTrcSelOut::createNrSampFld( uiParent* prnt )
{
    const uiString nrsamplabel =
		tr("Interpolate if hole is smaller than N traces");
    nrsampfld_ = new uiGenInput( prnt, nrsamplabel,
				 IntInpSpec(0).setName("Interpolate") );
    nrsampfld_->attach( alignedBelow, interpfld_ );
}


void uiAttrTrcSelOut::createAddWidthFld( uiParent* prnt )
{
    const uiString zlabel = createAddWidthLabel();
    addwidthfld_ = new uiGenInput( prnt, zlabel, BoolInpSpec(false) );
    addwidthfld_->attach( alignedBelow, nrsampfld_ );
    addwidthfld_->valueChanged.notify( mCB(this,uiAttrTrcSelOut,extraWidthSel));
}


void uiAttrTrcSelOut::createWidthFld( uiParent* prnt )
{
    widthfld_ = new uiGenInput( prnt, tr("Extra interval length"),
				FloatInpSpec() );
    widthfld_->attach( alignedBelow, addwidthfld_ );
    widthfld_->checked.notify( mCB(this,uiAttrTrcSelOut,extraWidthSel) );
}


void uiAttrTrcSelOut::createMainHorFld( uiParent* prnt )
{
    const uiString mainhorlabel = tr("Main %1").arg(uiStrings::sHorizon());
    mainhorfld_ = new uiGenInput( prnt, mainhorlabel,
				 BoolInpSpec(true,tr("Top"),
				 uiStrings::sBottom()) );
    mainhorfld_->attach( alignedBelow, widthfld_ );
}


void uiAttrTrcSelOut::createCubeBoundsFlds( uiParent* prnt )
{
    const uiString choicelbl = tr("Define Z limits for the output cube");
    setcubeboundsfld_ = new uiGenInput ( prnt, choicelbl, BoolInpSpec(true) );
    setcubeboundsfld_->attach( alignedBelow,
			       mainhorfld_ ? mainhorfld_
					   : nrsampfld_ ? nrsampfld_
							: outsidevalfld_ );
    setcubeboundsfld_->setValue( false );
    setcubeboundsfld_->valueChanged.notify(
	    mCB(this,uiAttrTrcSelOut,cubeBoundsSel) );

    cubeboundsfld_ = new uiGenInput ( prnt, tr("Z Range"),
				      FloatInpIntervalSpec()
				      .setName("Z Start",0)
				      .setName("Z Stop",1) );
    cubeboundsfld_->attach( alignedBelow, setcubeboundsfld_ );
}


void uiAttrTrcSelOut::createOutputFld( uiParent* prnt )
{
    const Seis::GeomType gt = Seis::geomTypeOf( ads_->is2D(), false );
    outpfld_ = new uiSeisSel( prnt, uiSeisSel::ioContext( gt, false ),
			      uiSeisSel::Setup(gt) );
    const bool noadvdlg = usesinglehor_ || ads_->is2D();
    outpfld_->attach( alignedBelow, noadvdlg ? cubeboundsfld_ : outsidevalfld_);
}


bool uiAttrTrcSelOut::prepareProcessing()
{
    if ( !uiAttrEMOut::prepareProcessing() ) return false;

    if ( !objfld_->ioobj(true) )
    {
	uiMSG().error( tr("Please select first horizon") );
	return false;
    }

    if ( !usesinglehor_ && !obj2fld_->ioobj(true) )
    {
	uiMSG().error( tr("Please select second horizon") );
	return false;
    }

    const IOObj* outioobj = outpfld_->ioobj();
    if ( !outioobj )
	return false;

    mDynamicCastGet(uiSeis2DSubSel*,seis2dsubsel,seissubselfld_);
    if ( ads_->is2D() && seis2dsubsel )
    {
	bool lkexists = false;
	Seis2DDataSet dataset( *outioobj );
	if ( seis2dsubsel->isSingLine() )
	    lkexists = dataset.indexOf( seis2dsubsel->selectedLine() ) >= 0;
	else
	    lkexists = dataset.nrLines();

	if ( lkexists )
	{
	    uiString msg( tr("Output attribute already exists. Do you\n"
		    "want to continue and overwrite existing attribute?") );
	    if ( !uiMSG().askOverwrite(msg) ) return false;
	}

	EM::SurfaceIOData data;
	uiString errmsg;
	if ( !EM::EMM().getSurfaceData(objfld_->key(true),data,errmsg) )
	{
	    uiMSG().error( errmsg );
	    return false;
	}
    }

    return true;
}


void uiAttrTrcSelOut::getJobName( BufferString& jobnm ) const
{
    jobnm = outpfld_->ioobj()->name();
}


bool uiAttrTrcSelOut::fillPar( IOPar& iopar )
{
    BufferString outnm = outpfld_->getInput();
    iopar.set( sKey::Target(), outnm );

    if ( !uiAttrEMOut::fillPar(iopar) )
	return false;

    const IOObj* outioobj = outpfld_->ioobj( true );
    if ( !outioobj )
	return false;

    const Desc* desc = ads_->getDesc( attrfld_->attribID() );
    if ( desc && desc->isStored() )
    {
	PtrMan<IOObj> inioobj = IOM().get(
					MultiID(desc->getStoredID().buf()) );
	if ( inioobj )
	{
	    const IOPar& pars = inioobj->pars();
	    BufferString typestr; pars.get( sKey::Type(), typestr );
	    if ( !typestr.isEmpty() )
	    {
		outioobj->pars().set( sKey::Type(), typestr );
		IOM().commitChanges( *outioobj );
	    }
	}
    }

    fillOutPar( iopar, Output::tskey(), SeisTrcStorOutput::seisidkey(),
		outioobj->key() );

    BufferString tmpkey = IOPar::compKey( LocationOutput::surfidkey(), 0);
    BufferString key = IOPar::compKey( sKey::Geometry(), tmpkey );
    iopar.set( key, objfld_->key(true) );

    if ( !usesinglehor_ )
    {
	tmpkey = IOPar::compKey( LocationOutput::surfidkey(), 1);
	key = IOPar::compKey( sKey::Geometry(), tmpkey );
	iopar.set( key, obj2fld_->key(true) );
    }

    PtrMan<IOPar> subselpar = new IOPar;
    if ( !seissubselfld_->fillPar(*subselpar) )
	return false;

    TrcKeySampling horsamp; horsamp.usePar( *subselpar );
    if ( horsamp.isEmpty() )
	getComputableSurf( horsamp );

    const bool is2d = ads_->is2D();
    BufferString typestr;
    subselpar->get( sKey::Type(), typestr );
    const bool issubsel = typestr != sKey::None();
    const bool usesamp = !is2d || issubsel;
    if ( !issubsel && !is2d )
	subselpar->set( sKey::Type(), sKey::Range() );

    if ( usesamp )
    {
	mDynamicCastGet(uiSeis2DSubSel*,seis2dsubsel,seissubselfld_)
	if ( !is2d || (seis2dsubsel && seis2dsubsel->isSingLine()) )
	{
	    key = IOPar::compKey( sKey::Geometry(),
				  SeisTrcStorOutput::inlrangekey() );
	    iopar.set( key, horsamp.start_.inl(), horsamp.stop_.inl() );

	    key = IOPar::compKey( sKey::Geometry(),
				  SeisTrcStorOutput::crlrangekey() );
	    iopar.set( key, horsamp.start_.crl(), horsamp.stop_.crl() );
	}
    }

    TrcKeyZSampling::removeInfo( *subselpar );
    iopar.mergeComp( *subselpar,
		     IOPar::compKey(sKey::Output(),sKey::Subsel()) );

    Interval<float> zinterval;
    if ( gatefld_ )
    {
	zinterval = gatefld_->getFInterval();
	if ( mIsUdf(zinterval.start) )
	    zinterval.start = 0;

	if ( mIsUdf(zinterval.stop) )
	    zinterval.stop = 0;
    }
    else
    {
	zinterval.start = extraztopfld_->getFValue();
	zinterval.stop = extrazbotfld_->getFValue();
    }

    key = IOPar::compKey( sKey::Geometry(), "ExtraZInterval" );
    iopar.set( key, zinterval );

    key = IOPar::compKey( sKey::Geometry(), "Outside Value" );
    iopar.set( key, outsidevalfld_->getFValue() );

    int nrsamp = 0;
    if ( interpfld_ && interpfld_->isChecked() )
	nrsamp = interpfld_->getBoolValue() ? mUdf(int)
					   : nrsampfld_->getIntValue();

    key = IOPar::compKey( sKey::Geometry(), "Interpolation Stepout" );
    iopar.set( key, nrsamp );

    if ( !usesinglehor_ && addwidthfld_ && addwidthfld_->getBoolValue() )
    {
	key = IOPar::compKey( sKey::Geometry(), "Artificial Width" );
	iopar.set( key, widthfld_->getFValue() );

	key = IOPar::compKey( sKey::Geometry(), "Leading Horizon" );
	iopar.set( key, mainhorfld_->getBoolValue()? 1 : 2 );
    }

    Interval<float> cubezbounds;
    cubezbounds = setcubeboundsfld_->getBoolValue()
				? cubeboundsfld_->getFInterval()
				: Interval<float>( mUdf(float), mUdf(float) );
    if ( !mIsUdf(cubezbounds.start) )
    {
	key = IOPar::compKey( sKey::Geometry(), "Z Boundaries" );
	iopar.set( key, cubezbounds );
    }

    batchjobfld_->saveProcPars( *outioobj );
    return true;
}


void uiAttrTrcSelOut::getComputableSurf( TrcKeySampling& trcsampling )
{
    EM::SurfaceIOData sd;
    uiString errmsg;
    if ( !EM::EMM().getSurfaceData(objfld_->key(true),sd,errmsg) )
	return;

    Interval<int> inlrg(sd.rg.start_.inl(), sd.rg.stop_.inl());
    Interval<int> crlrg(sd.rg.start_.crl(), sd.rg.stop_.crl());

    if ( !usesinglehor_ )
    {
	EM::SurfaceIOData sd2;
	if ( !EM::EMM().getSurfaceData(obj2fld_->key(true),sd2,errmsg) )
	    return;

	Interval<int> inlrg2(sd2.rg.start_.inl(), sd2.rg.stop_.inl());
	Interval<int> crlrg2(sd2.rg.start_.crl(), sd2.rg.stop_.crl());

	inlrg.start = mMAX( inlrg.start, inlrg2.start);
	inlrg.stop = mMIN( inlrg.stop, inlrg2.stop);
	crlrg.start = mMAX( crlrg.start, crlrg2.start);
	crlrg.stop = mMIN( crlrg.stop, crlrg2.stop);
    }

    trcsampling.set( inlrg, crlrg );
}


uiString uiAttrTrcSelOut::createAddWidthLabel()
{
    uiString text = tr("Add fixed interval length to Main Horizon");
    uiString ifinterp = tr("in case of interpolation conflict");
    uiString ifnointerp = tr("in case of holes in second Horizon");
    text.append( interpfld_->isChecked() ? ifinterp : ifnointerp, true );
    return text;
}


void uiAttrTrcSelOut::attribSel( CallBacker* )
{
    if ( ads_->is2D() )
    {
	const Desc* desc = ads_->getDesc( attrfld_->attribID() );
	if ( !desc )
	    desc = ads_->getFirstStored();
	if ( desc )
	{
	    LineKey lk( desc->getStoredID(true) );
	    if ( !lk.isEmpty() )
	    {
		PtrMan<IOObj> ioobj = IOM().get(
					    MultiID(lk.lineName().buf()) );
		if ( ioobj )
		    seissubselfld_->setInput( *ioobj );
	    }
	}

	lineSel(0);
    }
}


void uiAttrTrcSelOut::objSel( CallBacker* )
{
    const IOObj* ioobj1 = objfld_->ioobj( true );
    const IOObj* ioobj2 = obj2fld_ ? obj2fld_->ioobj( true ) : nullptr;
    if ( !ioobj1 || (!usesinglehor_ && !ioobj2) )
	return;

    if ( ads_->is2D() )
    {
	EM::IOObjInfo info( ioobj1->key() );
	TypeSet<Pos::GeomID> geomids;
	info.getGeomIDs( geomids );
	if ( !usesinglehor_ )
	{
	    EM::IOObjInfo info2( ioobj2->key() );
	    TypeSet<Pos::GeomID> geomids2;
	    info2.getGeomIDs( geomids2 );
	    for ( int idx=geomids.size()-1; idx>=0; idx-- )
	    {
		if ( !geomids2.isPresent(geomids[idx]) )
		    geomids.removeSingle( idx );
	    }
	}

	mDynamicCastGet( uiSeis2DSubSel* , seis2dsubsel, seissubselfld_ );
	seis2dsubsel->setInputLines( geomids );
    }

    TrcKeyZSampling cs;
    attrfld_->getRanges( cs );

    TrcKeySampling trcsampling;
    getComputableSurf( trcsampling );

    cs.hsamp_.limitTo( trcsampling );
    seissubselfld_->setInput( cs );
}


void uiAttrTrcSelOut::interpSel( CallBacker* )
{
    nrsampfld_->display( interpfld_->isChecked() ? !interpfld_->getBoolValue()
					       : false );
    if ( !addwidthfld_ )
	return;

    const uiString text = createAddWidthLabel();
    addwidthfld_->setTitleText( text );
}


void uiAttrTrcSelOut::extraWidthSel( CallBacker* )
{
    if ( !addwidthfld_ )
	return;

    widthfld_->display( addwidthfld_->getBoolValue(), false );
    mainhorfld_->display( addwidthfld_->getBoolValue(), false );
}


void uiAttrTrcSelOut::cubeBoundsSel( CallBacker* )
{
    cubeboundsfld_->display( setcubeboundsfld_->getBoolValue() );
}


void uiAttrTrcSelOut::extraParsCB( CallBacker* )
{
    xparsdlg_->go();
}


void uiAttrTrcSelOut::extraDlgDone( CallBacker* )
{
    if ( !ads_->is2D() )
    {
	interpSel(0);
	extraWidthSel(0);
    }

    cubeBoundsSel(0);
}


void uiAttrTrcSelOut::lineSel( CallBacker* )
{
    if ( !ads_->is2D() ) return;

    batchjobfld_->jobSpecUpdated();
}
