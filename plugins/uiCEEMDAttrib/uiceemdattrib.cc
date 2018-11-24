/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Paul
 * DATE     : Dec 2012
-*/

#include "uiceemdattrib.h"
#include "ceemdattrib.h"

#include "attribdesc.h"
#include "attribdescset.h"
#include "attribfactory.h"
#include "attribparam.h"
#include "attribparamgroup.h"
#include "keystrs.h"
#include "survinfo.h"
#include "trckeyzsampling.h"
#include "uiattribfactory.h"
#include "uiattrsel.h"
#include "uibutton.h"
#include "uicombobox.h"
#include "uigeninput.h"
#include "uimsg.h"
#include "uiseislinesel.h"
#include "uispinbox.h"
#include "uitrcpositiondlg.h"

using namespace Attrib;

mInitAttribUI(uiCEEMDAttrib,CEEMD,toUiString("CEEMD"),sBasicGrp())

uiCEEMDAttrib::uiCEEMDAttrib( uiParent* p, bool is2d )
	: uiAttrDescEd(p,is2d, mODHelpKey(mCEEMDAttribHelpID) )
	, positiondlg_(0)
	, panelview_( new uiCEEMDPanel(p) )
{
    inpfld_ = createInpFld( is2d );
    setHAlignObj( inpfld_ );

    uiStringSet methodstrs;
    methodstrs.add( tr("Empirical Mode Decomposition (EMD)") );
    methodstrs.add( tr("Ensemble EMD") );
    methodstrs.add( tr("Complete Ensemble EMD") );
    methodfld_ = new uiGenInput( this, uiStrings::sMethod(),
				 StringListInpSpec(methodstrs) );
    methodfld_->attach( alignedBelow, inpfld_ );

    maximffld_ = new uiGenInput( this, tr("Maximum number of IMFs"),
				 IntInpSpec() );
    maximffld_->setElemSzPol(uiObject::Small);
    maximffld_->attach( alignedBelow, methodfld_ );

    stopimffld_ = new uiGenInput( this, tr("IMF threshhold"), FloatInpSpec() );
    stopimffld_->setElemSzPol(uiObject::Small);
    stopimffld_->attach( rightOf, maximffld_ );

    maxsiftfld_ = new uiGenInput( this, tr("Maximum number of Sifts"),
				  IntInpSpec() );
    maxsiftfld_->setElemSzPol(uiObject::Small);
    maxsiftfld_->attach( alignedBelow, maximffld_ );

    stopsiftfld_ = new uiGenInput( this, tr("Sift threshhold"),
				    FloatInpSpec() );
    stopsiftfld_->setElemSzPol(uiObject::Small);
    stopsiftfld_->attach( rightOf, maxsiftfld_ );

    CallBack cbtfpanel = mCB(this, uiCEEMDAttrib, panelTFPush);
    tfpanelbut_ = new uiPushButton( this, tr("Display Time/Frequency panel"),
			mCB(this,uiCEEMDAttrib,panelTFPush), false );
    tfpanelbut_->attach( alignedBelow, maxsiftfld_ );

    uiStringSet attriboutputstrs;
    attriboutputstrs.add( uiStrings::sFrequency() );
    attriboutputstrs.add( tr("Peak Frequency") );
    attriboutputstrs.add( tr("Peak Amplitude") );
    attriboutputstrs.add( tr("IMF Component") );
    attriboutputfld_ = new uiGenInput( this, uiStrings::sOutput(),
				    StringListInpSpec(attriboutputstrs) );
    CallBack cboutsel = mCB(this, uiCEEMDAttrib, outSel);
    attriboutputfld_->valuechanged.notify(cboutsel);
    attriboutputfld_->attach( alignedBelow, tfpanelbut_ );

    const float nyqfreq = 0.5f / SI().zRange().step;
    const float freqscale = zIsTime() ? 1.f : 1000.f;
    const int scalednyfreq = mNINT32( nyqfreq * freqscale );
    outputfreqfld_ = new uiGenInput( this, tr("Output Frequency / Step (Hz)"),
			    IntInpSpec(5,StepInterval<int>(1,scalednyfreq,1)) );
    outputfreqfld_->setElemSzPol( uiObject::Small );
    outputfreqfld_->attach( alignedBelow, attriboutputfld_ );

    stepoutfreqfld_ = new uiGenInput( this, uiString::empty(),
			IntInpSpec(5,StepInterval<int>(1,scalednyfreq/2,1)) );
    stepoutfreqfld_->setElemSzPol(uiObject::Small);
    stepoutfreqfld_->attach( rightOf, outputfreqfld_ );
    prevpar_.setEmpty();

    outputcompfld_ = new uiGenInput( this, tr("Output IMF Component Nr."),
	IntInpSpec() );
    outputcompfld_->setElemSzPol(uiObject::Small);
    outputcompfld_->attach( alignedBelow, attriboutputfld_ );

    outSel(0);
}


