/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Helene Payraudeau
 Date:          September 2005
 RCS:           $Id: uiattrtrcselout.cc,v 1.14 2006-05-03 15:26:48 cvsbert Exp $
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
    : uiFullBatchDialog( p, "Create Horizon related cube output",
	    		 "process_attrib_em" )
    , ctio(*mMkCtxtIOObj(EMHorizon))
    , ctio2(*mMkCtxtIOObj(EMHorizon))
    , ctioout(*mMkCtxtIOObj(SeisTrc))
    , ads(const_cast<DescSet&>(ad))
    , nlamodel(n)
    , nlaid(mid)
    , usesinglehor_(usesinglehor)
    , extraztopfld(0)
    , extrazbotfld(0)
    , gatefld(0)
    , mainhorfld(0)
    , widthfld(0)
    , addwidthfld(0)
{
    if ( usesinglehor_)
	setHelpID( "104.4.2" );
    else	
	setHelpID( "104.4.1" );

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

    ctio.ctxt.forread = true;
    objfld = new uiIOObjSel( uppgrp, ctio, "Calculate along surface" );
    objfld->attach( alignedBelow, attrfld );
    objfld->selectiondone.notify( mCB(this,uiAttrTrcSelOut,objSel) );

    createSubSelFld( uppgrp );
    createZIntervalFld( uppgrp );
    createOutsideValFld( uppgrp );
    createInterpFld( uppgrp );
    createNrSampFld( uppgrp );
    createOutputFld( uppgrp );

    uppgrp->setHAlignObj( attrfld );
}


void uiAttrTrcSelOut::createTwoHorUI()
{
    uiGroup* maingrp = new uiGroup( uppgrp, "Main group" );
    uiGroup* extragrp = new uiGroup( uppgrp, "Extra Options group" );
    
    createAttrFld( maingrp );
    
    ctio.ctxt.forread = true;
    objfld = new uiIOObjSel( maingrp, ctio,"Calculate between top surface:");
    objfld->attach( alignedBelow, attrfld );
    
    ctio2.ctxt.forread = true;
    obj2fld = new uiIOObjSel( maingrp, ctio2, "and bottom surface:" );
    obj2fld->attach( alignedBelow, objfld );
    obj2fld->selectiondone.notify( mCB(this,uiAttrTrcSelOut,objSel) );

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

    uiSeparator* sep = new uiSeparator( uppgrp, "Main-Extra sep" );
    sep->attach( centeredBelow, maingrp );
    sep->attach( widthSameAs, maingrp );

    uiLabel* extralbl = new uiLabel( uppgrp, "Extra options:" );
    extralbl->attach( alignedBelow, sep );
    
    extragrp->attach( alignedBelow, extralbl );

    uppgrp->setHAlignObj( attrfld );
}


uiAttrTrcSelOut::~uiAttrTrcSelOut()
{
    delete ctio.ioobj;
    delete &ctio;

    if ( !usesinglehor_ )
    {
	delete ctio2.ioobj;
	delete &ctio2;
    }
    
    delete ctioout.ioobj;
    delete &ctioout;
}


void uiAttrTrcSelOut::createAttrFld( uiGroup* grp )
{
    attrfld = new uiAttrSel( grp, &ads, "Quantity to output" );
    attrfld->selectiondone.notify( mCB(this,uiAttrTrcSelOut,attrSel) );
    attrfld->setNLAModel( nlamodel );
}


void uiAttrTrcSelOut::createZIntervalFld( uiGroup* grp )
{
    const char* gatelabel = "Z Interval required around surfaces";
    gatefld = new uiGenInput( grp, gatelabel, FloatInpIntervalSpec() );
    gatefld->attach( alignedBelow, subselfld );
    uiLabel* lbl = new uiLabel( grp, SI().getZUnit() );
    lbl->attach( rightOf, (uiObject*)gatefld );
}


void uiAttrTrcSelOut::createExtraZTopFld( uiGroup* grp )
{
    extraztopfld = new uiGenInput( grp, "plus", IntInpSpec() );
    extraztopfld->setElemSzPol(uiObject::Small);
    extraztopfld->attach( rightOf, objfld );
    extraztopfld->setValue(0);
    uiLabel* toplbl = new uiLabel( grp, SI().getZUnit() );
    toplbl->attach( rightOf, extraztopfld );
}


void uiAttrTrcSelOut::createExtraZBotFld( uiGroup* grp )
{
    extrazbotfld = new uiGenInput( grp, "plus", IntInpSpec() );
    extrazbotfld->setElemSzPol(uiObject::Small);
    extrazbotfld->attach( rightOf, obj2fld );
    extrazbotfld->setValue(0);
    uiLabel* botlbl = new uiLabel( grp, SI().getZUnit() );
    botlbl->attach( rightOf, extrazbotfld );
}


