/*+
   * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
   * AUTHOR   : Nageswara
   * DATE     : Mar 2008
 -*/

static const char* rcsID mUnusedVar = "$Id$";

#include "uistratamp.h"

#include "attribdesc.h"
#include "attribdescset.h"
#include "attribdescsetsholder.h"
#include "attriboutput.h"
#include "cubesampling.h"
#include "emioobjinfo.h"
#include "emsurfacetr.h"
#include "stratamp.h"
#include "survinfo.h"

#include "uiattrsel.h"
#include "uicombobox.h"
#include "uigeninput.h"
#include "uimsg.h"
#include "uiioobjsel.h"
#include "uipossubsel.h"


static const char* statstrs[] = { "Min", "Max", "Average", "RMS", "Sum", 0 };

uiStratAmpCalc::uiStratAmpCalc( uiParent* p )
    : uiFullBatchDialog( p, Setup("StratalAmplitude")
	    		    .procprognm("od_stratamp") )
    , horctio1_(*mMkCtxtIOObj(EMHorizon3D))
    , horctio2_(*mMkCtxtIOObj(EMHorizon3D))
    , isoverwrite_(false)
{
    const Attrib::DescSet* ads = Attrib::DSHolder().getDescSet(false,false);
    inpfld_ = new uiAttrSel( uppgrp_, *ads, "Quantity to output" );
    inpfld_->selectionDone.notify( mCB(this,uiStratAmpCalc,inpSel) );

    winoption_= new uiGenInput( uppgrp_, "Window Option",
	                        BoolInpSpec(true, "Single Horizon",
				"Double Horizon") );
    winoption_->valuechanged.notify( mCB(this,uiStratAmpCalc,choiceSel) );
    winoption_->attach( alignedBelow, inpfld_ );

    horfld1_ = new uiIOObjSel( uppgrp_, horctio1_, "    Horizon" );
    horfld1_->selectionDone.notify( mCB(this,uiStratAmpCalc,inpSel) );
    horfld1_->attach( alignedBelow, winoption_ );

    horfld2_ = new uiIOObjSel( uppgrp_, horctio2_, "Bottom Horizon" );
    horfld2_->selectionDone.notify( mCB(this,uiStratAmpCalc,inpSel) );
    horfld2_->attach( alignedBelow, horfld1_ );

    BufferString lbltxt = "Z Offset ";
    lbltxt += SI().getZUnitString(); lbltxt += " Top";
    tophorshiftfld_ = new uiGenInput( uppgrp_, lbltxt,
	    			      FloatInpSpec(0).setName("Top") );
    tophorshiftfld_->attach( alignedBelow, horfld2_ );
    tophorshiftfld_->setElemSzPol( uiObject::Small );
    bothorshiftfld_ = new uiGenInput( uppgrp_, "Bottom", FloatInpSpec(0) );
    bothorshiftfld_->attach( rightTo, tophorshiftfld_ );
    bothorshiftfld_->setElemSzPol( uiObject::Small );

    rangefld_= new uiPosSubSel( uppgrp_, uiPosSubSel::Setup(false,false) );
    rangefld_->attach( alignedBelow, tophorshiftfld_ );

    ampoptionfld_ = new uiLabeledComboBox( uppgrp_, statstrs,
	    				   "Amplitude Option" );
    ampoptionfld_->attach( alignedBelow, rangefld_ );

    selfld_= new uiGenInput( uppgrp_, "Add result as an attribute to",
			     BoolInpSpec(true,"Top Horizon","Bottom Horizon") );
    selfld_->valuechanged.notify( mCB(this,uiStratAmpCalc,setParFileNameCB) );
    selfld_->attach( alignedBelow, ampoptionfld_ );

    foldfld_ = new uiGenInput( uppgrp_, "Output fold as an extra attribute",
	    		       BoolInpSpec(false) ) ;
    foldfld_->attach( alignedBelow, selfld_ );

    attribnamefld_ = new uiGenInput( uppgrp_, "Attribute name",
			             StringInpSpec("Stratal Amplitude") );
    attribnamefld_->valuechanged.notify(
	    			mCB(this,uiStratAmpCalc,setParFileNameCB) );
    attribnamefld_->attach( alignedBelow, foldfld_ );
    addStdFields( false, true );
    uppgrp_->setHAlignObj( inpfld_ );
    setParFileName();

    postFinalise().notify( mCB(this,uiStratAmpCalc,choiceSel) );
}


uiStratAmpCalc::~uiStratAmpCalc()
{
    delete horctio1_.ioobj; delete &horctio1_;
    delete horctio2_.ioobj; delete &horctio2_;
}


void uiStratAmpCalc::choiceSel( CallBacker* )
{
    usesingle_ = winoption_->getBoolValue();
    horfld1_->setLabelText( usesingle_ ? "    Horizon" 
				       : "Top Horizon" );
    horfld2_->display( !usesingle_ );
    selfld_->display( !usesingle_ );
}


void uiStratAmpCalc::inpSel( CallBacker* )
{
    HorSampling hs;
    getAvailableRange( hs );
    CubeSampling incs( rangefld_->envelope() );
    incs.hrg = hs;
    rangefld_->setInput( incs );
    setParFileName();
}


void uiStratAmpCalc::setParFileName()
{
    BufferString parfnm( "Stratal_Amplitude_" );
    const bool addtotop = usesingle_ || selfld_->getBoolValue();
    addtotop ? parfnm.add( horfld1_->getInput() )
	     : parfnm.add( horfld2_->getInput() );
    parfnm.add( "_").add( attribnamefld_->text() );
    setParFileNmDef( parfnm );
}


