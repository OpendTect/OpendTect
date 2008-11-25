/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        H. Payraudeau
 Date:          December 2005
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uishortcuts.cc,v 1.13 2008-11-25 15:35:26 cvsbert Exp $";


#include "uishortcuts.h"
#include "uishortcutsmgr.h"
#include "uicombobox.h"


static const char* sSupportedStates[] =
	{ "----", "Shift", "Control", 0 };

uiShortcutsDlg::uiShortcutsDlg( uiParent* p, const char* selkey )
    : uiDialog( p,uiDialog::Setup( "Set up shortcuts",
				   "Select keys used as shortcuts", "0.2.4" ) )
    , scl_(*new uiShortcutsList(SCMgr().getList(selkey)))
{
    uiLabeledComboBox* prevlcbox = 0;
    for ( int idx=0; idx<scl_.names().size(); idx++ )
    {
	const uiKeyDesc& kd = scl_.keyDescs()[idx];
	const BufferString& nm = scl_.names().get( idx );

	uiLabeledComboBox* lcbox
	    	= new uiLabeledComboBox( this, sSupportedStates, nm );
	lcbox->box()->setCurrentItem( kd.stateStr() );
	stateboxes_ += lcbox->box();
	if ( prevlcbox )
	    lcbox->attach( alignedBelow, prevlcbox );
	prevlcbox = lcbox;

	uiComboBox* box = new uiComboBox( this, uiKeyDesc::sKeyKeyStrs(),
					  BufferString("Keys",idx).buf() );
	box->setCurrentItem( kd.keyStr() );
	keyboxes_ += box;
	box->attach( rightOf, lcbox );
    }
}


uiShortcutsDlg::~uiShortcutsDlg()
{
    delete &scl_;
}


bool uiShortcutsDlg::acceptOK( CallBacker* )
{
    scl_.keyDescs().erase();
    for ( int idx=0; idx<stateboxes_.size(); idx++ )
    {
	uiComboBox* statecb = stateboxes_[idx];
	uiComboBox* keycb = keyboxes_[idx];
	scl_.keyDescs() += uiKeyDesc( statecb->text(), keycb->text() );
    }

    SCMgr().setList( scl_ );
    return true;
}
