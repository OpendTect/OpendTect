/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Helene Payraudeau
 Date:          September 2005
 RCS:           $Id: uiattrtrcselout.cc,v 1.17 2006-07-27 14:42:57 cvshelene Exp $
________________________________________________________________________

-*/


#include "uiattrtrcselout.h"
#include "attribdesc.h"
#include "attribdescset.h"
#include "attribengman.h"
#include "attriboutput.h"
#include "ctxtioobj.h"
#include "emmanager.h"
#include "emsurfacetr.h"
#include "ioman.h"
#include "ioobj.h"
#include "iopar.h"
#include "keystrs.h"
#include "multiid.h"
#include "ptrman.h"
#include "seistrcsel.h"
#include "seistrctr.h"
#include "survinfo.h"

#include "uiattrsel.h"
#include "uibinidsubsel.h"
#include "uigeninput.h"
#include "uiioobjsel.h"
#include "uilabel.h"
#include "uimsg.h"
#include "uiseissel.h"
#include "uiseparator.h"


using namespace Attrib;


uiAttrTrcSelOut::uiAttrTrcSelOut( uiParent* p, const DescSet& ad,
			      const NLAModel* n, const MultiID& mid, 
			      bool usesinglehor )
    : uiFullBatchDialog(p,Setup("Create Horizon related cube output")
	    		  .procprognm("process_attrib_em"))
    , ctio_(*mMkCtxtIOObj(EMHorizon))
    , ctio2_(*mMkCtxtIOObj(EMHorizon))
    , ctioout_(*mMkCtxtIOObj(SeisTrc))
    , ads_(const_cast<DescSet&>(ad))
    , nlamodel_(n)
    , nlaid_(mid)
    , usesinglehor_(usesinglehor)
    , extraztopfld_(0)
    , extrazbotfld_(0)
    , gatefld_(0)
    , mainhorfld_(0)
    , widthfld_(0)
    , addwidthfld_(0)
{
    if ( usesinglehor_)
	setHelpID( usesinglehor_ ? "104.4.2" : "104.4.1" );

    setTitleText( "" );

    if ( usesinglehor_ )
	createSingleHorUI();
    else
	createTwoHorUI();
    
    addStdFields();
    singmachfld->display( false );
    singmachfld->setValue( true );

    objSel(0);
    interpSel(0);
    extraWidthSel(0);
}


void uiAttrTrcSelOut::createSingleHorUI()
{
    createAttrFld( uppgrp );

    ctio_.ctxt.forread = true;
    objfld_ = new uiIOObjSel( uppgrp, ctio_, "Calculate along surface" );
    objfld_->attach( alignedBelow, attrfld_ );
    objfld_->selectiondone.notify( mCB(this,uiAttrTrcSelOut,objSel) );

    createSubSelFld( uppgrp );
    createZIntervalFld( uppgrp );
    createOutsideValFld( uppgrp );
    createInterpFld( uppgrp );
    createNrSampFld( uppgrp );
    createCubeBoundsFlds( uppgrp );
    createOutputFld( uppgrp );

    uppgrp->setHAlignObj( attrfld_ );
}


void uiAttrTrcSelOut::createTwoHorUI()
{
    uiGroup* maingrp = new uiGroup( uppgrp, "Main group" );
    uiGroup* extragrp = new uiGroup( uppgrp, "Extra Options group" );
    
    createAttrFld( maingrp );
    
    ctio_.ctxt.forread = true;
    objfld_ = new uiIOObjSel( maingrp, ctio_,"Calculate between top surface:");
    objfld_->attach( alignedBelow, attrfld_ );
    
    ctio2_.ctxt.forread = true;
    obj2fld_ = new uiIOObjSel( maingrp, ctio2_, "and bottom surface:" );
    obj2fld_->attach( alignedBelow, objfld_ );
    obj2fld_->selectiondone.notify( mCB(this,uiAttrTrcSelOut,objSel) );

    createExtraZTopFld( maingrp );
    createExtraZBotFld( maingrp );
    createSubSelFld( maingrp );
    createOutsideValFld( maingrp );
    createOutputFld( maingrp );
    createInterpFld( extragrp );
    createNrSampFld( extragrp );
    createAddWidthFld( extragrp );
    createWidthFld( extragrp );
    createMainHorFld( extragrp );
    createCubeBoundsFlds( extragrp );

    uiSeparator* sep = new uiSeparator( uppgrp, "Main-Extra sep" );
    sep->attach( centeredBelow, maingrp );
    sep->attach( widthSameAs, maingrp );

    uiLabel* extralbl = new uiLabel( uppgrp, "Extra options:" );
    extralbl->attach( alignedBelow, sep );
    
    extragrp->attach( alignedBelow, extralbl );

    uppgrp->setHAlignObj( attrfld_ );
}


