/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          September 2007
________________________________________________________________________

-*/
static const char* rcsID = "$Id$";

#include "uiveldesc.h"

#include "ctxtioobj.h"
#include "ioman.h"
#include "seisselection.h"
#include "separstr.h"
#include "survinfo.h"
#include "timedepthconv.h"
#include "unitofmeasure.h"
#include "velocitycalc.h"
#include "zdomain.h"

#include "uibutton.h"
#include "uicombobox.h"
#include "uigeninput.h"
#include "uimsg.h"
#include "uistaticsdesc.h"
#include "uitaskrunner.h"


static const char* sKeyDefVelCube = "Default.Cube.Velocity";

uiVelocityDesc::uiVelocityDesc( uiParent* p, const uiVelocityDesc::Setup* vsu )
    : uiGroup( p, "Velocity type selector" )
{
    typefld_ = new uiGenInput( this, "Velocity type",
			StringListInpSpec(VelocityDesc::TypeNames()) );
    typefld_->valuechanged.notify( mCB(this,uiVelocityDesc,updateFlds) );

    uiGroup* vigrp = new uiGroup( this, "Vel info grp" );
    hasstaticsfld_ = new uiGenInput( vigrp, "Has statics", BoolInpSpec(true) );
    hasstaticsfld_->valuechanged.notify(mCB(this,uiVelocityDesc,updateFlds));
    staticsfld_ = new uiStaticsDesc( vigrp, 0 );
    staticsfld_->attach( alignedBelow, hasstaticsfld_ );
    vigrp->setHAlignObj( hasstaticsfld_ );
    vigrp->attach( alignedBelow, typefld_ );

    setdefbox_ = new uiCheckBox( this, "Set as default" );
    setdefbox_->attach( alignedBelow, vigrp );

    setHAlignObj( typefld_ );

    set( vsu ? vsu->desc_ : VelocityDesc() );
}


void uiVelocityDesc::updateFlds( CallBacker* )
{
    VelocityDesc::Type type = (VelocityDesc::Type) typefld_->getIntValue();
    if ( type!=VelocityDesc::RMS )
    {
	hasstaticsfld_->display( false );
	staticsfld_->display( false );
	return;
    }

    hasstaticsfld_->display( true );
    staticsfld_->display( hasstaticsfld_->getBoolValue() );
}


NotifierAccess& uiVelocityDesc::typeChangeNotifier() 
{ return typefld_->valuechanged; }


void uiVelocityDesc::set( const VelocityDesc& desc )
{
    typefld_->setValue( desc.type_ );
    hasstaticsfld_->setValue( !desc.statics_.horizon_.isEmpty() );
    staticsfld_->set( desc.statics_ );
    updateFlds( 0 ); 
}


bool uiVelocityDesc::get( VelocityDesc& res, bool disperr ) const
{
    res.type_ = (VelocityDesc::Type) typefld_->getIntValue();
    if ( res.type_!=VelocityDesc::RMS || !hasstaticsfld_->getBoolValue() )
    {
	res.statics_.horizon_.setEmpty();
	res.statics_.vel_ = mUdf(float);
	res.statics_.velattrib_.setEmpty();
    }
    else
    {
	if ( !staticsfld_->get( res.statics_, disperr ) )
	    return false;
    }

    return true;
}


bool uiVelocityDesc::updateAndCommit( IOObj& ioobj, bool disperr )
{
    VelocityDesc desc;
    if ( !get( desc, disperr ) )
	return false;

    if ( desc.type_ != VelocityDesc::Unknown )
	desc.fillPar( ioobj.pars() );
    else
	desc.removePars( ioobj.pars() );
    
    if ( !IOM().commitChanges(ioobj) )
    {
	if ( disperr ) uiMSG().error("Cannot write velocity information");
	return false;
    }

    if ( setdefbox_->isChecked() )
    {
	SI().getPars().set( sKeyDefVelCube, ioobj.key() );
	SI().savePars();
    }

    return true;
}


