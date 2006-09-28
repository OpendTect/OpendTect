/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        H. Huck
 Date:          July  2006
 RCS:           $Id: uigapdeconattrib.cc,v 1.12 2006-09-28 16:39:52 cvshelene Exp $
________________________________________________________________________

-*/

#include "uigapdeconattrib.h"
#include "uigdexamacorr.h"
#include "gapdeconattrib.h"

#include "attribdesc.h"
#include "attribdescset.h"
#include "attribfactory.h"
#include "attribparam.h"
#include "survinfo.h"
#include "uiattribfactory.h"
#include "uiattrsel.h"
#include "uislicesel.h"
#include "uibutton.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "uispinbox.h"
#include "cubesampling.h"
#include "volstatsattrib.h"
#include "hilbertattrib.h"
#include "uimsg.h"

using namespace Attrib;


class uiGDPositionDlg: public uiDialog
{
    public:
			uiGDPositionDlg(uiParent*,const CubeSampling&);
			~uiGDPositionDlg();

    void                popUpPosDlg();
    const CubeSampling&	getCubeSampling();

    uiGenInput*		inlcrlfld_;
    CubeSampling	cs_;
    uiSliceSel*		posdlg_; 
};


mInitUI( uiGapDeconAttrib, "GapDecon" )

uiGapDeconAttrib::uiGapDeconAttrib( uiParent* p )
	: uiAttrDescEd ( p )
    	, acorrview_ ( new GapDeconACorrView(0) )
{
    inpfld_ = getInpFld();

    BufferString gatestr = "Correlation window ";
    gatestr += SI().getZUnit();
    gatefld_ = new uiGenInput( this, gatestr, FloatInpIntervalSpec() );
    gatefld_->attach( alignedBelow, inpfld_ );

    CallBack cbexam = mCB(this,uiGapDeconAttrib,examPush);
    exambut_ = new uiPushButton( this, "&Examine", cbexam, true);
    exambut_->attach( rightOf, gatefld_ );

    BufferString lagstr = "Lag size ";
    lagstr += SI().getZUnit();
    lagfld_ = new uiGenInput( this, lagstr, IntInpSpec() );
    lagfld_->attach( alignedBelow, gatefld_ );
    
    BufferString gapstr = "Gap size ";
    gapstr += SI().getZUnit();
    gapfld_ = new uiGenInput( this, gapstr, IntInpSpec() );
    gapfld_->attach( alignedBelow, lagfld_ );
    
    noiselvlfld_ = new uiGenInput( this, "random noise added", IntInpSpec() );
    noiselvlfld_->attach( alignedBelow, gapfld_ );
    uiLabel* percentlbl = new uiLabel( this, "%" );
    percentlbl->attach( rightOf, noiselvlfld_ );
    
    nrtrcsfld_ = new uiLabeledSpinBox( this, "nr traces mixed" );
    nrtrcsfld_->box()->setMinValue( 1 );
    nrtrcsfld_->box()->setStep( 2, true );
    nrtrcsfld_->attach( alignedBelow, noiselvlfld_ );
    uiLabel* stepoutlbl = new uiLabel( this, "( Smoothing parameter )" );
    stepoutlbl->attach( rightOf, nrtrcsfld_ );
    
    isinpzerophasefld_ = new uiGenInput( this, "Input is", 
				 BoolInpSpec("Zero phase", "Minimum phase") );
    isinpzerophasefld_->attach( alignedBelow, nrtrcsfld_ );
    
    isoutzerophasefld_ = new uiGenInput( this, "Output is", 
				 BoolInpSpec("Zero phase", "Minimum phase") );
    isoutzerophasefld_->attach( alignedBelow, isinpzerophasefld_ );
    
    setHAlignObj( gatefld_ );
}


uiGapDeconAttrib::~uiGapDeconAttrib()
{
    delete acorrview_;
}

    
const char* uiGapDeconAttrib::getAttribName() const
{ return GapDecon::attribName(); }


void uiGapDeconAttrib::set2D( bool yn )
{
    exambut_->display(!yn);
}


