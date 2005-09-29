/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Helene Payraudeau
 Date:          September 2005
 RCS:           $Id: uiattrtrcselout.cc,v 1.3 2005-09-29 11:29:42 cvshelene Exp $
________________________________________________________________________

-*/


#include "uiattrtrcselout.h"
#include "attribdesc.h"
#include "attribdescset.h"
#include "attriboutput.h"
#include "attribfactory.h"
#include "emsurfacetr.h"
#include "emmanager.h"
#include "uiattrsel.h"
#include "uiseissel.h"
#include "uibinidsubsel.h"
#include "uiioobjsel.h"
#include "uigeninput.h"
#include "ctxtioobj.h"
#include "seistrctr.h"
#include "ioman.h"
#include "ioobj.h"
#include "iopar.h"
#include "nladesign.h"
#include "nlamodel.h"
#include "uimsg.h"
#include "ptrman.h"
#include "multiid.h"

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
{
    setHelpID( "101.2.5" );//TODO find an appropriate id
    setTitleText( "" );

    attrfld = new uiAttrSel( uppgrp, &ads, "Quantity to output" );
    attrfld->setNLAModel( nlamodel );

    if ( usesinglehor_ )
    {
	ctio.ctxt.forread = true;
	objfld = new uiIOObjSel( uppgrp, ctio, "Calculate along surface" );
	objfld->attach( alignedBelow, attrfld );
	objfld->selectiondone.notify( mCB(this,uiAttrTrcSelOut,objSel) );
    }
    else
    {
	ctio.ctxt.forread = true;
	objfld = new uiIOObjSel( uppgrp, ctio, "Calculate between surface:" );
	objfld->attach( alignedBelow, attrfld );
	
	ctio2.ctxt.forread = true;
	obj2fld = new uiIOObjSel( uppgrp, ctio2, "and surface:" );
	obj2fld->attach( alignedBelow, objfld );
	obj2fld->selectiondone.notify( mCB(this,uiAttrTrcSelOut,objSel) );

	extrazfld = new uiGenInput( uppgrp, "Extra Z Interval", BoolInpSpec() );
	extrazfld->setValue( false );
	extrazfld->attach( alignedBelow, obj2fld );
	extrazfld->valuechanged.notify( mCB(this,uiAttrTrcSelOut,extraZSel) );
    }

    const char* gatelabel = "Z Interval required around surfaces";
    gatefld = new uiGenInput( uppgrp, gatelabel, FloatInpIntervalSpec() );
    gatefld->attach( alignedBelow, usesinglehor_ ? (uiGroup*)objfld : 
	    					   (uiGroup*)extrazfld );
    gatefld->display(usesinglehor_);
    
    subselfld = new uiBinIDSubSel( uppgrp, uiBinIDSubSel::Setup().
	    				   withtable(false).withz(false).
					   withstep(true) );
    subselfld->attach( alignedBelow, gatefld );

    const char* outsidevallabel = "value outside computed area";
    outsidevalfld = new uiGenInput( uppgrp, outsidevallabel, FloatInpSpec() );
    outsidevalfld->attach( alignedBelow, subselfld );
    outsidevalfld->setValue(0);
	    
    ctioout.ctxt.forread = false;
    outpfld = new uiSeisSel( uppgrp, ctioout, SeisSelSetup() );
    outpfld->attach( alignedBelow, outsidevalfld );

    uppgrp->setHAlignObj( attrfld );
    addStdFields();
    singmachfld->display( false );
    singmachfld->setValue( true );

    objSel(0);
    extraZSel(0);
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

    Interval<float> zinterval = gatefld->getFInterval();
    if ( Values::isUdf(zinterval.start) )
	zinterval = Interval<float>(0,0);
    BufferString gatestr = zinterval.start; gatestr += "`";
    gatestr += zinterval.stop;
    
    key = keybase; key += "ExtraZInterval";
    iopar.set( key, gatestr );

    key = keybase; key += "Outside Value";
    iopar.set( key, outsidevalfld->getfValue() );
	
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


void uiAttrTrcSelOut::objSel( CallBacker* cb )
{
    if ( !objfld->commitInput(false) || 
	 ( !usesinglehor_ && !obj2fld->commitInput(false) ) ) 
	return;
    
    HorSampling horsampling;
    getComputableSurf( horsampling );
    subselfld->setInput( horsampling );
}


void uiAttrTrcSelOut::extraZSel( CallBacker* cb )
{
    gatefld->display( extrazfld ? extrazfld->getBoolValue() : true );
}


#define mErrRet(str) { uiMSG().message( str ); return false; }

bool uiAttrTrcSelOut::addNLA( DescID& id )
{
    BufferString defstr("NN specification=");
    defstr += nlaid;
    BufferString attribname;
    if ( !Desc::getAttribName(defstr,attribname) )
	mErrRet("Cannot find attribute name");
    RefMan<Desc> ad = PF().createDescCopy( attribname );
    if ( !ad )
    {
	BufferString err = "Cannot find factory-entry for "; err += attribname;
	mErrRet(err);
    }

    if ( !ad->parseDefStr(defstr) )
    {
	BufferString err = "Cannot parse: "; err += defstr;
	mErrRet(err);
    }

    ad->setHidden( true );
    const NLADesign& nlades = nlamodel->design();
    ad->setUserRef( *nlades.outputs[attrfld->outputNr()] );
    ad->selectOutput( attrfld->outputNr() );

    const int nrinputs = nlades.inputs.size();
    for ( int idx=0; idx<nrinputs; idx++ )
    {
        const char* inpname = nlades.inputs[idx]->buf();
	DescID descid = ads.getID( inpname, true );
        if ( descid < 0 && IOObj::isKey(inpname) )
        {
            descid = ads.getID( inpname, false );
            if ( descid < 0 )
            {
                // It could be 'storage', but it's not yet in the old set ...
                PtrMan<IOObj> ioobj = IOM().get( MultiID(inpname) );
                if ( ioobj )
                {
		    BufferString defstr("Storage id="); defstr += inpname;
		    BufferString attribname;
		    if ( !Desc::getAttribName( defstr, attribname ) )
			mErrRet("Cannot find attribute name")
		    RefMan<Desc> newdesc = 
					PF().createDescCopy(attribname);
		    if ( !newdesc )
		    {
			BufferString err = "Cannot find factory-entry for "; 
			err += attribname;
			mErrRet(err);
		    }

		    if ( !newdesc->parseDefStr(defstr) )
		    {
			BufferString err = "Cannot parse: "; err += defstr;
			mErrRet(err);
		    }
                    newdesc->setUserRef( ioobj->name() );
                    descid = ads.addDesc( newdesc );
                }
            }
	}

        ad->setInput( idx, ads.getDesc(descid) );
    }

    id = ads.addDesc( ad );
    if ( id < 0 )
    {
        uiMSG().error( ads.errMsg() );
        return false;
    }

    return true;
}