uiVelocityDescDlg::uiVelocityDescDlg( uiParent* p, const IOObj* sel,
				      const uiVelocityDesc::Setup* vsu )
    : uiDialog( p, uiDialog::Setup("Specify velocity information",0,"103.6.7") )
{
    uiSeisSel::Setup ssu( Seis::Vol ); ssu.seltxt( "Velocity cube" );
    volselfld_ = new uiSeisSel( this, uiSeisSel::ioContext(Seis::Vol,true),
	    			ssu );
    if ( sel ) volselfld_->setInput( *sel );
    volselfld_->selectionDone.notify(mCB(this,uiVelocityDescDlg,volSelChange) );

    veldescfld_ = new uiVelocityDesc( this, vsu );
    veldescfld_->attach( alignedBelow, volselfld_ );

    BufferString str( "Vavg range at top " );
    str += VelocityDesc::getVelUnit( true );

    topavgvelfld_ =
	new uiGenInput( this, str.buf(), FloatInpIntervalSpec(false) );
    topavgvelfld_->attach( alignedBelow, veldescfld_ );

    scanavgvel_ = new uiPushButton( this, "Scan",
			    mCB(this,uiVelocityDescDlg, scanAvgVelCB ), false );
    scanavgvel_->attach( rightOf, topavgvelfld_ );

    str = "Vavg range at bottom ";
    str += VelocityDesc::getVelUnit( true );

    botavgvelfld_ =
	new uiGenInput( this, str.buf(), FloatInpIntervalSpec(false) );
    botavgvelfld_->attach( alignedBelow, topavgvelfld_ );

    volSelChange( 0 );
}


uiVelocityDescDlg::~uiVelocityDescDlg()
{ }


IOObj* uiVelocityDescDlg::getSelection() const
{
    return volselfld_->getIOObj(true);
}


void uiVelocityDescDlg::volSelChange(CallBacker*)
{
    const IOObj* ioobj = volselfld_->ioobj( true );
    scanavgvel_->setSensitive( ioobj );

    VelocityDesc vd;
    Interval<float> topavgvel = Time2DepthStretcher::getDefaultVAvg();
    Interval<float> botavgvel = Time2DepthStretcher::getDefaultVAvg();

    if ( ioobj )
    {
	vd.usePar( ioobj->pars() );
	ioobj->pars().get( VelocityStretcher::sKeyTopVavg(), topavgvel );
	ioobj->pars().get( VelocityStretcher::sKeyBotVavg(), botavgvel );
    }

    veldescfld_->set( vd );
    topavgvelfld_->setValue( topavgvel );
    botavgvelfld_->setValue( botavgvel );
}


void uiVelocityDescDlg::scanAvgVelCB( CallBacker* )
{
    const IOObj* ioobj = volselfld_->ioobj( true );
    if ( !ioobj )
	return;

    VelocityDesc desc;
    if ( !veldescfld_->get( desc, true ) )
	return;

    VelocityModelScanner scanner( *ioobj, desc );
    uiTaskRunner tr( this );
    if ( tr.execute( scanner ) )
    {
	topavgvelfld_->setValue( scanner.getTopVAvg() );
	botavgvelfld_->setValue( scanner.getBotVAvg() );
    }
}


bool uiVelocityDescDlg::acceptOK(CallBacker*)
{
    volselfld_->commitInput();
    PtrMan<IOObj> ioobj = volselfld_->getIOObj( false );
    if ( !ioobj )
	return false;

    Interval<float> range = topavgvelfld_->getFInterval(0);
    range.sort();
    ioobj->pars().set( VelocityStretcher::sKeyTopVavg(), range );

    range = botavgvelfld_->getFInterval(0);
    range.sort();
    ioobj->pars().set( VelocityStretcher::sKeyBotVavg(), range );

    return veldescfld_->updateAndCommit( *ioobj, true );
}