bool uiGapDeconAttrib::setParameters( const Attrib::Desc& desc )
{
    if ( strcmp(desc.attribName(),GapDecon::attribName()) )
	return false;

    mIfGetFloatInterval( GapDecon::gateStr(), gate, gatefld_->setValue(gate) )
    mIfGetInt( GapDecon::lagsizeStr(), lagsz, lagfld_->setValue(lagsz) )
    mIfGetInt( GapDecon::gapsizeStr(), gapsz, gapfld_->setValue(gapsz) )
    mIfGetInt( GapDecon::nrtrcsStr(), nrtmixed, nrtrcsfld_->box()->
	    						setValue(nrtmixed) )
    mIfGetInt( GapDecon::noiselevelStr(), nlvl, noiselvlfld_->setValue(nlvl) )
    mIfGetBool( GapDecon::isinp0phaseStr(), isinp0ph, 
	    	isinpzerophasefld_->setValue(isinp0ph) )
    mIfGetBool( GapDecon::isout0phaseStr(), isout0ph, 
	    	isoutzerophasefld_->setValue(isout0ph) )
    return true;
}


bool uiGapDeconAttrib::setInput( const Attrib::Desc& desc )
{
    bool isinp0ph = isinpzerophasefld_->getBoolValue();
    int nrtrcsmixed = nrtrcsfld_->box()->getValue();

    if ( nrtrcsmixed == 1 && !isinp0ph )
    {
	putInp( inpfld_, desc, 0 );
	return true;
    }
    const Desc* neededdesc = desc.getInput(0);
    if ( isinp0ph && neededdesc )
	neededdesc = neededdesc->getInput(0);
    if ( nrtrcsmixed != 1 && neededdesc )
	neededdesc = neededdesc->getInput(0);

    if ( !neededdesc )
	inpfld_->setDescSet( desc.descSet() );
    else
    {
	inpfld_->setDesc( neededdesc );
//	inpfld_->updateHistory( adsman_->inputHistory() );
    }

    inpfld_->setIgnoreDesc(&desc);
    return true;
}


bool uiGapDeconAttrib::getParameters( Attrib::Desc& desc )
{
    if ( strcmp(desc.attribName(),GapDecon::attribName()) )
	return false;

    mSetFloatInterval( GapDecon::gateStr(), gatefld_->getFInterval() );
    mSetInt( GapDecon::lagsizeStr(), lagfld_->getIntValue() );
    mSetInt( GapDecon::gapsizeStr(), gapfld_->getIntValue() );
    mSetInt( GapDecon::nrtrcsStr(), nrtrcsfld_->box()->getValue() );
    mSetInt( GapDecon::noiselevelStr(), noiselvlfld_->getIntValue() );
    mSetBool( GapDecon::isinp0phaseStr(), isinpzerophasefld_->getBoolValue() );
    mSetBool( GapDecon::isout0phaseStr(), isoutzerophasefld_->getBoolValue() );

    return true;
}


bool uiGapDeconAttrib::getInput( Attrib::Desc& desc )
{
    bool isinp0ph = isinpzerophasefld_->getBoolValue();
    bool isout0ph = isoutzerophasefld_->getBoolValue();
    int nrtrcsmixed = nrtrcsfld_->box()->getValue();

    if ( nrtrcsmixed == 1 && !isinp0ph && !isout0ph )
    {
	inpfld_->processInput();
	fillInp( inpfld_, desc, 0 );
	return true;
    }
    else
    {
	DescID inputid = DescID::undef(); //in that case use the input field
	if ( nrtrcsmixed != 1 )
	    inputid = createVolStatsDesc( desc, nrtrcsmixed );
	if ( isinp0ph )
	    createHilbertDesc( desc, inputid );

	if ( !desc.setInput( 0, desc.descSet()->getDesc(inputid)) )
	{
	    errmsg_ += "The suggested attribute for input 0";
	    errmsg_ += " is incompatible with the input (wrong datatype)";
	}
    }

    return true;
}


