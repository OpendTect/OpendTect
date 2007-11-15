/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          08/08/2000
 RCS:           $Id: uiveldesc.cc,v 1.1 2007-11-15 21:14:46 cvskris Exp $
________________________________________________________________________

-*/

#include "uiveldesc.h"

#include "ctxtioobj.h"
#include "ioman.h"
#include "seistrctr.h"
#include "seistrcsel.h"
#include "uigeninput.h"
#include "uimsg.h"
#include "uiseissel.h"



uiVelocityDesc::uiVelocityDesc( uiParent* p, const VelocityDesc& desc )
    : uiGroup( p, "Velocity type selector" )
{
    typefld_ = new uiGenInput( this, "Velocity type",
	    		       StringListInpSpec( VelocityDesc::TypeNames ) );

    samplefld_ = new uiGenInput( this, "Sample range",
	    StringListInpSpec( VelocityDesc::SampleRangeNames ) );
    samplefld_->attach( alignedBelow, typefld_ );

    setHAlignObj( typefld_ );
    set( desc );
}


void uiVelocityDesc::set( const VelocityDesc& desc )
{
    typefld_->setValue( desc.type_ );
    samplefld_->setValue( desc.samplerange_ );
}


VelocityDesc uiVelocityDesc::get() const
{
    return VelocityDesc( (VelocityDesc::Type) typefld_->getIntValue(),
	    		 (VelocityDesc::SampleRange) samplefld_->getIntValue());
}


uiVelocityDescDlg::uiVelocityDescDlg( uiParent* p )
    : uiDialog( this, uiDialog::Setup("Set velocity information", 0, 0 ) )
    , ctxt_( *new CtxtIOObj( SeisTrcTranslatorGroup::ioContext() ) )
{
    ctxt_.ctxt.forread = true;
    SeisSelSetup setup( false );
    volsel_ = new uiSeisSel( this, ctxt_, setup );
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

    veldesc_->get().fillPar( ctxt_.ioobj->pars() );
    if ( !IOM().commitChanges(*ctxt_.ioobj) )
    {
	uiMSG().error("Cannot write velocity information");
	return false;
    }

    return true;
}