void uiStratAmpCalc::setParFileNameCB( CallBacker* )
{
    setParFileName();
}


void uiStratAmpCalc::getAvailableRange( HorSampling& hs )
{
    CubeSampling cs;
    if ( inpfld_->getRanges(cs) )
	hs.limitTo( cs.hrg );

    if ( horfld1_->commitInput() )
    {
	EM::IOObjInfo eminfo( horctio1_.ioobj->key() );
	HorSampling emhs;
	emhs.set( eminfo.getInlRange(), eminfo.getCrlRange() );
	hs.limitTo( emhs );
    }

    if ( horfld2_->commitInput() )
    {
	EM::IOObjInfo eminfo( horctio2_.ioobj->key() );
	HorSampling emhs;
	emhs.set( eminfo.getInlRange(), eminfo.getCrlRange() );
	hs.limitTo( emhs );
    }
}


#define mErrRet(s) { uiMSG().error(s); return false; }

bool uiStratAmpCalc::checkInpFlds()
{
    if ( inpfld_->isEmpty() )
	mErrRet( "Missing Input\nPlease select the input attribute / seismics");
    
    if ( usesingle_ && !horfld1_->commitInput() )
	mErrRet( "Missing Input\nPlease select the input Horizon" );
    if ( !usesingle_ )
    {
	if ( !horfld1_->commitInput() || !horfld2_->commitInput() )
	    mErrRet( "Missing Input\nPlease Check Top / Bottom Horizon" );
    }

    if ( !usesingle_ && horctio1_.ioobj->key() == horctio2_.ioobj->key() )
	      mErrRet( "Select Two Different Horizons" );

    return true;
}


bool uiStratAmpCalc::prepareProcessing()
{
    if ( !checkInpFlds() ) return false;

    const bool addtotop = usesingle_ || selfld_->getBoolValue();
    EM::IOObjInfo eminfo( addtotop ? horctio1_.ioobj->key()
	    			   : horctio2_.ioobj->key() );
    BufferStringSet attrnms;
    eminfo.getAttribNames( attrnms );
    const char* attribnm = attribnamefld_->text();
    isoverwrite_ = false;
    if ( attrnms.indexOf( attribnm ) >= 0 )
    {
	BufferString errmsg = "Attribute name ";
	errmsg.add( attribnm ).add( " already exists, Overwrite?" );
	if ( !uiMSG().askOverwrite(errmsg) )
	    return false;
	else
	    isoverwrite_ = true;
    }

    return true;
}


bool uiStratAmpCalc::fillPar( IOPar& iop )
{
    iop.setYN( StratAmpCalc::sKeySingleHorizonYN(), usesingle_ );
    iop.set( StratAmpCalc::sKeyTopHorizonID() , horctio1_.ioobj->key() );
    if ( !usesingle_ )
	iop.set( StratAmpCalc::sKeyBottomHorizonID(), horctio2_.ioobj->key() );

    const bool addtotop = usesingle_ || selfld_->getBoolValue();
    iop.setYN( StratAmpCalc::sKeyAddToTopYN(), addtotop );
    iop.set( StratAmpCalc::sKeyAmplitudeOption(), ampoptionfld_->box()->text());
    iop.setYN( StratAmpCalc::sKeyOutputFoldYN(), foldfld_->getBoolValue() );
    iop.set( StratAmpCalc::sKeyTopShift(),
	     tophorshiftfld_->getfValue() / SI().zDomain().userFactor() );
    iop.set( StratAmpCalc::sKeyBottomShift(),
	     bothorshiftfld_->getfValue() / SI().zDomain().userFactor() );
    iop.set( StratAmpCalc::sKeyAttribName(), attribnamefld_->text() );
    iop.setYN( StratAmpCalc::sKeyIsOverwriteYN(), isoverwrite_ );

    HorSampling hs;
    getAvailableRange( hs );
    HorSampling inhs = rangefld_->envelope().hrg;
    hs.limitTo( inhs );
    IOPar subselpar;
    hs.fillPar( subselpar );
    subselpar.set( sKey::ZRange(), SI().zRange(false) );
    iop.mergeComp( subselpar, IOPar::compKey(sKey::Output(),sKey::Subsel()) );

    const Attrib::DescID targetid = inpfld_->attribID();
    Attrib::DescSet* clonedset = Attrib::DSHolder().getDescSet(
			    inpfld_->is2D(),false)->optimizeClone( targetid );
    IOPar attrpar( "Attribute Descriptions" );
    if ( !clonedset ) return false;

    clonedset->fillPar( attrpar );
    for ( int idx=0; idx<attrpar.size(); idx++ )
    {
	const char* nm = attrpar.getKey( idx );
	iop.add( IOPar::compKey(Attrib::SeisTrcStorOutput::attribkey(),nm),
		   attrpar .getValue(idx) );
    }

    Attrib::Desc* desc = clonedset->getDesc( targetid );
    BufferString storedid = desc ? desc->getStoredID() : "";
    if ( !storedid.isEmpty() )
	iop.set( "Input Line Set", storedid.buf() );

    const BufferString keybase = IOPar::compKey( Attrib::Output::outputstr(),0);
    const BufferString attribkey =
	    IOPar::compKey( keybase, Attrib::SeisTrcStorOutput::attribkey() );
    iop.set( IOPar::compKey(attribkey,Attrib::DescSet::highestIDStr()), 1 );
    iop.set( IOPar::compKey(attribkey,0), targetid.asInt() );
    iop.set( IOPar::compKey(sKey::Output(), sKey::Type()), sKey::Cube() );
    return true;
}
