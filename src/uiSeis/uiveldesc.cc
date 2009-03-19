/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          September 2007
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiveldesc.cc,v 1.17 2009-03-19 16:12:28 cvsbert Exp $";

#include "uiveldesc.h"

#include "ctxtioobj.h"
#include "ioman.h"
#include "seistrctr.h"
#include "seisselection.h"
#include "uibutton.h"
#include "uigeninput.h"
#include "uimsg.h"



uiVelocityDesc::uiVelocityDesc( uiParent* p, const uiVelocityDesc::Setup* vsu )
    : uiGroup( p, "Velocity type selector" )
    , samplefld_( 0 )
{
    typefld_ = new uiGenInput( this, "Velocity type",
			StringListInpSpec(VelocityDesc::TypeNames()) );
    typefld_->valuechanged.notify( mCB(this,uiVelocityDesc,velTypeChange) );

    if ( !vsu || vsu->withspan_ )
    {
	samplefld_ = new uiGenInput( this, "Sample span",
			StringListInpSpec(VelocityDesc::SampleSpanNames()) );
	samplefld_->attach( alignedBelow, typefld_ );
    }

    setHAlignObj( typefld_ );
    set( vsu ? vsu->desc_ : VelocityDesc() );
}


void uiVelocityDesc::velTypeChange( CallBacker* )
{
    if ( samplefld_ )
	samplefld_->display( typefld_->getIntValue() );
}


void uiVelocityDesc::set( const VelocityDesc& desc )
{
    typefld_->setValue( desc.type_ );
    if ( samplefld_ ) samplefld_->setValue( desc.samplespan_ );
    velTypeChange( 0 );
}


VelocityDesc uiVelocityDesc::get() const
{
    return VelocityDesc( (VelocityDesc::Type) typefld_->getIntValue(),
	    samplefld_
	    ? (VelocityDesc::SampleSpan) samplefld_->getIntValue()
	    :  VelocityDesc::Centered);
}


bool uiVelocityDesc::updateAndCommit( IOObj& ioobj )
{
    const VelocityDesc desc = get();

    if ( desc.type_ != VelocityDesc::Unknown )
	desc.fillPar( ioobj.pars() );
    else
    {
	ioobj.pars().remove( VelocityDesc::sKeyVelocityDesc() );
	ioobj.pars().remove( VelocityDesc::sKeyIsVelocity() );
    }
    
    if ( !IOM().commitChanges(ioobj) )
    {
	uiMSG().error("Cannot write velocity information");
	return false;
    }

    return true;
}


uiVelocityDescDlg::uiVelocityDescDlg( uiParent* p, const IOObj* sel,
				      const uiVelocityDesc::Setup* vsu )
    : uiDialog( this, uiDialog::Setup("Edit velocity information",0,mNoHelpID) )
    , ctxt_(*uiSeisSel::mkCtxtIOObj(Seis::Vol,true))
{
    if ( sel ) ctxt_.setObj( sel->clone() );

    uiSeisSel::Setup ssu( Seis::Vol ); ssu.seltxt( "Velocity cube" );
    volsel_ = new uiSeisSel( this, ctxt_, ssu );
    volsel_->selectiondone.notify( mCB(this,uiVelocityDescDlg,volSelChange) );
    veldesc_ = new uiVelocityDesc( this, vsu );
    veldesc_->attach( alignedBelow, volsel_ );

    volSelChange( 0 );
}


uiVelocityDescDlg::~uiVelocityDescDlg()
{
    delete ctxt_.ioobj; delete &ctxt_;
}


IOObj* uiVelocityDescDlg::getSelection() const
{
    return ctxt_.ioobj ? ctxt_.ioobj->clone() : 0;
}


void uiVelocityDescDlg::volSelChange(CallBacker*)
{
    volsel_->commitInput( false );
    veldesc_->display( ctxt_.ioobj );
    if ( !ctxt_.ioobj )
	return;

    VelocityDesc vd;
    vd.usePar( ctxt_.ioobj->pars() );
    veldesc_->set( vd );
}


bool uiVelocityDescDlg::acceptOK(CallBacker*)
{
    volsel_->commitInput( false );
    if ( !ctxt_.ioobj )
    {
	uiMSG().error( "Cannot find selected object" );
	return false;
    }

    return veldesc_->updateAndCommit( *ctxt_.ioobj );
}


uiVelSel::uiVelSel(uiParent* p, CtxtIOObj& ctxt, const uiSeisSel::Setup& setup )
    : uiSeisSel( p, ctxt, setup )
{
    editcubebutt_ = new uiPushButton( this, ctio_.ioobj ? "Edit" : "Create",
				      mCB(this,uiVelSel,editCB), false );
    editcubebutt_->attach( rightOf, selbut_ );
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
    uiVelocityDescDlg dlg( this, ctio_.ioobj );
    if ( dlg.go() )
	ctio_.setObj( dlg.getSelection() );

    updateInput();
}


void uiVelSel::updateInput()
{
    uiSeisSel::updateInput();
    editcubebutt_->setText( ctio_.ioobj ? "Edit ..." : "Create ..." );
}