uiAttrTrcSelOut::~uiAttrTrcSelOut()
{
    delete ctio_.ioobj;
    delete &ctio_;

    if ( !usesinglehor_ )
    {
	delete ctio2_.ioobj;
	delete &ctio2_;
    }
    
    delete ctioout_.ioobj;
    delete &ctioout_;
}


void uiAttrTrcSelOut::createAttrFld( uiGroup* grp )
{
    attrfld_ = new uiAttrSel( grp, &ads_, "Quantity to output" );
    attrfld_->selectiondone.notify( mCB(this,uiAttrTrcSelOut,attrSel) );
    attrfld_->setNLAModel( nlamodel_ );
}


void uiAttrTrcSelOut::createZIntervalFld( uiGroup* grp )
{
    const char* gatelabel = "Z Interval required around surfaces";
    gatefld_ = new uiGenInput( grp, gatelabel, FloatInpIntervalSpec() );
    gatefld_->attach( alignedBelow, subselfld_ );
    uiLabel* lbl = new uiLabel( grp, SI().getZUnit() );
    lbl->attach( rightOf, (uiObject*)gatefld_ );
}


void uiAttrTrcSelOut::createExtraZTopFld( uiGroup* grp )
{
    extraztopfld_ = new uiGenInput( grp, "plus", IntInpSpec() );
    extraztopfld_->setElemSzPol(uiObject::Small);
    extraztopfld_->attach( rightOf, objfld_ );
    extraztopfld_->setValue(0);
    uiLabel* toplbl = new uiLabel( grp, SI().getZUnit() );
    toplbl->attach( rightOf, extraztopfld_ );
}


void uiAttrTrcSelOut::createExtraZBotFld( uiGroup* grp )
{
    extrazbotfld_ = new uiGenInput( grp, "plus", IntInpSpec() );
    extrazbotfld_->setElemSzPol(uiObject::Small);
    extrazbotfld_->attach( rightOf, obj2fld_ );
    extrazbotfld_->setValue(0);
    uiLabel* botlbl = new uiLabel( grp, SI().getZUnit() );
    botlbl->attach( rightOf, extrazbotfld_ );
}


void uiAttrTrcSelOut::createSubSelFld( uiGroup* grp )
{
    subselfld_ = new uiBinIDSubSel( grp, uiBinIDSubSel::Setup().
	    				   withtable(false).withz(false).
					   withstep(true) );
    subselfld_->attach( alignedBelow, usesinglehor_ ? (uiGroup*)objfld_
	    					    : (uiGroup*)obj2fld_ );
}


void uiAttrTrcSelOut::createOutsideValFld( uiGroup* grp )
{
    const char* outsidevallabel = "Value outside computed area";
    outsidevalfld_ = new uiGenInput( grp, outsidevallabel, FloatInpSpec() );
    outsidevalfld_->attach( alignedBelow, usesinglehor_ ? (uiGroup*)gatefld_ 
	    					       : (uiGroup*)subselfld_ );
    outsidevalfld_->setValue(0);
}


