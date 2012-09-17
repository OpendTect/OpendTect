/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          May 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uistaticsdesc.cc,v 1.5 2010/11/10 15:26:43 cvsbert Exp $";

#include "uistaticsdesc.h"

#include "ctxtioobj.h"
#include "emioobjinfo.h"
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



uiStaticsDesc::uiStaticsDesc( uiParent* p, const StaticsDesc* sd )
    : uiGroup( p, "Statics editor" )
{
    IOObjContext ctxt = EMHorizon3DTranslatorGroup::ioContext();    
    ctxt.forread = true;
    horfld_ = new uiIOObjSel( this, ctxt, "Statics elevation" );
    horfld_->selectionDone.notify( mCB(this,uiStaticsDesc,updateFlds));

    useconstantvelfld_ = new uiGenInput( this, "Use constant velocity",
	    BoolInpSpec(true) );
    useconstantvelfld_->valuechanged.notify(
	    mCB(this,uiStaticsDesc,updateFlds));
    useconstantvelfld_->attach( alignedBelow, horfld_ );

    BufferString label = "Statics velocity ";
    label += SI().xyInFeet() ? "[ft/s]" : "[m/s]";
    constantvelfld_ = new uiGenInput( this, label.buf(), FloatInpSpec());
    constantvelfld_->attach( alignedBelow, useconstantvelfld_ );

    horattribfld_ = new uiLabeledComboBox( this, "Velocity attribute" );
    horattribfld_->attach( alignedBelow, useconstantvelfld_ );
    setHAlignObj( horattribfld_ );

    set( sd ? *sd : StaticsDesc() );
}


void uiStaticsDesc::updateFlds( CallBacker* )
{
    EM::IOObjInfo eminfo( horfld_->key(true) );

    BufferStringSet attrnms;
    const bool horizonhasattribs =
	eminfo.isOK() && eminfo.getAttribNames( attrnms );

    horfld_->display( true );
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

	horattribfld_->box()->setEmpty();
	horattribfld_->box()->addItems( attrnms );
    }
}


void uiStaticsDesc::set( const StaticsDesc& desc )
{
    horfld_->setInput( desc.horizon_ );
    useconstantvelfld_->setValue( desc.velattrib_.isEmpty() );
    constantvelfld_->setValue( desc.vel_ );
   
    updateFlds( 0 ); 

    horattribfld_->box()->setText( desc.velattrib_ );
}


bool uiStaticsDesc::get( StaticsDesc& res, bool disperr ) const
{
    const IOObj* ioobj = horfld_->ioobj( !disperr );
    if ( !ioobj ) return false;

    res.horizon_ = ioobj->key();

    if ( useconstantvelfld_->getBoolValue() )
    {
	if ( mIsUdf(constantvelfld_->getfValue() ) )
	{
	    if ( disperr )
		uiMSG().error("Statics Velocity not specified");
	    return false;
	}
	    
	res.vel_ = constantvelfld_->getfValue();
    }
    else
    {
	res.vel_ = mUdf(float);
	res.velattrib_ = horattribfld_->box()->text();
    }

    return true;
}


bool uiStaticsDesc::updateAndCommit( IOObj& ioobj, bool disperr )
{
    StaticsDesc desc;
    if ( !get( desc, disperr ) )
	return false;

    if ( !IOM().commitChanges(ioobj) )
    {
	if ( disperr ) uiMSG().error("Cannot write statics information");
	return false;
    }

    return true;
}


