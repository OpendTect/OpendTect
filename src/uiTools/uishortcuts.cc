/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uishortcuts.h"
#include "uishortcutsmgr.h"
#include "uicombobox.h"
#include "uilabel.h"
#include "uispinbox.h"
#include "od_helpids.h"



static const char* sSupportedStates[] =
	{ "----", "Shift", "Control", 0 };

uiShortcutsDlg::uiShortcutsDlg( uiParent* p, const char* selkey )
    : uiDialog(p,Setup(tr("Set up shortcuts"),
		       tr("Select keys used as shortcuts"),
		       mODHelpKey(mShortcutsDlgHelpID)))
    , scl_(*new uiShortcutsList(SCMgr().getList(selkey)))
{
    lblspinboxes_.allowNull();
    uiLabeledComboBox* prevlcbox = 0;
    for ( int idx=0; idx<scl_.names().size(); idx++ )
    {
	const uiKeyDesc* kd = scl_.keyDescs()[idx];
	const uiString& nm = toUiString(scl_.names().get( idx ));

	auto* lcbox = new uiLabeledComboBox( this, sSupportedStates, nm );
	lcbox->box()->setCurrentItem( kd->stateStr() );
	stateboxes_ += lcbox->box();
	if ( prevlcbox )
	    lcbox->attach( alignedBelow, prevlcbox );
	prevlcbox = lcbox;

	auto* box = new uiComboBox( this, uiKeyDesc::sKeyKeyStrs(),
					  BufferString("Keys",idx).buf() );
	box->setCurrentItem( kd->keyStr() );
	keyboxes_ += box;
	box->attach( rightOf, lcbox );

	uiKeyDesc* nonconstkd = const_cast<uiKeyDesc*>(kd);
	mDynamicCastGet( uiExtraIntKeyDesc*, eikd, nonconstkd )
	if ( eikd )
	{
	    auto* lsb = new uiLabeledSpinBox(this,eikd->getLabel());
	    lsb->box()->setMinValue( 1 );
	    lsb->box()->setValue( eikd->getIntValue() );
	    lblspinboxes_ += lsb;
	    lsb->attach( rightOf, box );
	}
	else
	    lblspinboxes_ += 0;
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
	if ( lblspinboxes_[idx] )
	{
	    auto* uieikd = new uiExtraIntKeyDesc( statecb->text(),
	    			keycb->text(),
				lblspinboxes_[idx]->box()->getIntValue() );
	    uieikd->setIntLabel( lblspinboxes_[idx]->label()->text() );
	    scl_.keyDescs() += uieikd;
	}
	else
	    scl_.keyDescs() += new uiKeyDesc( statecb->text(), keycb->text() );
    }

    SCMgr().setList( scl_ );
    return true;
}