void uiAttrTrcSelOut::createSubSelFld( uiGroup* grp )
{
    subselfld = new uiBinIDSubSel( grp, uiBinIDSubSel::Setup().
	    				   withtable(false).withz(false).
					   withstep(true) );
    subselfld->attach( alignedBelow, usesinglehor_ ? (uiGroup*)objfld
	    					   : (uiGroup*)obj2fld );
}


void uiAttrTrcSelOut::createOutsideValFld( uiGroup* grp )
{
    const char* outsidevallabel = "Value outside computed area";
    outsidevalfld = new uiGenInput( grp, outsidevallabel, FloatInpSpec() );
    outsidevalfld->attach( alignedBelow, usesinglehor_ ? (uiGroup*)gatefld 
	    					       : (uiGroup*)subselfld );
    outsidevalfld->setValue(0);
}


void uiAttrTrcSelOut::createInterpFld( uiGroup* grp )
{
    const char* interplabel = "Interpolate surfaces";
    const char* flbl = "Full interpolation";
    const char* plbl = "Partial interpolation";
    interpfld = new uiGenInput( grp, interplabel, BoolInpSpec(flbl, plbl) );
    interpfld->setValue( true );
    interpfld->setWithCheck( true );
    interpfld->setChecked( true );
    interpfld->valuechanged.notify( mCB(this,uiAttrTrcSelOut,interpSel) );
    interpfld->checked.notify( mCB(this,uiAttrTrcSelOut,interpSel) );
    if ( usesinglehor_ )
	interpfld->attach( alignedBelow, outsidevalfld );
}


void uiAttrTrcSelOut::createNrSampFld( uiGroup* grp )
{
    const char* nrsamplabel = "Interpolate if hole is smaller than N traces";
    nrsampfld = new uiGenInput( grp, nrsamplabel, IntInpSpec() );
    nrsampfld->attach( alignedBelow, interpfld );
    nrsampfld->display( false );
}


void uiAttrTrcSelOut::createAddWidthFld( uiGroup* grp )
{
    BufferString zlabel = createAddWidthLabel();
    addwidthfld = new uiGenInput( grp, zlabel, BoolInpSpec() );
    addwidthfld->setValue( false );
    addwidthfld->attach( alignedBelow, nrsampfld );
    addwidthfld->valuechanged.notify( mCB(this,uiAttrTrcSelOut,
					  extraWidthSel) );
}


void uiAttrTrcSelOut::createWidthFld( uiGroup* grp )
{
    widthfld = new uiGenInput( grp,"Extra interval length", FloatInpSpec() );
    widthfld->attach( alignedBelow, addwidthfld );
    widthfld->checked.notify( mCB(this,uiAttrTrcSelOut,extraWidthSel) );
    widthfld->display( false );
}


void uiAttrTrcSelOut::createMainHorFld( uiGroup* grp )
{
    const char* mainhorlabel = "Main surface";
    mainhorfld = new uiGenInput( grp, mainhorlabel, 
	    			 BoolInpSpec( "Top", "Bottom" ) );
    mainhorfld->attach( alignedBelow, widthfld );
    mainhorfld->display( false );
}

    
void uiAttrTrcSelOut::createOutputFld( uiGroup* grp )
{
    ctioout.ctxt.forread = false;
    outpfld = new uiSeisSel( grp, ctioout, SeisSelSetup() );
    outpfld->attach( alignedBelow, outsidevalfld );
    outpfld->attach( alignedBelow, usesinglehor_ ? nrsampfld : outsidevalfld);
}


bool uiAttrTrcSelOut::prepareProcessing()
{
    if ( !objfld->commitInput(false) )
    {
	uiMSG().error( "Please select first surface" );
	return false;
    }

    if ( !usesinglehor_ && !obj2fld->commitInput(false) )
    {
	uiMSG().error( "Please select second surface" );
	return false;
    }

    attrfld->processInput();
    if ( attrfld->attribID() < 0 && attrfld->outputNr() < 0 )
    {
	uiMSG().error( "Please select the output quantity" );
	return false;
    }

    bool haveoutput = outpfld->commitInput( true );
    if ( !haveoutput || !ctioout.ioobj )
    {
	uiMSG().error( "Please select output" );
	return false;
    }
    
    return true;
}