bool uiCEEMDAttrib::getParameters( Desc& desc )
{
    if ( FixedString(desc.attribName())!=CEEMD::attribName() )
	return false;

    mSetEnum( CEEMD::emdmethodStr(), methodfld_->getIntValue() );
    const float stopimf = stopimffld_->getFValue();
    mSetFloat( CEEMD::stopimfStr(), stopimf );
    const float stopsift = stopsiftfld_->getFValue();
    mSetFloat( CEEMD::stopsiftStr(), stopsift );
    mSetInt( CEEMD::maxnrimfStr(), maximffld_->getIntValue() );
    mSetInt( CEEMD::maxsiftStr(), maxsiftfld_->getIntValue() );
    mSetInt( CEEMD::outputfreqStr(), outputfreqfld_->getIntValue() );
    mSetInt( CEEMD::stepoutfreqStr(), stepoutfreqfld_->getIntValue() );
    mSetInt( CEEMD::outputcompStr(), outputcompfld_->getIntValue() );
    mSetEnum( CEEMD::attriboutputStr(), attriboutputfld_->getIntValue() );
    mSetBool( CEEMD::usetfpanelStr(), false );

    return true;
}

uiRetVal uiCEEMDAttrib::getInput( Attrib::Desc& desc )
{
    return fillInp( inpfld_, desc, 0 );
}

bool uiCEEMDAttrib::setParameters( const Desc& desc )
{
    if ( FixedString(desc.attribName())!=CEEMD::attribName() )
	return false;

    mIfGetEnum( CEEMD::emdmethodStr(), method,
		methodfld_->setValue(method) )
    mIfGetFloat( CEEMD::stopimfStr(), stopimf,
		stopimffld_->setValue(stopimf) );
    mIfGetFloat( CEEMD::stopsiftStr(), stopsift,
		stopsiftfld_->setValue(stopsift) );
    mIfGetInt( CEEMD::maxnrimfStr(), maximf,
	       maximffld_->setValue(maximf) );
    mIfGetInt( CEEMD::maxsiftStr(), maxsift,
	       maxsiftfld_->setValue(maxsift) );
    mIfGetInt( CEEMD::outputfreqStr(), outputfreq,
	       outputfreqfld_->setValue(outputfreq) );
    mIfGetInt( CEEMD::stepoutfreqStr(), stepoutfreq,
	       stepoutfreqfld_->setValue(stepoutfreq) );
    mIfGetEnum( CEEMD::attriboutputStr(), attriboutput,
		attriboutputfld_->setValue(attriboutput) )
    mIfGetInt( CEEMD::outputcompStr(), outputcomp,
	       outputcompfld_->setValue(outputcomp) );

	outSel(0);

    return true;
}

bool uiCEEMDAttrib::setInput( const Desc& desc )
{
    putInp( inpfld_, desc, 0 );
    return true;
}



bool uiCEEMDAttrib::getOutput( Attrib::Desc& desc )
{
    const int attrsel = attriboutputfld_->getIntValue();
    const bool needoutfreqfld = attrsel == 0;
    const bool needoutcompfld = attrsel == 3;

    const int outidx = needoutfreqfld
	? mCast(int,outputfreqfld_->getIntValue()
		/stepoutfreqfld_->getIntValue()-1)
	: needoutcompfld ? outputcompfld_->getIntValue()-1
			 : 0;
    fillOutput( desc, outidx );
    return true;
}


void uiCEEMDAttrib::outSel( CallBacker* cb )
{
    const int attrsel = attriboutputfld_->getIntValue();
    const bool needoutfreqfld = attrsel == 0;
    const bool needoutcompfld = attrsel == 3;

    outputfreqfld_->display(needoutfreqfld);
    stepoutfreqfld_->display(needoutfreqfld);
    outputcompfld_->display(needoutcompfld);
}


void uiCEEMDAttrib::panelTFPush( CallBacker* cb )
{
    if ( inpfld_->attribID().isInvalid() )
	{ uiMSG().error( tr("Please select Input Data") ); return; }

    DBKey dbkey;
    getInputDBKey( dbkey );

    TrcKeyZSampling tzs;
    inpfld_->getRanges( tzs );
    if ( positiondlg_ ) delete positiondlg_;

    positiondlg_ = new uiTrcPositionDlg( this, tzs, is2d_, dbkey );
    setPrevSel();
    positiondlg_->show();
    positiondlg_->windowClosed.notify(
				mCB(this,uiCEEMDAttrib,viewPanelCB) );
}

void uiCEEMDAttrib::getInputDBKey( DBKey& dbkey ) const
{
    if ( !ads_->is2D() ) return;

    Desc* tmpdesc = ads_->getDesc( inpfld_->attribID() );
    if ( !tmpdesc ) return;

    dbkey = tmpdesc->getStoredID();
}

