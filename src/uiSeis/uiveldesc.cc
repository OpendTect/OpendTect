/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          September 2007
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uiveldesc.h"

#include "ctxtioobj.h"
#include "ioobjtags.h"
#include "seisselection.h"
#include "separstr.h"
#include "survinfo.h"
#include "timedepthconv.h"
#include "unitofmeasure.h"
#include "velocitycalc.h"
#include "zdomain.h"

#include "uibutton.h"
#include "uizrangeinput.h"
#include "uicombobox.h"
#include "uigeninput.h"
#include "uistrings.h"
#include "uimsg.h"
#include "uistaticsdesc.h"
#include "uitaskrunner.h"
#include "od_helpids.h"


static const char* sKeyDefVelCube = "Default.Cube.Velocity";

uiVelocityDesc::uiVelocityDesc( uiParent* p, const uiVelocityDesc::Setup* vsu )
    : uiGroup( p, "Velocity type selector" )
{
    typefld_ = new uiGenInput( this, tr("Velocity type"),
			StringListInpSpec(VelocityDesc::TypeNames()) );
    typefld_->valuechanged.notify( mCB(this,uiVelocityDesc,updateFlds) );

    uiGroup* vigrp = new uiGroup( this, "Vel info grp" );
    hasstaticsfld_ = new uiGenInput( vigrp, tr("Has statics"),
				     BoolInpSpec(true) );
    hasstaticsfld_->valuechanged.notify(mCB(this,uiVelocityDesc,updateFlds));
    staticsfld_ = new uiStaticsDesc( vigrp, 0 );
    staticsfld_->attach( alignedBelow, hasstaticsfld_ );
    vigrp->setHAlignObj( hasstaticsfld_ );
    vigrp->attach( alignedBelow, typefld_ );

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

    uiString errmsg = tr("Cannot write velocity information");

    if ( desc.type_ != VelocityDesc::Unknown )
    {
	if ( !SetVelocityTag( ioobj, desc ) )
	{
	    if ( disperr ) uiMSG().error( errmsg );
	    return false;
	}
    }
    else
    {
	if ( !RemoveVelocityTag( ioobj ) )
	{
	    if ( disperr ) uiMSG().error( errmsg );
	    return false;
	}
    }

    return true;
}


uiVelocityDescDlg::uiVelocityDescDlg( uiParent* p, const IOObj* sel,
				      const uiVelocityDesc::Setup* vsu )
    : uiDialog( p, uiDialog::Setup(tr("Specify velocity information"),0,
                                    mODHelpKey(mVelocityDescDlg) ) )
    , toprange_( mUdf(float), mUdf(float ) )
    , bottomrange_( mUdf(float), mUdf(float ) )
{
    uiSeisSel::Setup ssu( Seis::Vol ); ssu.seltxt( tr("Velocity cube") );
    volselfld_ = new uiSeisSel( this, uiSeisSel::ioContext(Seis::Vol,true),
				ssu );
    if ( sel )
	volselfld_->setInput( *sel );

    volselfld_->selectionDone.notify(mCB(this,uiVelocityDescDlg,volSelChange) );

    veldescfld_ = new uiVelocityDesc( this, vsu );
    veldescfld_->attach( alignedBelow, volselfld_ );

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
    if ( ioobj )
    {
	if ( !ioobj->pars().get( VelocityStretcher::sKeyTopVavg(),toprange_ ) ||
	     !ioobj->pars().get( VelocityStretcher::sKeyBotVavg(),bottomrange_))
	{
	    toprange_.start = mUdf(float);
	    toprange_.stop = mUdf(float);
	    bottomrange_.start = mUdf(float);
	    bottomrange_.stop = mUdf(float);
	}


	if ( !GetVelocityTag( *ioobj, oldveldesc_ ) )
	    oldveldesc_.type_ = VelocityDesc::Unknown;
    }

    veldescfld_->set( oldveldesc_ );
}


bool uiVelocityDescDlg::scanAvgVel( const IOObj& ioobj,
				    const VelocityDesc& desc )
{
    VelocityModelScanner scanner( ioobj, desc );
    uiTaskRunner taskrunner( this );
    if ( !TaskRunner::execute( &taskrunner, scanner ) )
	return false;

    toprange_ = scanner.getTopVAvg();
    bottomrange_ = scanner.getBotVAvg();

    toprange_.sort();
    bottomrange_.sort();

    return true;
}


bool uiVelocityDescDlg::acceptOK(CallBacker*)
{
    volselfld_->commitInput();
    PtrMan<IOObj> ioobj = volselfld_->getIOObj( false );
    if ( !ioobj )
    {
	uiMSG().error(tr("Please select a valid volume cube."));
	return false;
    }

    VelocityDesc desc;
    if ( !veldescfld_->get( desc, true ) )
    {
	uiMSG().error(tr("Please provide valid velocity type"));
	return false;
    }

    if ( oldveldesc_==desc )
	return true;

    if ( desc.isVelocity() )
    {
	if ( !scanAvgVel(*ioobj,desc) )
	    return false;

	ioobj->pars().set( VelocityStretcher::sKeyTopVavg(), toprange_ );
	ioobj->pars().set( VelocityStretcher::sKeyBotVavg(), bottomrange_ );
    }
    else
    {
	ioobj->pars().removeWithKey( VelocityStretcher::sKeyTopVavg() );
	ioobj->pars().removeWithKey( VelocityStretcher::sKeyBotVavg() );
    }

    return veldescfld_->updateAndCommit( *ioobj, true );
}


