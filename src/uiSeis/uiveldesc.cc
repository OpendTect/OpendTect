/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          September 2007
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiveldesc.cc,v 1.23 2009-05-05 20:16:39 cvskris Exp $";

#include "uiveldesc.h"

#include "ctxtioobj.h"
#include "emmanager.h"
#include "emsurfacetr.h"
#include "emsurfaceiodata.h"
#include "ioman.h"
#include "seistrctr.h"
#include "seisselection.h"
#include "survinfo.h"
#include "uibutton.h"
#include "uigeninput.h"
#include "uicombobox.h"
#include "uimsg.h"



uiVelocityDesc::uiVelocityDesc( uiParent* p, const uiVelocityDesc::Setup* vsu )
    : uiGroup( p, "Velocity type selector" )
{
    typefld_ = new uiGenInput( this, "Velocity type",
			StringListInpSpec(VelocityDesc::TypeNames()) );
    typefld_->valuechanged.notify( mCB(this,uiVelocityDesc,updateFlds) );

    hasstaticsfld_ = new uiGenInput( this, "Has statics", BoolInpSpec(true) );
    hasstaticsfld_->valuechanged.notify(mCB(this,uiVelocityDesc,updateFlds));
    hasstaticsfld_->attach( alignedBelow, typefld_ );

    IOObjContext ctxt = EMHorizon3DTranslatorGroup::ioContext();    
    ctxt.forread = true;
    staticshorfld_ = new uiIOObjSel( this, ctxt, "Statics elevation" );
    staticshorfld_->attach( alignedBelow, hasstaticsfld_ );
    staticshorfld_->selectiondone.notify( mCB(this,uiVelocityDesc,updateFlds));

    useconstantvelfld_ = new uiGenInput( this, "Use constant velocity",
	    BoolInpSpec(true) );
    useconstantvelfld_->valuechanged.notify(
	    mCB(this,uiVelocityDesc,updateFlds));
    useconstantvelfld_->attach( alignedBelow, staticshorfld_ );

    BufferString label = "Statics velocity ";
    label += SI().xyInFeet() ? "[ft/s]" : "[m/s]";
    constantvelfld_ = new uiGenInput( this, label.buf(), FloatInpSpec());
    constantvelfld_->attach( alignedBelow, useconstantvelfld_ );

    horattribfld_ = new uiLabeledComboBox( this, "Velocity attribute" );
    horattribfld_->attach( alignedBelow, useconstantvelfld_ );
    setHAlignObj( typefld_ );

    set( vsu ? vsu->desc_ : VelocityDesc() );
}


void uiVelocityDesc::updateFlds( CallBacker* )
{
    VelocityDesc::Type type = (VelocityDesc::Type) typefld_->getIntValue();
    if ( type!=VelocityDesc::RMS )
    {
	hasstaticsfld_->display( false );
	staticshorfld_->display( false );
	useconstantvelfld_->display( false );
	constantvelfld_->display( false );
	horattribfld_->display( false );
	return;
    }

    hasstaticsfld_->display( true );

    if ( !hasstaticsfld_->getBoolValue() )
    {
	staticshorfld_->display( false );
	useconstantvelfld_->display( false );
	constantvelfld_->display( false );
	horattribfld_->display( false );
	return;
    }

    EM::SurfaceIOData sd;
    const FixedString err =
	EM::EMM().getSurfaceData( staticshorfld_->key(true), sd );

    const bool horizonhasattribs = err.isEmpty() && sd.valnames.size();

    staticshorfld_->display( true );
    useconstantvelfld_->display( true );
    useconstantvelfld_->setSensitive( horizonhasattribs );

    if ( !horizonhasattribs )
	useconstantvelfld_->setValue( true );

    if ( useconstantvelfld_->getBoolValue() )
    {
	constantvelfld_->display( true );
	horattribfld_->display( false );
    }
    else
    {
	constantvelfld_->display( false );
	horattribfld_->display( true );

	horattribfld_->box()->empty();
	horattribfld_->box()->addItems( sd.valnames );
    }
}


void uiVelocityDesc::set( const VelocityDesc& desc )
{
    typefld_->setValue( desc.type_ );
    hasstaticsfld_->setValue( !desc.staticshorizon_.isEmpty() );
    staticshorfld_->setInput( desc.staticshorizon_ );
    useconstantvelfld_->setValue( desc.staticsvelattrib_.isEmpty() );
    constantvelfld_->setValue( desc.staticsvel_ );
   
    updateFlds( 0 ); 

    horattribfld_->box()->setText( desc.staticsvelattrib_ );
}


bool uiVelocityDesc::get( VelocityDesc& res, bool disperr ) const
{
    res.type_ = (VelocityDesc::Type) typefld_->getIntValue();
    if ( res.type_!=VelocityDesc::RMS || !hasstaticsfld_->getBoolValue() )
    {
	res.staticshorizon_.setEmpty();
	res.staticsvel_ = mUdf(float);
	res.staticsvelattrib_.setEmpty();
    }
    else
    {
	const IOObj* ioobj = staticshorfld_->ioobj( !disperr );
	if ( !ioobj ) return false;

	res.staticshorizon_ = ioobj->key();

	if ( useconstantvelfld_->getBoolValue() )
	{
	    if ( mIsUdf(constantvelfld_->getfValue() ) )
	    {
		if ( disperr )
		    uiMSG().error("Statics Velocity not specified");
		return false;
	    }
		
	    res.staticsvel_ = constantvelfld_->getfValue();
	}
	else
	{
	    res.staticsvel_ = mUdf(float);
	    res.staticsvelattrib_ = horattribfld_->box()->text();
	}
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

    return true;
}


uiVelocityDescDlg::uiVelocityDescDlg( uiParent* p, const IOObj* sel,
				      const uiVelocityDesc::Setup* vsu )
    : uiDialog( this, uiDialog::Setup("Edit velocity information",0,mNoHelpID) )
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