uiVelSel::uiVelSel( uiParent* p, IOObjContext& ctxt,
		    const uiSeisSel::Setup& setup, bool iseditbutton )
    : uiSeisSel( p, ctxt, setup )
    , velrgchanged( this )
    , editcubebutt_(0)
{
    if ( iseditbutton )
    {
	editcubebutt_ = new uiPushButton( this, "",
		mCB(this,uiVelSel,editCB), false );
	editcubebutt_->attach( rightOf, selbut_ );
	selectionDoneCB( 0 );
	selectionDone.notify( mCB(this,uiVelSel,selectionDoneCB) );
    }

    const char* res = SI().pars().find( sKeyDefVelCube );
    if ( res && *res && IOObj::isKey(res) )
	setInput( MultiID(res) );
    else
	setInput( "" );

    selectionDoneCB( 0 ); //sets the ranges
}


const IOObjContext& uiVelSel::ioContext()
{
    static PtrMan<IOObjContext> velctxt = 0;
    if ( !velctxt )
    {
	velctxt = new IOObjContext( uiSeisSel::ioContext(Seis::Vol,true) );
	velctxt->toselect.require_.setYN( VelocityDesc::sKeyIsVelocity(), true);
    }

    return *velctxt;
}


void uiVelSel::editCB(CallBacker*)
{
    uiVelocityDescDlg dlg( this, workctio_.ioobj );
    if ( dlg.go() )
    {
	PtrMan<IOObj> sel = dlg.getSelection();
	if ( sel )
	    setInput( sel->key() );
    }

    trg_ = dlg.getVelocityTopRange();
    brg_ = dlg.getVelocityBottomRange();
    velrgchanged.trigger();
    selectionDone.trigger();
    
    updateEditButton();
}


void uiVelSel::setInput( const MultiID& mid )
{
    uiIOObjSel::setInput( mid );
    updateEditButton();
}


void uiVelSel::selectionDoneCB( CallBacker* cb )
{
    trg_ = Time2DepthStretcher::getDefaultVAvg();
    brg_ = Time2DepthStretcher::getDefaultVAvg();

    PtrMan<IOObj> ioob = getIOObj( true );
    if ( ioob )
    {
	ioob->pars().get( VelocityStretcher::sKeyTopVavg(), trg_ );
	ioob->pars().get( VelocityStretcher::sKeyBotVavg(), brg_ );
	trg_.sort();
	brg_.sort();
    }

    velrgchanged.trigger();
    updateEditButton();
}


void uiVelSel::updateEditButton()
{
    if ( editcubebutt_ )
	editcubebutt_->setText( ioobj(true) ? "Edit ..." : "Create ..." );
}

uiTimeDepthBase::uiTimeDepthBase( uiParent* p, bool t2d )
    : uiZAxisTransform( p )
    , transform_ ( 0 )
    , t2d_( t2d )
{
    IOObjContext ctxt = uiVelSel::ioContext();
    ctxt.forread = true;
    uiSeisSel::Setup su( false, false ); su.seltxt("Velocity model");
    velsel_ = new uiVelSel( this, ctxt, su );
    velsel_->velrgchanged.notify(
	    mCB(this,uiTimeDepthBase,setZRangeCB) );
    velsel_->selectionDone.notify(
	    mCB(this,uiTimeDepthBase,setZRangeCB) );
	
    BufferString str = t2d ? sKey::Depth.str() : sKey::Time.str();
    str += " range ";
    str += UnitOfMeasure::zUnitAnnot( !t2d, true, true );

    rangefld_ = new uiGenInput(this, str.buf(),
	    		       FloatInpIntervalSpec(true) );
    rangefld_->attach( alignedBelow, velsel_ );

    setHAlignObj( rangefld_ );
    
    setZRangeCB( 0 );
}


uiTimeDepthBase::~uiTimeDepthBase()
{
    if ( transform_ ) transform_->unRef();
}


ZAxisTransform* uiTimeDepthBase::getSelection()
{
    return transform_;
}


StepInterval<float> uiTimeDepthBase::getZRange() const
{
    StepInterval<float> res = rangefld_->getFStepInterval();
    if ( !t2d_ )
    {
	res.start /= 1000;
	res.stop /= 1000;
	res.step /= 1000;
    }

    return res;
} 