void uiCEEMDAttrib::setPrevSel()
{
    if ( !positiondlg_ )
    {
	prevpar_.setEmpty();
	return;
    }

    if ( prevpar_.isEmpty() )
	return;

    if ( is2d_ )
    {
	BufferString lnm;
	prevpar_.get( sKey::LineName(), lnm );
	positiondlg_->linesfld_->setInput( lnm );
	positiondlg_->linesfld_->nameChanged.trigger();
	int trcnr;
	prevpar_.get( sKey::TraceNr(), trcnr );
	positiondlg_->trcnrfld_->box()->setValue( trcnr );
	return;
    }

    BinID bid;
    prevpar_.get( sKey::Position(), bid );
    positiondlg_->inlfld_->box()->setValue( bid.inl() );
    positiondlg_->crlfld_->setValue( bid.crl() );
}

void uiCEEMDAttrib::viewPanelCB( CallBacker* )
{
    const int res = positiondlg_->uiResult();
    if ( !res )
	return;

    getPrevSel();
    DescSet* dset = new DescSet( *ads_ );
    DescID ceemdid = createCEEMDDesc( dset );

    const TrcKeyZSampling tzs = positiondlg_->getTrcKeyZSampling();
    const Pos::GeomID gid = positiondlg_->geomID();
    panelview_->compAndDispAttrib( dset, ceemdid, tzs, gid );
}


void uiCEEMDAttrib::getPrevSel()
{
    prevpar_.setEmpty();
    if ( !positiondlg_ )
	return;

    if ( ads_->is2D() )
    {
	const char* sellnm = positiondlg_->linesfld_->getInput();
	prevpar_.set( sKey::LineName(), sellnm );
	prevpar_.set( sKey::TraceNr(),
		      positiondlg_->trcnrfld_->box()->getIntValue() );
	return;
    }

    BinID bid;
    bid.inl() = positiondlg_->inlfld_->box()->getIntValue();
    bid.crl() = positiondlg_->crlfld_->getIntValue();
    prevpar_.set( sKey::Position(), bid );
}


DescID uiCEEMDAttrib::createCEEMDDesc( DescSet* dset ) const
{
    DescID inpid = inpfld_->attribID();
    Desc* newdesc = createNewDesc( dset, inpid,
				   CEEMD::attribName(), 0, "" );
    if ( !newdesc )
	return DescID();

    fillInCEEMDDescParams( newdesc );
    newdesc->updateParams();
    newdesc->setUserRef( "CEEMD" );
    return dset->addDesc( newdesc );
}


Desc* uiCEEMDAttrib::createNewDesc( DescSet* descset, DescID inpid,
					 const char* attribnm,
					 int inpidx, BufferString specref) const
{
    Desc* inpdesc = descset->getDesc( inpid );
    Desc* newdesc = PF().createDescCopy( attribnm );
    if ( !newdesc || !inpdesc )
	return 0;

    newdesc->selectOutput( 0 );
    newdesc->setInput( inpidx, inpdesc );
    newdesc->setIsHidden( true );
    BufferString usrref = "_"; usrref += inpdesc->userRef(); usrref += specref;
    newdesc->setUserRef( usrref );
    return newdesc;
}


#define mSetParam( type, nm, str, fn )\
{\
    mDynamicCastGet(type##Param*, nm, newdesc->getValParam(str))\
    nm->setValue( fn );\
}


void uiCEEMDAttrib::fillInCEEMDDescParams( Desc* newdesc ) const
{
    mSetParam(Enum,method,CEEMD::emdmethodStr(),
	      methodfld_->getIntValue() )
    mSetParam(Float,stopimf,CEEMD::stopimfStr(),stopimffld_->getFValue())
    mSetParam(Int,maxsift,CEEMD::maxsiftStr(), maxsiftfld_->getIntValue())
    mSetParam(Float,stopsift,CEEMD::stopsiftStr(),stopsiftfld_->getFValue())
    mSetParam(Int,maximf,CEEMD::maxnrimfStr(), maximffld_->getIntValue())
    mSetParam(Int,outputfreq,CEEMD::outputfreqStr(),
	outputfreqfld_->getIntValue())
    mSetParam(Int,stepoutfreq,CEEMD::stepoutfreqStr(),
	stepoutfreqfld_->getIntValue())
    mSetParam(Enum,output,CEEMD::attriboutputStr(), 0 )
    mSetParam(Int,outputcomp,CEEMD::outputcompStr(),
	outputcompfld_->getIntValue())
    mSetParam(Bool,usetfpanel,CEEMD::usetfpanelStr(), true)


    }


//______________________________________________________________________

const char* uiCEEMDPanel::getProcName()
{ return "Compute all frequencies for a single trace"; }

const char* uiCEEMDPanel::getPackName()
{ return "Spectral Decomposition time/frequency spectrum"; }

const char* uiCEEMDPanel::getPanelName()
{ return "Time Frequency spectrum"; }
