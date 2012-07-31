/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        H. Huck
 Date:          July  2006
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id: uigapdeconattrib.cc,v 1.54 2012-07-31 04:20:09 cvskris Exp $";

#include "uigapdeconattrib.h"
#include "uigdexamacorr.h"
#include "gapdeconattrib.h"

#include "attribdesc.h"
#include "attribdescset.h"
#include "attribfactory.h"
#include "attribparam.h"
#include "seistrctr.h"
#include "survinfo.h"
#include "uiattrsel.h"
#include "uiseisioobjinfo.h"
#include "uislicesel.h"
#include "uibutton.h"
#include "uicombobox.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "uispinbox.h"
#include "cubesampling.h"
#include "volstatsattrib.h"
#include "hilbertattrib.h"
#include "uimsg.h"

using namespace Attrib;

const char* uiGapDeconAttrib::sKeyOnInlineYN()	{ return "OnInlineYN"; }
const char* uiGapDeconAttrib::sKeyLineName()	{ return "Line Name"; }

mInitAttribUI(uiGapDeconAttrib,GapDecon,"GapDecon",sKeyFilterGrp())


class uiGDPositionDlg: public uiDialog
{
    public:
			uiGDPositionDlg(uiParent*,const CubeSampling&,bool,
					const MultiID&);
			~uiGDPositionDlg();

    void                popUpPosDlg();
    const CubeSampling&	getCubeSampling();
    LineKey		getLineKey() const;
    void		setPrefCS(CubeSampling* prefcs)	{ prefcs_ = prefcs; }

    uiGenInput*		inlcrlfld_;
    uiLabeledComboBox*	linesfld_;
    CubeSampling	cs_;
    CubeSampling*	prefcs_;
    uiSliceSelDlg*	posdlg_;
    bool 		is2d_; 
    MultiID 		mid_; 
    IOPar		prevpar_;
};


uiGapDeconAttrib::uiGapDeconAttrib( uiParent* p, bool is2d )
	: uiAttrDescEd(p,is2d,"101.0.101")
    	, acorrview_ ( new GapDeconACorrView(p) )
    	, positiondlg_(0)
{
    par_.setEmpty();
    inpfld_ = createInpFld( is2d );

    BufferString gatestr = "Correlation window ";
    gatestr += SI().getZUnitString();
    gatefld_ = new uiGenInput( this, gatestr, FloatInpIntervalSpec() );
    gatefld_->attach( alignedBelow, inpfld_ );

    CallBack cbexam = mCB(this,uiGapDeconAttrib,examPush);
    exambut_ = new uiPushButton( this, "&Examine ...", cbexam, true);
    exambut_->attach( rightOf, gatefld_ );

    BufferString lagstr = "Lag size ";
    lagstr += SI().getZUnitString();
    lagfld_ = new uiGenInput( this, lagstr, IntInpSpec() );
    lagfld_->attach( alignedBelow, gatefld_ );
    
    BufferString gapstr = "Gap size ";
    gapstr += SI().getZUnitString();
    gapfld_ = new uiGenInput( this, gapstr, IntInpSpec() );
    gapfld_->attach( alignedBelow, lagfld_ );
    
    noiselvlfld_ = new uiGenInput( this, "Random noise added", IntInpSpec() );
    noiselvlfld_->attach( alignedBelow, gapfld_ );
    uiLabel* percentlbl = new uiLabel( this, "%" );
    percentlbl->attach( rightOf, noiselvlfld_ );

    wantmixfld_ = new uiGenInput( this, "Use trace averaging",
	    			  BoolInpSpec(true) );
    wantmixfld_->valuechanged.notify( mCB(this,uiGapDeconAttrib,mixSel) );
    wantmixfld_->attach( alignedBelow, noiselvlfld_ );
//    uiLabel* stepoutlbl = new uiLabel( this, "( Smoothing parameter )" );
//    stepoutlbl->attach( rightOf, wantmixfld_ );
    
    stepoutfld_ = new uiLabeledSpinBox( this, "stepout" );
    stepoutfld_->box()->setMinValue( 1 );
    stepoutfld_->box()->setStep( 1, true );
    stepoutfld_->attach( rightOf, wantmixfld_ );

    BoolInpSpec bis( true, "Zero phase", "Minimum phase" );
    isinpzerophasefld_ = new uiGenInput( this, "Input is", bis );
    isinpzerophasefld_->attach( alignedBelow, wantmixfld_ );
    
    isoutzerophasefld_ = new uiGenInput( this, "Output is", bis );
    isoutzerophasefld_->attach( alignedBelow, isinpzerophasefld_ );

    CallBack cbqc = mCB(this,uiGapDeconAttrib,qCPush);
    qcbut_ = new uiPushButton( this, "Check &parameters ...", cbqc, true);
    qcbut_->attach( alignedBelow, isoutzerophasefld_ );
    
    setHAlignObj( gatefld_ );
}


