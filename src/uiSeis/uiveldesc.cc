/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          08/08/2000
 RCS:           $Id: uiveldesc.cc,v 1.7 2007-12-28 21:18:38 cvskris Exp $
________________________________________________________________________

-*/

#include "uiveldesc.h"

#include "ctxtioobj.h"
#include "ioman.h"
#include "seistrctr.h"
#include "seisselection.h"
#include "uigeninput.h"
#include "uimsg.h"
#include "uiseissel.h"



uiVelocityDesc::uiVelocityDesc( uiParent* p, const VelocityDesc& desc )
    : uiGroup( p, "Velocity type selector" )
{
    typefld_ = new uiGenInput( this, "Velocity type",
	    		       StringListInpSpec( VelocityDesc::TypeNames ) );
    typefld_->valuechanged.notify( mCB(this, uiVelocityDesc, velTypeChange) );

    samplefld_ = new uiGenInput( this, "Sample range",
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


uiVelocityDescDlg::uiVelocityDescDlg( uiParent* p )
    : uiDialog( this, uiDialog::Setup("Set velocity information", 0, 0 ) )
    , ctxt_(*mGetCtxtIOObj(SeisTrc,Seis))
{
    ctxt_.ctxt.forread = true;
    volsel_ = new uiSeisSel( this, ctxt_, uiSeisSel::Setup(Seis::Vol) );
    volsel_->selectiondone.notify( mCB(this,uiVelocityDescDlg,volSelChange) );
    veldesc_ = new uiVelocityDesc( this, VelocityDesc() );
    veldesc_->attach( alignedBelow, volsel_ );
}


uiVelocityDescDlg::~uiVelocityDescDlg()
{
    delete ctxt_.ioobj;
    delete &ctxt_;
}


void uiVelocityDescDlg::volSelChange(CallBacker*)
{
    volsel_->commitInput( false );
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