bool uiAttrTrcSelOut::fillPar( IOPar& iopar )
{
    DescID nladescid( -1, true );
    if ( nlamodel && attrfld->outputNr() >= 0 )
    {
	if ( !nlaid || !(*nlaid) )
	{ 
	    uiMSG().message( "NN needs to be stored before creating volume" ); 
	    return false; 
	}
	if ( !addNLA( nladescid ) )	return false;
    }

    IOPar attrpar( "Attribute Descriptions" );
    ads.fillPar( attrpar );
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
    iopar.set( key, nladescid < 0 ? attrfld->attribID().asInt() 
	    			  : nladescid.asInt() );

    key = keybase; key += SeisTrcStorOutput::seisidkey;
    iopar.set( key, ctioout.ioobj->key() );

    BufferString outnm = outpfld->getInput();
    iopar.set( "Target value", outnm );

    keybase = sKey::Geometry; keybase += ".";
    key = keybase; key += LocationOutput::surfidkey; key += ".0";
    iopar.set( key, ctio.ioobj->key() );

    if ( !usesinglehor_ )
    {
	key = keybase; key += LocationOutput::surfidkey; key += ".1";
	iopar.set( key, ctio2.ioobj->key() );
    }

    PtrMan<IOPar> subselpar = new IOPar;
    subselfld->fillPar( *subselpar );

    HorSampling horsamp; horsamp.usePar( *subselpar );
    if ( horsamp.isEmpty() )
	getComputableSurf( horsamp );

    key = keybase; key += SeisTrcStorOutput::inlrangekey;
    iopar.set( key, horsamp.start.inl, horsamp.stop.inl );

    key = keybase; key += SeisTrcStorOutput::crlrangekey;
    iopar.set( key, horsamp.start.crl, horsamp.stop.crl );

    Interval<float> zinterval;
    if ( gatefld )
	zinterval = gatefld->getFInterval();
    else
    {
	zinterval.start = extraztopfld->getValue();
	zinterval.stop = extrazbotfld->getValue();
    }
    
    BufferString gatestr = zinterval.start; gatestr += "`";
    gatestr += zinterval.stop;
    
    key = keybase; key += "ExtraZInterval";
    iopar.set( key, gatestr );

    key = keybase; key += "Outside Value";
    iopar.set( key, outsidevalfld->getfValue() );

    int nrsamp = 0;
    if ( interpfld->isChecked() )
	nrsamp = interpfld->getBoolValue() ? mUdf(int) 
	    				   : nrsampfld->getIntValue();
    
    key = keybase; key += "Interpolation Stepout";
    iopar.set( key, nrsamp );

    if ( !usesinglehor_ && addwidthfld->getBoolValue() )
    {
	key = keybase; key += "Artificial Width";
	iopar.set( key, widthfld->getfValue() );
	
	key = keybase; key += "Leading Horizon";
	iopar.set( key, mainhorfld->getBoolValue()? 1 : 2 );
    }
    
    ads.removeDesc( nladescid );

    return true;
}


void uiAttrTrcSelOut::getComputableSurf( HorSampling& horsampling )
{
    EM::SurfaceIOData sd;
    EM::EMM().getSurfaceData( ctio.ioobj->key(), sd );

    Interval<int> inlrg(sd.rg.start.inl, sd.rg.stop.inl);
    Interval<int> crlrg(sd.rg.start.crl, sd.rg.stop.crl);

    if ( !usesinglehor_ )
    {
	EM::SurfaceIOData sd2;
	EM::EMM().getSurfaceData( ctio2.ioobj->key(), sd2 );

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
    text += interpfld->isChecked()? ifinterp : ifnointerp;
    return text;
}


void uiAttrTrcSelOut::attrSel( CallBacker* cb )
{
    setParFileNmDef( attrfld->getInput() );
}


void uiAttrTrcSelOut::objSel( CallBacker* cb )
{
    if ( !objfld->commitInput(false) || 
	 ( !usesinglehor_ && !obj2fld->commitInput(false) ) ) 
	return;
    
    HorSampling horsampling;
    getComputableSurf( horsampling );
    uiBinIDSubSel::Data subseldata = subselfld->getInput();
    subseldata.cs_.hrg = horsampling; subselfld->setInput( subseldata );
}


void uiAttrTrcSelOut::interpSel( CallBacker* cb )
{
    nrsampfld->display( interpfld->isChecked() ? !interpfld->getBoolValue() 
	    				       : false );

    if ( !addwidthfld )
	return;

    BufferString text = createAddWidthLabel();
    addwidthfld->setTitleText(text);
}


void uiAttrTrcSelOut::extraWidthSel( CallBacker* cb )
{
    if ( !addwidthfld )
	return;
    widthfld->display( addwidthfld->getBoolValue() );
    mainhorfld->display( addwidthfld->getBoolValue() );
}



#define mErrRet(str) { uiMSG().message( str ); return false; }

bool uiAttrTrcSelOut::addNLA( DescID& id )
{
    BufferString defstr("NN specification=");
    defstr += nlaid;

    const int outputnr = attrfld->outputNr();
    BufferString errmsg;
    Attrib::EngineMan::addNLADesc( defstr, id, ads, outputnr, nlamodel, errmsg);
    if ( errmsg.size() )
	mErrRet( errmsg );

    return true;
}
