/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          September 2007
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiveldesc.cc,v 1.12 2008-11-25 15:35:26 cvsbert Exp $";

#include "uiveldesc.h"

#include "ctxtioobj.h"
#include "ioman.h"
#include "seistrctr.h"
#include "seisselection.h"
#include "uibutton.h"
#include "uigeninput.h"
#include "uimsg.h"



uiVelocityDesc::uiVelocityDesc( uiParent* p, const VelocityDesc& desc )
    : uiGroup( p, "Velocity type selector" )
{
    typefld_ = new uiGenInput( this, "Velocity type",
	    		       StringListInpSpec( VelocityDesc::TypeNames ) );
    typefld_->valuechanged.notify( mCB(this, uiVelocityDesc, velTypeChange) );

    samplefld_ = new uiGenInput( this, "Sample span",
	    StringListInpSpec( VelocityDesc::SampleSpanNames ) );
    samplefld_->attach( alignedBelow, typefld_ );

    setHAlignObj( typefld_ );
    set( desc );
}


void uiVelocityDesc::velTypeChange( CallBacker* )
{
    samplefld_->display( typefld_->getIntValue() );
}


void uiVelocityDesc::set( const VelocityDesc& desc )
{
    typefld_->setValue( desc.type_ );
    samplefld_->setValue( desc.samplespan_ );
    velTypeChange( 0 );
}


VelocityDesc uiVelocityDesc::get() const
{
    return VelocityDesc( (VelocityDesc::Type) typefld_->getIntValue(),
	    		 (VelocityDesc::SampleSpan) samplefld_->getIntValue());
}


uiVelocityDescDlg::uiVelocityDescDlg( uiParent* p, const IOObj* sel )
    : uiDialog( this, uiDialog::Setup("Edit velocity information",0,mNoHelpID) )
    , ctxt_(*mGetCtxtIOObj(SeisTrc,Seis))
{
    ctxt_.ctxt.forread = true;
    if ( sel ) ctxt_.ioobj = sel->clone();

    volsel_ = new uiSeisSel( this, ctxt_, uiSeisSel::Setup(Seis::Vol) );
    volsel_->selectiondone.notify( mCB(this,uiVelocityDescDlg,volSelChange) );
    veldesc_ = new uiVelocityDesc( this, VelocityDesc() );
    veldesc_->attach( alignedBelow, volsel_ );

    volSelChange( 0 );
}


uiVelocityDescDlg::~uiVelocityDescDlg()
{
    delete ctxt_.ioobj;
    delete &ctxt_;
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

    const VelocityDesc desc = veldesc_->get();

    if ( desc.type_==VelocityDesc::Unknown )
    {
	ctxt_.ioobj->pars().remove( VelocityDesc::sKeyVelocityDesc() );
	ctxt_.ioobj->pars().remove( VelocityDesc::sKeyIsVelocity() );
    }
    else
    {
	veldesc_->get().fillPar( ctxt_.ioobj->pars() );
    }
    
    if ( !IOM().commitChanges(*ctxt_.ioobj) )
    {
	uiMSG().error("Cannot write velocity information");
	return false;
    }

    return true;
}


uiVelSel::uiVelSel(uiParent* p, CtxtIOObj& ctxt, const uiSeisSel::Setup& setup )
    : uiSeisSel( p, ctxt, setup )
{
    editcubebutt_ = new
	uiPushButton( this, ctio.ioobj ? "Edit" : "Add", mCB(this,uiVelSel,editCB), false );
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
    uiVelocityDescDlg dlg( this, ctio.ioobj );
    dlg.go();
}


void uiVelSel::updateInput()
{
    uiSeisSel::updateInput();
    editcubebutt_->setText( ctio.ioobj ? "Edit" : "Add" );
}