void uiTimeDepthBase::setZRangeCB( CallBacker* )
{
    StepInterval<float> rg;
    const StepInterval<float> zrg = SI().zRange(true);
    const Interval<float> topvelrg = velsel_->getVelocityTopRange();
    const Interval<float> botvelrg = velsel_->getVelocityBottomRange();

    if ( t2d_ && SI().zIsTime() )
    {
	rg.start = zrg.start * topvelrg.start / 2;
	rg.stop = zrg.stop * botvelrg.stop / 2;
	rg.step = (rg.stop-rg.start) / zrg.nrSteps();
    }
    else if ( !t2d_ && !SI().zIsTime() )
    {
	rg.start = 2 * zrg.start / topvelrg.stop;
	rg.stop = 2 * zrg.stop / botvelrg.start;
	rg.step = (rg.stop-rg.start) / zrg.nrSteps();
    }
    else
	rg = SI().zRange( true );

    if ( !t2d_ )
	rg.scale( SI().zFactor(true) );

    rangefld_->setValue( rg );
}


const char* uiTimeDepthBase::selName() const
{ return selname_.buf(); }

#define mErrRet(s) { uiMSG().error(s); return false; }


bool uiTimeDepthBase::acceptOK()
{
    if ( transform_ ) transform_->unRef();
    transform_ = 0;

    const IOObj* ioobj = velsel_->ioobj( false );
    if ( !ioobj )
	return false;

    VelocityDesc desc;
    if ( !desc.usePar( ioobj->pars() ) )
	mErrRet("Cannot read velocity information for selected model");

    BufferString zdomain = ioobj->pars().find( ZDomain::sKey() ).str();
    if ( zdomain.isEmpty() )
	zdomain = ZDomain::SI().key();

    FixedString err;
    if ( !TimeDepthConverter::isVelocityDescUseable( desc,
	    zdomain==ZDomain::sKeyTime(), &err ) )
    {
	mErrRet( err.str() )
    }

    if ( t2d_ )
    {
	mTryAlloc( transform_, Time2DepthStretcher() );
    }
    else
    {
	mTryAlloc( transform_, Depth2TimeStretcher() );
    }

    if ( !transform_ )
	mErrRet("Could not allocate memory");

    transform_->ref();
    if ( !transform_->setVelData( ioobj->key()  ) || !transform_->isOK() )
    {
	FileMultiString fms("Internal: Could not initialize transform" );
	fms += transform_->errMsg();
	uiMSG().errorWithDetails( fms );
	return false;
    }

    selname_ = ioobj->name();
    selkey_ = ioobj->key();

    return true;
}


FixedString uiTimeDepthBase::getZDomain() const
{
    return t2d_ ? ZDomain::sKeyDepth() : ZDomain::sKeyTime();
}


void uiTime2Depth::initClass()
{
    uiZAxisTransform::factory().addCreator( create,
		Time2DepthStretcher::sFactoryKeyword(), "Time to depth" );
}


uiZAxisTransform* uiTime2Depth::create( uiParent* p, const char* fromdomain,
					const char* todomain )
{
    if ( fromdomain && fromdomain!=ZDomain::sKeyTime() )
	return 0;

    if ( todomain && todomain!=ZDomain::sKeyDepth() )
	return 0;

    return new uiTime2Depth( p );
}


uiTime2Depth::uiTime2Depth( uiParent* p )
    : uiTimeDepthBase( p, true )
{}


void uiDepth2Time::initClass()
{
    uiZAxisTransform::factory().addCreator( create,
		Depth2TimeStretcher::sFactoryKeyword(), "Depth to Time" );
}


uiZAxisTransform* uiDepth2Time::create( uiParent* p, const char* fromdomain,
       					const char* todomain )
{
    if ( fromdomain && fromdomain!=ZDomain::sKeyDepth() )
	return 0;

    if ( todomain && todomain!=ZDomain::sKeyTime() )
	return 0;

    return new uiDepth2Time( p );
}


uiDepth2Time::uiDepth2Time( uiParent* p )
    : uiTimeDepthBase( p, false )
{}