uiGapDeconAttrib::~uiGapDeconAttrib()
{
    delete acorrview_;
}

    
void uiGapDeconAttrib::mixSel( CallBacker* )
{
    stepoutfld_->display( wantmixfld_->getBoolValue() );
}


bool uiGapDeconAttrib::setParameters( const Attrib::Desc& desc )
{
    if ( strcmp(desc.attribName(),GapDecon::attribName()) )
	return false;

    mIfGetFloatInterval( GapDecon::gateStr(), gate, gatefld_->setValue(gate) )
    mIfGetInt( GapDecon::lagsizeStr(), lagsz, lagfld_->setValue(lagsz) )
    mIfGetInt( GapDecon::gapsizeStr(), gapsz, gapfld_->setValue(gapsz) )
    int stout;
    mIfGetInt( GapDecon::stepoutStr(), stepout, 
	       stepoutfld_->box()->setValue(stepout); stout = stepout; )
    wantmixfld_->setValue( stout>0 );
    mIfGetInt( GapDecon::noiselevelStr(), nlvl, noiselvlfld_->setValue(nlvl) )
    mIfGetBool( GapDecon::isinp0phaseStr(), isinp0ph, 
	    	isinpzerophasefld_->setValue(isinp0ph) )
    mIfGetBool( GapDecon::isout0phaseStr(), isout0ph, 
	    	isoutzerophasefld_->setValue(isout0ph) )

    mixSel(0);
    return true;
}


bool uiGapDeconAttrib::setInput( const Attrib::Desc& desc )
{
    bool isinp0ph = isinpzerophasefld_->getBoolValue();
    int stepout = wantmixfld_->getBoolValue() ? stepoutfld_->box()->getValue() 
					      : 0;

    if ( stepout == 0 && !isinp0ph )
    {
	putInp( inpfld_, desc, 0 );
	return true;
    }
    const Desc* neededdesc = desc.getInput(0);
    if ( isinp0ph && neededdesc )
	neededdesc = neededdesc->getInput(0);

    if ( !neededdesc )
	inpfld_->setDescSet( desc.descSet() );
    else
	inpfld_->setDesc( neededdesc );

    return true;
}


bool uiGapDeconAttrib::getParameters( Attrib::Desc& desc )
{
    if ( strcmp(desc.attribName(),GapDecon::attribName()) )
	return false;

    mSetFloatInterval( GapDecon::gateStr(), gatefld_->getFInterval() );
    mSetInt( GapDecon::lagsizeStr(), lagfld_->getIntValue() );
    mSetInt( GapDecon::gapsizeStr(), gapfld_->getIntValue() );
    bool domixing = wantmixfld_->getBoolValue();
    mSetInt( GapDecon::stepoutStr(), 
	     domixing? stepoutfld_->box()->getValue() : 0 );
    mSetInt( GapDecon::noiselevelStr(), noiselvlfld_->getIntValue() );
    mSetBool( GapDecon::isinp0phaseStr(), isinpzerophasefld_->getBoolValue() );
    mSetBool( GapDecon::isout0phaseStr(), isoutzerophasefld_->getBoolValue() );
    mSetBool( GapDecon::onlyacorrStr(), false );

    return true;
}