void uiAttrTrcSelOut::createInterpFld( uiGroup* grp )
{
    const char* interplabel = "Interpolate surfaces";
    const char* flbl = "Full interpolation";
    const char* plbl = "Partial interpolation";
    interpfld_ = new uiGenInput( grp, interplabel, BoolInpSpec(flbl, plbl) );
    interpfld_->setValue( true );
    interpfld_->setWithCheck( true );
    interpfld_->setChecked( true );
    interpfld_->valuechanged.notify( mCB(this,uiAttrTrcSelOut,interpSel) );
    interpfld_->checked.notify( mCB(this,uiAttrTrcSelOut,interpSel) );
    if ( usesinglehor_ )
	interpfld_->attach( alignedBelow, outsidevalfld_ );
}


void uiAttrTrcSelOut::createNrSampFld( uiGroup* grp )
{
    const char* nrsamplabel = "Interpolate if hole is smaller than N traces";
    nrsampfld_ = new uiGenInput( grp, nrsamplabel, IntInpSpec() );
    nrsampfld_->attach( alignedBelow, interpfld_ );
    nrsampfld_->display( false );
}


void uiAttrTrcSelOut::createAddWidthFld( uiGroup* grp )
{
    BufferString zlabel = createAddWidthLabel();
    addwidthfld_ = new uiGenInput( grp, zlabel, BoolInpSpec() );
    addwidthfld_->setValue( false );
    addwidthfld_->attach( alignedBelow, nrsampfld_ );
    addwidthfld_->valuechanged.notify( mCB(this,uiAttrTrcSelOut,
					  extraWidthSel) );
}


void uiAttrTrcSelOut::createWidthFld( uiGroup* grp )
{
    widthfld_ = new uiGenInput( grp,"Extra interval length", FloatInpSpec() );
    widthfld_->attach( alignedBelow, addwidthfld_ );
    widthfld_->checked.notify( mCB(this,uiAttrTrcSelOut,extraWidthSel) );
    widthfld_->display( false );
}


void uiAttrTrcSelOut::createMainHorFld( uiGroup* grp )
{
    const char* mainhorlabel = "Main surface";
    mainhorfld_ = new uiGenInput( grp, mainhorlabel, 
	    			 BoolInpSpec( "Top", "Bottom" ) );
    mainhorfld_->attach( alignedBelow, widthfld_ );
    mainhorfld_->display( false );
}

   
void uiAttrTrcSelOut::createCubeBoundsFlds( uiGroup* grp )
{
    const char* choicelbl = "Define Z limits for the output cube";
    setcubeboundsfld_ = new uiGenInput ( grp, choicelbl, BoolInpSpec() );
    setcubeboundsfld_->attach( alignedBelow, mainhorfld_ ? mainhorfld_ 
	    						: nrsampfld_ );
    setcubeboundsfld_->setValue( false );
    setcubeboundsfld_->valuechanged.notify( mCB(this,uiAttrTrcSelOut,
		                                cubeBoundsSel) );

    cubeboundsfld_ = new uiGenInput ( grp, "Z start/stop", 
	    			      FloatInpIntervalSpec() );
    cubeboundsfld_->attach( alignedBelow, setcubeboundsfld_ );
    cubeboundsfld_->display( false );
}


void uiAttrTrcSelOut::createOutputFld( uiGroup* grp )
{
    ctioout_.ctxt.forread = false;
    outpfld_ = new uiSeisSel( grp, ctioout_, SeisSelSetup() );
    outpfld_->attach( alignedBelow, usesinglehor_ ? cubeboundsfld_ 
	    					  : outsidevalfld_);
}


bool uiAttrTrcSelOut::prepareProcessing()
{
    if ( !objfld_->commitInput(false) )
    {
	uiMSG().error( "Please select first surface" );
	return false;
    }

    if ( !usesinglehor_ && !obj2fld_->commitInput(false) )
    {
	uiMSG().error( "Please select second surface" );
	return false;
    }

    attrfld_->processInput();
    if ( attrfld_->attribID() < 0 && attrfld_->outputNr() < 0 )
    {
	uiMSG().error( "Please select the output quantity" );
	return false;
    }

    bool haveoutput = outpfld_->commitInput( true );
    if ( !haveoutput || !ctioout_.ioobj )
    {
	uiMSG().error( "Please select output" );
	return false;
    }
    
    return true;
}