void uiGapDeconAttrib::examPush( CallBacker* cb )
{
    if ( mIsUdf(gatefld_->getFInterval().start) || 
	 mIsUdf(gatefld_->getFInterval().stop) ||
	 inpfld_->attribID() == DescID::undef() )
    {
	BufferString errmsg = "Please, first, fill in the Input Data and the ";
	errmsg += "Correlation window fields";
	uiMSG().error( errmsg );
	return;
    }
    
    CubeSampling cs;
    inpfld_->getRanges(cs);
    positiondlg_ = new uiGDPositionDlg( this, cs );
    positiondlg_->go();
    if ( positiondlg_->uiResult() == 1 ) 
	positiondlg_->popUpPosDlg();

    if ( positiondlg_->posdlg_->uiResult() == 1 )
    {
	acorrview_->setCubesampling( positiondlg_->getCubeSampling() );
	acorrview_->setInputID( inpfld_->attribID() );
	acorrview_->setCorrWin( gatefld_->getFInterval() );
	acorrview_->setDescSet( ads_ );
	if ( acorrview_->computeAutocorr() )
	    acorrview_->createAndDisplay2DViewer();
    }
}


//TODO see which param we would want to eval
void uiGapDeconAttrib::getEvalParams( TypeSet<EvalParam>& params ) const
{
    params += EvalParam( "noise Level (%)", GapDecon::noiselevelStr() );
}


DescID uiGapDeconAttrib::createVolStatsDesc( Desc& desc, int nrtrcsmixed )
{
    inpfld_->processInput();
    const DescID inpid = inpfld_->attribID();
    DescSet* descset = const_cast<DescSet*>(desc.descSet());
    BinID userbid = descset->is2D() ? BinID(0,nrtrcsmixed)
				    : BinID(nrtrcsmixed,nrtrcsmixed);
    Interval<float> gate(0,0);
    TypeSet<DescID> attribids;
    descset->getIds( attribids );
    for ( int idx=0; idx<attribids.size(); idx++ )
    {
	const Desc* dsc = descset->getDesc( attribids[idx] );
	if ( !passStdCheck( dsc, VolStats::attribName(), 0 , 0 , inpid ) )
	    continue;

	if ( !passVolStatsCheck( dsc, userbid, gate ) )
	    continue;
	
	return attribids[idx];
    }

    Desc* newdesc = createNewDesc( descset, inpid, VolStats::attribName(), 0, 0,
	    			   "_mixingavg" );
    if ( !newdesc )
	return DescID::undef();

    mDynamicCastGet( Attrib::BinIDParam*,bidparam,
		     newdesc->getValParam(VolStats::stepoutStr()) )
    bidparam->setValue( userbid.inl, 0 ); bidparam->setValue( userbid.crl, 1 );
    mDynamicCastGet( Attrib::FloatGateParam*,gateparam,
		     newdesc->getValParam(VolStats::gateStr()) )
    gateparam->setValue( gate );

    return descset->addDesc( newdesc );
}


bool uiGapDeconAttrib::passStdCheck( const Desc* dsc, const char* attribnm,
				     int seloutidx, int inpidx, DescID inpid )
{
    if ( strcmp( dsc->attribName(), attribnm ) )
	return false;

    if ( dsc->selectedOutput() != seloutidx )
	return false;
    
    const Desc* inputdesc = dsc->getInput( inpidx );
    if ( !inputdesc || inputdesc->id() != inpid )
	return false;

    return true;
}


bool uiGapDeconAttrib::passVolStatsCheck( const Desc* dsc, BinID userbid, 
					  Interval<float> gate )
{
    Attrib::ValParam* valpar = const_cast<Attrib::ValParam*>(
	    dsc->getValParam(VolStats::stepoutStr()));
    mDynamicCastGet(Attrib::BinIDParam*,bidpar,valpar);
    if ( bidpar && bidpar->getValue() != userbid )
	return false;
    
    Attrib::ValParam* valpar2 = const_cast<Attrib::ValParam*>(
	    dsc->getValParam(VolStats::gateStr()));
    mDynamicCastGet(Attrib::FloatGateParam*,gatepar,valpar2);
    if ( gatepar && gatepar->getValue() != gate )
	return false;

    return true;
}