uiVelSel::uiVelSel( uiParent* p, const IOObjContext& ctxt,
		    const uiSeisSel::Setup& setup, bool iseditbutton )
    : uiSeisSel( p, ctxt, setup )
    , velrgchanged( this )
    , editcubebutt_(0)
{
    seissetup_.allowsetsurvdefault_ = true;
    seissetup_.survdefsubsel_ = "Velocity";
    if ( iseditbutton )
    {
	editcubebutt_ = new uiPushButton( this, "",
		mCB(this,uiVelSel,editCB), false );
	editcubebutt_->attach( rightOf, selbut_ );
	selectionDoneCB( 0 );
	selectionDone.notify( mCB(this,uiVelSel,selectionDoneCB) );
    }

    selectionDoneCB( 0 ); //sets the ranges
}


const IOObjContext& uiVelSel::ioContext()
{
    mDefineStaticLocalObject( PtrMan<IOObjContext>, velctxt, = 0 );
    if ( !velctxt )
    {

	IOObjContext* newvelctxt =
		new IOObjContext( uiSeisSel::ioContext(Seis::Vol,true) );
	newvelctxt->toselect.require_.setYN(
		VelocityDesc::sKeyIsVelocity(), true );

	if ( !velctxt.setIfNull(newvelctxt) )
	    delete newvelctxt;
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


void uiVelSel::fillDefault()
{
    workctio_.destroyAll();
    if ( !setup_.filldef_ || !workctio_.ctxt.forread )
        return;

    workctio_.fillDefaultWithKey( sKeyDefVelCube );
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
	editcubebutt_->setText( ioobj(true)
		    ? uiStrings::sEdit(false)
		    : uiStrings::sCreate(false) );
}


uiVelModelZAxisTransform::uiVelModelZAxisTransform( uiParent* p, bool t2d )
    : uiTime2DepthZTransformBase( p, t2d )
    , transform_ ( 0 )
    , rangefld_( 0 )
{
    IOObjContext ctxt = uiVelSel::ioContext();
    ctxt.forread = true;
    uiSeisSel::Setup su( false, false );

    su.seltxt( VelocityDesc::getVelVolumeLabel() );
    velsel_ = new uiVelSel( this, ctxt, su );
    velsel_->velrgchanged.notify(
	    mCB(this,uiVelModelZAxisTransform,setZRangeCB) );
    velsel_->selectionDone.notify(
	    mCB(this,uiVelModelZAxisTransform,setZRangeCB) );

    setHAlignObj( velsel_ );
}


uiVelModelZAxisTransform::~uiVelModelZAxisTransform()
{
    unRefAndZeroPtr( transform_ );
}


ZAxisTransform* uiVelModelZAxisTransform::getSelection()
{
    return transform_;
}


void uiVelModelZAxisTransform::enableTargetSampling()
{
    uiTime2DepthZTransformBase::enableTargetSampling();
    setZRangeCB( 0 );
}


void uiVelModelZAxisTransform::setZRangeCB( CallBacker* )
{
    if ( !rangefld_ )
	return;

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

    rangefld_->setZRange( rg );
}


const char* uiVelModelZAxisTransform::selName() const
{ return selname_.buf(); }


#define mErrRet(s) { uiMSG().error(s); return false; }


bool uiVelModelZAxisTransform::acceptOK()
{
    unRefAndZeroPtr( transform_ );

    const IOObj* ioobj = velsel_->ioobj( false );
    if ( !ioobj )
	return false;

    VelocityDesc desc;
    if ( !GetVelocityTag( *ioobj, desc ) )
	mErrRet(tr("Cannot read velocity information for selected model"));

    BufferString zdomain = ioobj->pars().find( ZDomain::sKey() );
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
	mErrRet(tr("Could not allocate memory"));

    transform_->ref();
    if ( !transform_->setVelData( ioobj->key()  ) || !transform_->isOK() )
    {
	TypeSet<uiString> msgs(1,
			tr("Internal: Could not initialize transform") );
	if ( !transform_->errMsg().isEmpty() )
	    msgs += transform_->errMsg();
	uiMSG().errorWithDetails( msgs );
	return false;
    }

    selname_ = ioobj->name();
    selkey_ = ioobj->key();

    return true;
}


FixedString uiVelModelZAxisTransform::getZDomain() const
{
    return t2d_ ? ZDomain::sKeyDepth() : ZDomain::sKeyTime();
}


void uiTime2Depth::initClass()
{
    uiZAxisTransform::factory().addCreator( createInstance,
		Time2DepthStretcher::sFactoryKeyword(), tr("Velocity volume") );
}


uiZAxisTransform* uiTime2Depth::createInstance( uiParent* p,
				       const char* fromdomain,
					const char* todomain )
{
    if ( fromdomain && fromdomain!=ZDomain::sKeyTime() )
	return 0;

    if ( todomain && todomain!=ZDomain::sKeyDepth() )
	return 0;

    return new uiTime2Depth( p );
}


uiTime2Depth::uiTime2Depth( uiParent* p )
    : uiVelModelZAxisTransform( p, true )
{}


void uiDepth2Time::initClass()
{
    uiZAxisTransform::factory().addCreator( createInstance,
		Depth2TimeStretcher::sFactoryKeyword(), tr("Velocity Model") );
}


uiZAxisTransform* uiDepth2Time::createInstance( uiParent* p,
			const char* fromdomain, const char* todomain )
{
    if ( fromdomain && fromdomain!=ZDomain::sKeyDepth() )
	return 0;

    if ( todomain && todomain!=ZDomain::sKeyTime() )
	return 0;

    return new uiDepth2Time( p );
}


uiDepth2Time::uiDepth2Time( uiParent* p )
    : uiVelModelZAxisTransform( p, false )
{}