bool uiAttrTrcSelOut::fillPar( IOPar& iopar )
{
    DescID nladescid( -1, true );
    if ( nlamodel_ && attrfld_->outputNr() >= 0 )
    {
	if ( !nlaid_ || !(*nlaid_) )
	{ 
	    uiMSG().message( "NN needs to be stored before creating volume" ); 
	    return false; 
	}
	if ( !addNLA( nladescid ) )	return false;
    }

    const Attrib::DescID targetid = nladescid < 0 ? attrfld_->attribID()
						  : nladescid;
    
    IOPar attrpar( "Attribute Descriptions" );
    Attrib::DescSet* clonedset = ads_.optimizeClone( targetid );
    if ( !clonedset )
	return false;
    clonedset->fillPar( attrpar );
    
    for ( int idx=0; idx<attrpar.size(); idx++ )
    {
        const char* nm = attrpar.getKey(idx);
        BufferString name(SeisTrcStorOutput::attribkey);
        name += "."; name += nm;
        iopar.add( name, attrpar.getValue(idx) );
    }

    BufferString key;
    BufferString keybase = Output::outputstr; keybase += ".1.";
    key = keybase; key += sKey::Type;
    iopar.set( key, Output::tskey );

    key = keybase; key += SeisTrcStorOutput::attribkey;
    key += "."; key += DescSet::highestIDStr();
    iopar.set( key, 1 );

    key = keybase; key += SeisTrcStorOutput::attribkey; key += ".0";
    iopar.set( key, nladescid < 0 ? attrfld_->attribID().asInt() 
	    			  : nladescid.asInt() );

    key = keybase; key += SeisTrcStorOutput::seisidkey;
    iopar.set( key, ctioout_.ioobj->key() );

    BufferString outnm = outpfld_->getInput();
    iopar.set( "Target value", outnm );

    keybase = sKey::Geometry; keybase += ".";
    key = keybase; key += LocationOutput::surfidkey; key += ".0";
    iopar.set( key, ctio_.ioobj->key() );

    if ( !usesinglehor_ )
    {
	key = keybase; key += LocationOutput::surfidkey; key += ".1";
	iopar.set( key, ctio2_.ioobj->key() );
    }

    PtrMan<IOPar> subselpar = new IOPar;
    subselfld_->fillPar( *subselpar );

    HorSampling horsamp; horsamp.usePar( *subselpar );
    if ( horsamp.isEmpty() )
	getComputableSurf( horsamp );

    key = keybase; key += SeisTrcStorOutput::inlrangekey;
    iopar.set( key, horsamp.start.inl, horsamp.stop.inl );

    key = keybase; key += SeisTrcStorOutput::crlrangekey;
    iopar.set( key, horsamp.start.crl, horsamp.stop.crl );

    Interval<float> zinterval;
    if ( gatefld_ )
	zinterval = gatefld_->getFInterval();
    else
    {
	zinterval.start = extraztopfld_->getValue();
	zinterval.stop = extrazbotfld_->getValue();
    }
    
    BufferString gatestr = zinterval.start; gatestr += "`";
    gatestr += zinterval.stop;
    
    key = keybase; key += "ExtraZInterval";
    iopar.set( key, gatestr );

    key = keybase; key += "Outside Value";
    iopar.set( key, outsidevalfld_->getfValue() );

    int nrsamp = 0;
    if ( interpfld_->isChecked() )
	nrsamp = interpfld_->getBoolValue() ? mUdf(int) 
	    				   : nrsampfld_->getIntValue();
    
    key = keybase; key += "Interpolation Stepout";
    iopar.set( key, nrsamp );

    if ( !usesinglehor_ && addwidthfld_->getBoolValue() )
    {
	key = keybase; key += "Artificial Width";
	iopar.set( key, widthfld_->getfValue() );
	
	key = keybase; key += "Leading Horizon";
	iopar.set( key, mainhorfld_->getBoolValue()? 1 : 2 );
    }
    
    Interval<float> cubezbounds;
    cubezbounds = setcubeboundsfld_->getBoolValue()
				? cubeboundsfld_->getFInterval()
				: Interval<float>( mUdf(float), mUdf(float) );
    BufferString zboundsstr = cubezbounds.start; zboundsstr += "`";
    zboundsstr += cubezbounds.stop;
   
    if ( !mIsUdf( cubezbounds.start ) )
    {
	key = keybase; key += "Z Boundaries";
	iopar.set( key, zboundsstr );
    }
    
    ads_.removeDesc( nladescid );
    delete clonedset;

    return true;
}


