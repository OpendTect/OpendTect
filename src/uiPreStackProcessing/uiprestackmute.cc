/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : April 2005
-*/


#include "uiprestackmute.h"

#include "uiprestackprocessor.h"
#include "prestackmute.h"
#include "prestackmutedeftransl.h"
#include "uiioobjsel.h"
#include "uigeninput.h"
#include "uimsg.h"
#include "od_helpids.h"

namespace PreStack
{

void uiMute::initClass()
{
    uiPSPD().addCreator( create, Mute::sFactoryKeyword() );
}


uiDialog* uiMute::create( uiParent* p, Processor* proc )
{
    mDynamicCastGet(Mute*,mute,proc);
    if ( !mute ) return 0;

    return new uiMute( p, mute );
}


uiMute::uiMute( uiParent* p, Mute* mute )
    : uiDialog(p,uiDialog::Setup(tr("Mute setup"),mNoDlgTitle,
				 mODHelpKey(mPreStackMuteHelpID)))
    , processor_(mute)
{
    uiIOObjSel::Setup mutesu( tr("Mute Definition") );
    mutesu.optional(true);

    const IOObjContext ctxt = mIOObjContext( MuteDef );
    mutedeffld_ = new uiIOObjSel( this, ctxt, mutesu );
    mutedeffld_->setChecked( !processor_->muteDefID().isUdf() );

    topfld_ = new uiGenInput( this, tr("Mute type"),
			      BoolInpSpec(true,tr("Outer"),tr("Inner")) );
    topfld_->attach( alignedBelow, mutedeffld_ );

    taperlenfld_ = new uiGenInput( this, tr("Taper length (in samples)"),
	    			   FloatInpSpec() );
    taperlenfld_->attach( alignedBelow, topfld_ );

    if( mutedeffld_->isChecked() )
	mutedeffld_->setInput( processor_->muteDefID() );
    else
	mutedeffld_->setInputText( 0 );
    topfld_->setValue( !processor_->isTailMute() );
    taperlenfld_->setValue( processor_->taperLength() );
}


bool uiMute::acceptOK( CallBacker* )
{
    if ( !processor_ ) return true;

    const IOObj* ioobj = mutedeffld_->isChecked()  ? mutedeffld_->ioobj() : 0;

    if ( ioobj )
	processor_->setMuteDefID( ioobj->key() );
    else
    {
	processor_->setEmptyMute();
	if ( mutedeffld_->isChecked() )
	{
	    uiMSG().error(tr("Mute Definition field is empty. "
		"Please provide the mute definition or uncheck the checkbox."));
	    return false;
	}
    }

    processor_->setTaperLength( taperlenfld_->getFValue() );
    processor_->setTailMute( !topfld_->getBoolValue() );
    return true;
}

} // namespace PreStack
