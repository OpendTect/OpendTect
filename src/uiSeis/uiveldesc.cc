/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          September 2007
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiveldesc.cc,v 1.29 2009-08-27 09:58:39 cvsbert Exp $";

#include "uiveldesc.h"

#include "ctxtioobj.h"
#include "emmanager.h"
#include "emsurfacetr.h"
#include "emsurfaceiodata.h"
#include "ioman.h"
#include "seistrctr.h"
#include "seisselection.h"
#include "separstr.h"
#include "survinfo.h"
#include "timedepthconv.h"
#include "uibutton.h"
#include "uigeninput.h"
#include "uicombobox.h"
#include "uimsg.h"
#include "uistaticsdesc.h"
#include "zdomain.h"

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
    : uiDialog( this, uiDialog::Setup("Edit velocity information",0,"103.6.7") )
{
    IOObjContext ctxt( SeisTrcTranslatorGroup::ioContext() );
    uiSeisSel::fillContext( Seis::Vol, true, ctxt );
    uiSeisSel::Setup ssu( Seis::Vol ); ssu.seltxt( "Velocity cube" );
    volsel_ = new uiSeisSel( this, ctxt, ssu );
    if ( sel ) volsel_->setInput( *sel );
    volsel_->selectiondone.notify( mCB(this,uiVelocityDescDlg,volSelChange) );

    veldesc_ = new uiVelocityDesc( this, vsu );
    veldesc_->attach( alignedBelow, volsel_ );

    volSelChange( 0 );
}


uiVelocityDescDlg::~uiVelocityDescDlg()
{ }


IOObj* uiVelocityDescDlg::getSelection() const
{
    return volsel_->getIOObj(true);
}


void uiVelocityDescDlg::volSelChange(CallBacker*)
{
    const IOObj* ioobj = volsel_->ioobj( true );
    if ( !ioobj )
	return;

    VelocityDesc vd;
    vd.usePar( ioobj->pars() );
    veldesc_->set( vd );
}


bool uiVelocityDescDlg::acceptOK(CallBacker*)
{
    volsel_->commitInput();
    PtrMan<IOObj> ioobj = volsel_->getIOObj( false );
    if ( !ioobj )
	return false;

    return veldesc_->updateAndCommit( *ioobj, true );
}


uiVelSel::uiVelSel( uiParent* p, IOObjContext& ctxt,
		    const uiSeisSel::Setup& setup )
    : uiSeisSel( p, ctxt, setup )
{
    editcubebutt_ = new uiPushButton( this, "",
	    mCB(this,uiVelSel,editCB), false );
    editcubebutt_->attach( rightOf, selbut_ );
    updateEditButton( 0 );
    selectiondone.notify( mCB(this,uiVelSel,updateEditButton) );

    const char* res = SI().pars().find( sKeyDefVelCube );
    if ( res && *res && IOObj::isKey(res) )
	setInput( MultiID(res) );
}


const IOObjContext& uiVelSel::ioContext()
{
    static PtrMan<IOObjContext> velctxt = 0;
    if ( !velctxt )
    {
	velctxt = new IOObjContext( SeisTrcTranslatorGroup::ioContext() );
	velctxt->deftransl = "CBVS";
	velctxt->parconstraints.setYN( VelocityDesc::sKeyIsVelocity(), true );
	velctxt->includeconstraints = true;
	velctxt->allowcnstrsabsent = false;
    }

    return *velctxt;
}


void uiVelSel::editCB(CallBacker*)
{
    uiVelocityDescDlg dlg( this, workctio_.ioobj );
    if ( dlg.go() )
	workctio_.setObj( dlg.getSelection() );

    updateEditButton( 0 );
}


void uiVelSel::setInput( const MultiID& mid )
{
    uiIOObjSel::setInput( mid );
    updateEditButton( 0 );
}


void uiVelSel::updateEditButton(CallBacker*)
{
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
}


uiTimeDepthBase::~uiTimeDepthBase()
{
    if ( transform_ ) transform_->unRef();
}


ZAxisTransform* uiTimeDepthBase::getSelection()
{
    return transform_;
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

    BufferString zdomain = ioobj->pars().find( ZDomain::sKey() ).buf();
    if ( zdomain.isEmpty() )
	zdomain = SI().getZDomainString();

    if ( zdomain==ZDomain::sKeyTWT() )
    {
	if ( desc.type_ != VelocityDesc::Interval &&
	     desc.type_ != VelocityDesc::RMS )
	    mErrRet("Only RMS and Interval allowed for time based models");
    }
    else if ( zdomain==ZDomain::sKeyDepth() )
    {
	if ( desc.type_ != VelocityDesc::Interval )
	    mErrRet("Only Interval velocity allowed for time based models");
    }
    else
    {
	mErrRet( "Velocity model must be in either time or depth");
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
    if ( !transform_->setVelData( ioobj->key() ) || !transform_->isOK() )
    {
	FileMultiString fms("Internal: Could not initialize transform" );
	fms += transform_->errMsg();
	uiMSG().errorWithDetails( fms );
	return false;
    }

    selname_ = ioobj->name();

    return true;
}


FixedString uiTimeDepthBase::getZDomain() const
{
    return t2d_ ? ZDomain::sKeyDepth() : ZDomain::sKeyTWT();
}


void uiTime2Depth::initClass()
{
    uiZAxisTransform::factory().addCreator( create,
		    Time2DepthStretcher::sName(), "Time to depth" );
}


uiZAxisTransform* uiTime2Depth::create( uiParent* p, const char* fromdomain )
{
    if ( fromdomain!=ZDomain::sKeyTWT() )
	return 0;

    return new uiTime2Depth( p );
}


uiTime2Depth::uiTime2Depth( uiParent* p )
    : uiTimeDepthBase( p, true )
{}


void uiDepth2Time::initClass()
{
    uiZAxisTransform::factory().addCreator( create,
		    Depth2TimeStretcher::sName(), "Depth to Time" );
}


uiZAxisTransform* uiDepth2Time::create( uiParent* p, const char* fromdomain )
{
    if ( fromdomain!=ZDomain::sKeyDepth() )
	return 0;

    return new uiDepth2Time( p );
}


uiDepth2Time::uiDepth2Time( uiParent* p )
    : uiTimeDepthBase( p, false )
{}