Desc* uiGapDeconAttrib::createNewDesc( DescSet* descset, DescID inpid, 
				       const char* attribnm, int seloutidx, 
				       int inpidx, BufferString specref )
{
    Desc* inpdesc = descset->getDesc( inpid );
    Desc* newdesc = PF().createDescCopy( attribnm );
    if ( !newdesc || !inpdesc ) 
	return 0;

    newdesc->selectOutput( seloutidx );
    newdesc->setInput( inpidx, inpdesc );
    newdesc->setHidden( true );
    BufferString usrref = "_"; usrref += inpdesc->userRef(); usrref += specref;
    newdesc->setUserRef( usrref );
    return newdesc;
}


void uiGapDeconAttrib::createHilbertDesc( Desc& desc, DescID& inputid )
{
    //TODO
    if ( inputid == DescID::undef() )
    {
	inpfld_->processInput();
	inputid = inpfld_->attribID();
    }
    
    DescSet* descset = const_cast<DescSet*>(desc.descSet());
    TypeSet<DescID> attribids;
    descset->getIds( attribids );
    for ( int idx=0; idx<attribids.size(); idx++ )
    {
	const Desc* dsc = descset->getDesc( attribids[idx] );
	if ( !passStdCheck( dsc, Hilbert::attribName(), 0 , 0 , inputid ) )
	    continue;

	inputid = attribids[idx];
	return;
    }

    Desc* newdesc = createNewDesc( descset, inputid, Hilbert::attribName(), 0,
	    			   0, "_imag" );
    inputid = newdesc ? descset->addDesc( newdesc ) : DescID::undef();
}


DescID uiGapDeconAttrib::createInvHilbertDesc( Desc& desc )
{
    //TODO
    return DescID(3,0);
}
//-----------------------------------------------------------------------------

uiGDPositionDlg::uiGDPositionDlg( uiParent* p, const CubeSampling& cs )
    : uiDialog( p, uiDialog::Setup("Gap Decon viewer position dialog","") )
    , cs_( cs )
{
    inlcrlfld_ = new uiGenInput( this, "Compute autocorrelation on:",
	    			 BoolInpSpec("Inline","Crossline") );
}


uiGDPositionDlg::~uiGDPositionDlg()
{
    delete posdlg_;
}

    
void uiGDPositionDlg::popUpPosDlg()
{
    CallBack dummycb;
    bool isinl = inlcrlfld_->getValue();
    CubeSampling inputcs = cs_;
    if ( isinl )
    {
	float crlwidth = inputcs.hrg.crlRange().width();
	inputcs.hrg.stop.inl = inputcs.hrg.start.inl;
	inputcs.hrg.stop.crl = 
	    mMIN( inputcs.hrg.start.crl + crlwidth/2 + 100,
		  inputcs.hrg.stop.crl );
	inputcs.hrg.start.crl = 
	    mMAX( inputcs.hrg.start.crl + crlwidth/2 - 100, 
		  inputcs.hrg.start.crl );
    }
    else
    {
	float inlwidth = inputcs.hrg.inlRange().width();
	inputcs.hrg.stop.crl = inputcs.hrg.start.crl;
	inputcs.hrg.stop.inl = 
	    mMIN( inputcs.hrg.start.inl + inlwidth/2 + 100,
		  inputcs.hrg.stop.inl );
	inputcs.hrg.start.inl = 
	    mMAX( inputcs.hrg.start.inl + inlwidth/2 - 100,
		  inputcs.hrg.start.inl );
    }

    if ( inputcs.zrg.nrSteps() > 200 )
	inputcs.zrg.stop = inputcs.zrg.start + inputcs.zrg.width()/4;
    
    posdlg_ = new uiSliceSel( this, inputcs, cs_, dummycb, 
			      isinl ? uiSliceSel::Inl : uiSliceSel::Crl );
    posdlg_->disableApplyButton();
    posdlg_->disableScrollButton();
    posdlg_->setModal( true );
    posdlg_->go();
}


const CubeSampling& uiGDPositionDlg::getCubeSampling()
{
    return posdlg_->getCubeSampling();
}