void uiAttrTrcSelOut::getComputableSurf( HorSampling& horsampling )
{
    EM::SurfaceIOData sd;
    EM::EMM().getSurfaceData( ctio_.ioobj->key(), sd );

    Interval<int> inlrg(sd.rg.start.inl, sd.rg.stop.inl);
    Interval<int> crlrg(sd.rg.start.crl, sd.rg.stop.crl);

    if ( !usesinglehor_ )
    {
	EM::SurfaceIOData sd2;
	EM::EMM().getSurfaceData( ctio2_.ioobj->key(), sd2 );

	Interval<int> inlrg2(sd2.rg.start.inl, sd2.rg.stop.inl);
	Interval<int> crlrg2(sd2.rg.start.crl, sd2.rg.stop.crl);

	inlrg.start = mMAX( inlrg.start, inlrg2.start);
	inlrg.stop = mMIN( inlrg.stop, inlrg2.stop);
	crlrg.start = mMAX( crlrg.start, crlrg2.start);
	crlrg.stop = mMIN( crlrg.stop, crlrg2.stop);
    }

    horsampling.set( inlrg, crlrg );
}


BufferString uiAttrTrcSelOut::createAddWidthLabel()
{
    BufferString zlabel = "Add fixed interval length to main surface \n";
    BufferString ifinterp = "in case of interpolation conflict";
    BufferString ifnointerp = "in case of holes in second surface";
    BufferString text = zlabel;
    text += interpfld_->isChecked()? ifinterp : ifnointerp;
    return text;
}


void uiAttrTrcSelOut::attrSel( CallBacker* cb )
{
    setParFileNmDef( attrfld_->getInput() );
}


void uiAttrTrcSelOut::objSel( CallBacker* cb )
{
    if ( !objfld_->commitInput(false) || 
	 ( !usesinglehor_ && !obj2fld_->commitInput(false) ) ) 
	return;
    
    HorSampling horsampling;
    getComputableSurf( horsampling );
    uiBinIDSubSel::Data subseldata = subselfld_->getInput();
    subseldata.cs_.hrg = horsampling; subselfld_->setInput( subseldata );
}


void uiAttrTrcSelOut::interpSel( CallBacker* cb )
{
    nrsampfld_->display( interpfld_->isChecked() ? !interpfld_->getBoolValue() 
	    				       : false );

    if ( !addwidthfld_ )
	return;

    BufferString text = createAddWidthLabel();
    addwidthfld_->setTitleText(text);
}


void uiAttrTrcSelOut::extraWidthSel( CallBacker* cb )
{
    if ( !addwidthfld_ )
	return;
    widthfld_->display( addwidthfld_->getBoolValue() );
    mainhorfld_->display( addwidthfld_->getBoolValue() );
}


void uiAttrTrcSelOut::cubeBoundsSel( CallBacker* cb )
{
    cubeboundsfld_->display( setcubeboundsfld_->getBoolValue() );
}


#define mErrRet(str) { uiMSG().message( str ); return false; }

bool uiAttrTrcSelOut::addNLA( DescID& id )
{
    BufferString defstr("NN specification=");
    defstr += nlaid_;

    const int outpnr = attrfld_->outputNr();
    BufferString errmsg;
    Attrib::EngineMan::addNLADesc( defstr, id, ads_, outpnr, nlamodel_, errmsg);
    if ( errmsg.size() )
	mErrRet( errmsg );

    return true;
}
