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
    const IOObjContext ctxt = mIOObjContext( MuteDef );
    mutedeffld_ = new uiIOObjSel( this, ctxt );

    topfld_ = new uiGenInput( this, tr("Mute type"),
			      BoolInpSpec(true,tr("Outer"),tr("Inner")) );
    topfld_->attach( alignedBelow, mutedeffld_ );

    taperlenfld_ = new uiGenInput( this, tr("Taper length (in samples)"),
	    			   FloatInpSpec() );
    taperlenfld_->attach( alignedBelow, topfld_ );

    mutedeffld_->setInput( processor_->muteDefID() );
    topfld_->setValue( !processor_->isTailMute() );
    taperlenfld_->setValue( processor_->taperLength() );
}


bool uiMute::acceptOK( CallBacker* )
{
    if ( !processor_ ) return true;

    const IOObj* ioobj = mutedeffld_->ioobj();
    if ( !ioobj )
    {
	processor_->setEmptyMute();
	return false;
    }

    processor_->setMuteDefID( ioobj->key() );
    processor_->setTaperLength( taperlenfld_->getfValue() );
    processor_->setTailMute( !topfld_->getBoolValue() );
    return true;
}

} // namespace PreStack