bool uiGapDeconAttrib::getInput( Attrib::Desc& desc )
{
    bool isinp0ph = isinpzerophasefld_->getBoolValue();
    int stepout = wantmixfld_->getBoolValue() ? 
		  stepoutfld_->box()->getValue() : 0;

    //create first input
    if ( !isinp0ph )
    {
	inpfld_->processInput();
	fillInp( inpfld_, desc, 0 );
    }
    else
    {
	DescID inputid = DescID::undef();
	createHilbertDesc( desc, inputid );
	if ( !desc.setInput( 0, desc.descSet()->getDesc(inputid)) )
	{
	    errmsg_ += "The suggested attribute for input 0";
	    errmsg_ += " is incompatible with the input (wrong datatype)";
	}
    }
    
    if ( stepout > 0 )
    {
	//create mixed input 
	DescID mixedinputid = DescID::undef(); 
	mixedinputid = createVolStatsDesc( desc, stepout );
	if ( isinp0ph )
	    createHilbertDesc( desc, mixedinputid );

	if ( !desc.setInput( 1, desc.descSet()->getDesc(mixedinputid)) )
	{
	    errmsg_ += "The suggested attribute for input 1";
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
    Interval<float> gate = gatefld_->getFInterval();

    MultiID mid;
    getInputMID( mid );
    if ( positiondlg_ ) delete positiondlg_;
    positiondlg_ = new uiGDPositionDlg( this, cs, ads_->is2D(), mid );
    if ( par_.size() )
    {
	if ( ads_->is2D() )
	{
	    BufferString lnm;
	    if ( par_.get( sKeyLineName(), lnm ) )
		positiondlg_->linesfld_->box()->setText( lnm );
	}
	else
	{
	    bool oninlineyn = false;
	    par_.getYN( sKeyOnInlineYN(), oninlineyn );
	    positiondlg_->inlcrlfld_->setValue( oninlineyn );
	}

	if ( par_.size() > 1 )
	    positiondlg_->prevpar_ = par_;
    }

    positiondlg_->go();
    if ( positiondlg_->uiResult() == 1 ) 
    {
	if ( !ads_->is2D() )
	{
	    const bool isoninline = positiondlg_->inlcrlfld_->getBoolValue();
	    bool prevsel = false;
	    par_.getYN( sKeyOnInlineYN(), prevsel );
	    if ( isoninline != prevsel )
	    {
		par_.setEmpty();
		positiondlg_->prevpar_.setEmpty();
	    }

	    par_.setYN( sKeyOnInlineYN(), isoninline );
	}
	else
	{
	    const char* lnm = positiondlg_->linesfld_->box()->text();
	    BufferString prevlnm;
	    par_.get( sKeyLineName(), prevlnm );
	    if ( !prevlnm.isEqual( lnm ) )
	    {
		par_.setEmpty();
		positiondlg_->prevpar_.setEmpty();
	    }

	    par_.set( sKeyLineName(), lnm );
	}

	positiondlg_->popUpPosDlg();
    }

    if ( positiondlg_->posdlg_ && positiondlg_->posdlg_->uiResult() == 1 )
    {
	DescSet* dset = new DescSet( *ads_ );
	DescID inpid = inpfld_->attribID();
	DescID gapdecid = createGapDeconDesc( inpid, inpid, dset, true );
	acorrview_->setAttribID( gapdecid );
	acorrview_->setCubeSampling( positiondlg_->getCubeSampling() );
	if ( dset->is2D() )
	    acorrview_->setLineKey( positiondlg_->getLineKey() );

	acorrview_->setDescSet( dset );
	if ( acorrview_->computeAutocorr(false) )
	    acorrview_->createAndDisplay2DViewer(false);

	positiondlg_->posdlg_->grp()->fillPar( positiondlg_->prevpar_ );
	par_.merge( positiondlg_->prevpar_ );
    }
}


//TODO see which param we would want to eval
void uiGapDeconAttrib::getEvalParams( TypeSet<EvalParam>& params ) const
{
    params += EvalParam( "noise Level (%)", GapDecon::noiselevelStr() );
}


DescID uiGapDeconAttrib::createVolStatsDesc( Desc& desc, int stepout )
{
    inpfld_->processInput();
    const DescID inpid = inpfld_->attribID();
    DescSet* descset = const_cast<DescSet*>(desc.descSet());
    BinID userbid = descset->is2D() ? BinID(0,stepout)
				    : BinID(stepout,stepout);
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
    mDynamicCastGet( Attrib::BoolParam*,steerparam,
		     newdesc->getValParam(VolStats::steeringStr()) )
    steerparam->setValue( false );
    mDynamicCastGet( Attrib::IntParam*,nrtrcsparam,
		     newdesc->getValParam(VolStats::nrtrcsStr()) )
    nrtrcsparam->setValue( 3 );

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


DescID uiGapDeconAttrib::createGapDeconDesc( DescID& inp0id, DescID inp1id,
					     DescSet* dset, bool onlyacorr )
{
    if ( inp0id == DescID::undef() )
    {
	inpfld_->processInput();
	inp0id = inpfld_->attribID();
    }

    Desc* newdesc = createNewDesc( dset, inp0id, GapDecon::attribName(),0,0,"");
    if ( !newdesc )
	return DescID::undef();

    mDynamicCastGet( FloatGateParam*,gateparam,
		     newdesc->getValParam(GapDecon::gateStr()) )
    gateparam->setValue( gatefld_->getFInterval() );

    mDynamicCastGet( BoolParam*,boolparam,
		     newdesc->getValParam(GapDecon::onlyacorrStr()) )
    boolparam->setValue( onlyacorr );

    if ( !onlyacorr )
    {
	fillInGDDescParams( newdesc );
	
	Desc* inp1desc = inp1id != DescID::undef() ? dset->getDesc( inp1id ) :0;
	if ( inp1desc )
	    newdesc->setInput( 1, inp1desc );
    }
    
    newdesc->updateParams();
    newdesc->setUserRef( onlyacorr ? "autocorrelation" : "gapdecon" );
    return dset->addDesc( newdesc );
}

   
void uiGapDeconAttrib::fillInGDDescParams( Desc* newdesc )
{
    mDynamicCastGet( IntParam*,lagparam, 
		     newdesc->getValParam(GapDecon::lagsizeStr()) )
    lagparam->setValue( lagfld_->getIntValue() );
    
    mDynamicCastGet( IntParam*,gapparam, 
		     newdesc->getValParam(GapDecon::gapsizeStr()) )
    gapparam->setValue( gapfld_->getIntValue() );
    
    mDynamicCastGet( IntParam*,noiselvlparam, 
		     newdesc->getValParam(GapDecon::noiselevelStr()) )
    noiselvlparam->setValue( noiselvlfld_->getIntValue() );

    int stepout = wantmixfld_->getBoolValue() ?
		  stepoutfld_->box()->getValue() : 0;
    mDynamicCastGet( IntParam*,stepoutparam, 
		     newdesc->getValParam(GapDecon::stepoutStr()) )
    stepoutparam->setValue( stepout );

    mDynamicCastGet( BoolParam*,outphparam, 
		     newdesc->getValParam(GapDecon::isout0phaseStr()) )
    outphparam->setValue( isoutzerophasefld_->getBoolValue() );
}


void uiGapDeconAttrib::qCPush( CallBacker* cb )
{
    BufferString errmsg;
    if ( mIsUdf(gatefld_->getFInterval().start) || 
	 mIsUdf(gatefld_->getFInterval().stop) )
	errmsg = "Please fill in the 'Correlation window' field";
    else if ( inpfld_->attribID() == DescID::undef() )
	errmsg = "Please fill in the input data";
    else if ( mIsUdf(lagfld_->getIntValue()) )
	errmsg = "Please fill in the 'Lag size' field";
    else if ( mIsUdf(gapfld_->getIntValue()) )
	errmsg = "Please fill in the 'Gap size' field";
   
    if ( errmsg.size() )
    { 
	uiMSG().error( errmsg );
	return;
    }
   
    CubeSampling cs;
    inpfld_->getRanges(cs);
    Interval<float> gate = gatefld_->getFInterval();
    gate.scale(1./SI().zDomain().userFactor());
    if ( cs.zrg.start < gate.start )
	cs.zrg.start = gate.start;
    if ( cs.zrg.stop > gate.stop )
	cs.zrg.stop = gate.stop;

    MultiID mid;
    getInputMID(mid);
    CubeSampling prefcs;
    bool validprefcs = false;
    if ( positiondlg_ )
    {
	prefcs = positiondlg_->getCubeSampling();
	validprefcs = true;
	delete positiondlg_;
    }
    positiondlg_ = new uiGDPositionDlg( this, cs, ads_->is2D(), mid );
    if ( validprefcs ) positiondlg_->setPrefCS(&prefcs);
    positiondlg_->go();
    if ( positiondlg_->uiResult() == 1 ) 
	positiondlg_->popUpPosDlg();

    if ( positiondlg_->posdlg_->uiResult() == 1 )
    {
	DescID inp0id = DescID::undef();
	DescID inp1id = DescID::undef();
	DescSet* dset = new DescSet( *ads_ );
	prepareInputDescs( inp0id, inp1id, dset );
	DescID gapdecid = createGapDeconDesc( inp0id, inp1id, dset, false );
	DescID autocorrid = createGapDeconDesc( gapdecid, inp1id, dset, true );
	acorrview_->setAttribID( autocorrid );
	acorrview_->setCubeSampling( positiondlg_->getCubeSampling() );
	if ( dset->is2D() )
	    acorrview_->setLineKey( positiondlg_->getLineKey() );
	acorrview_->setDescSet( dset );
	if ( acorrview_->computeAutocorr(true) )
	    acorrview_->createAndDisplay2DViewer(true);
    }
}


void uiGapDeconAttrib::prepareInputDescs( DescID& inp0id, DescID& inp1id, 
					  DescSet* dset )
{
    Desc* newdesc = PF().createDescCopy( GapDecon::attribName() );
    if ( !newdesc )
	return;

    newdesc->ref();
    newdesc->setDescSet( dset );
    getInput( *newdesc );

    inp0id = newdesc->inputId(0);
    inp1id = newdesc->inputId(1);

    newdesc->unRef();
}


void uiGapDeconAttrib::getInputMID( MultiID& mid ) const
{
    if ( !ads_->is2D() ) return;
    
    Desc* tmpdesc = ads_->getDesc( inpfld_->attribID() );
    if ( !tmpdesc ) return;
    mid = MultiID( tmpdesc->getStoredID().buf() );
}

//-----------------------------------------------------------------------------

uiGDPositionDlg::uiGDPositionDlg( uiParent* p, const CubeSampling& cs,
				  bool is2d, const MultiID& mid )
    : uiDialog( p, uiDialog::Setup("Gap Decon viewer position",0,mNoHelpID) )
    , cs_( cs )
    , prefcs_(0)
    , is2d_( is2d )
    , mid_( mid )
    , linesfld_(0)
    , inlcrlfld_(0)
    , posdlg_(0)
{
    if ( is2d )
    {
	SeisIOObjInfo objinfo( mid_ );
	BufferStringSet linenames;
	objinfo.getLineNames( linenames );
	linesfld_ = new uiLabeledComboBox( this, 
					   "Compute autocorrelation on line:" );
	for ( int idx=0; idx<linenames.size(); idx++ )
	    linesfld_->box()->addItem( linenames.get(idx) );
    }
    else
	inlcrlfld_ = new uiGenInput( this, "Compute autocorrelation on:",
				    BoolInpSpec(true,"Inline","Crossline") );
    setOkText("Next >>" );
}


uiGDPositionDlg::~uiGDPositionDlg()
{
    delete posdlg_;
}

    
void uiGDPositionDlg::popUpPosDlg()
{
    CallBack dummycb;
    bool is2d = inlcrlfld_ == 0;
    bool isinl = is2d ? false : inlcrlfld_->getBoolValue();
    CubeSampling inputcs = cs_;
    if ( is2d )
    {
	SeisTrcTranslator::getRanges( mid_, inputcs, getLineKey() );
	cs_.hrg.set( inputcs.hrg.inlRange(), inputcs.hrg.crlRange() );
    }

    cs_.zrg.stop = cs_.zrg.width();
    cs_.zrg.start = 0;
    if ( prefcs_ )
	inputcs = *prefcs_;
    else
    {
	if ( !is2d )
	{
	    if ( isinl )
		inputcs.hrg.stop.inl = inputcs.hrg.start.inl
		    		     = inputcs.hrg.inlRange().snappedCenter();
	    else
		inputcs.hrg.stop.crl = inputcs.hrg.start.crl
		    		     = inputcs.hrg.crlRange().snappedCenter();
	}

	inputcs.zrg.start = 0;
    }

    ZDomain::Info info( ZDomain::SI() );
    uiSliceSel::Type tp = is2d ? uiSliceSel::TwoD
			       : (isinl ? uiSliceSel::Inl : uiSliceSel::Crl);
    posdlg_ = new uiSliceSelDlg( this, inputcs, cs_, dummycb, tp, info );
    posdlg_->grp()->enableApplyButton( false );
    posdlg_->grp()->enableScrollButton( false );
    posdlg_->setModal( true );
    if ( !prevpar_.isEmpty() )
	posdlg_->grp()->usePar( prevpar_ );

    posdlg_->go();
}


const CubeSampling& uiGDPositionDlg::getCubeSampling()
{
    return posdlg_->getCubeSampling();
}


LineKey uiGDPositionDlg::getLineKey() const
{
    LineKey lk;
    if ( !linesfld_ ) return lk;
    
    lk.setLineName( linesfld_->box()->text() );
    return lk;
}
