/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Dec 2003
________________________________________________________________________

-*/

#include "uiodusrinteractionsettings.h"

#include "settingsaccess.h"

#include "uicombobox.h"
#include "uigeninput.h"
#include "uigraphicsview.h"
#include "uilabel.h"
#include "uispinbox.h"
#include "uishortcutsmgr.h"
#include "uivirtualkeyboard.h"

#include "ui3dviewer.h"
#include "uiodapplmgr.h"
#include "uiodscenemgr.h"
#include "uivispartserv.h"

static const char* sSupportedStates[] = { "----", "Shift", "Control", 0 };
static const char* sSCListSelKey = "ODScene";


uiKeyboardInteractionSettingsGroup::uiKeyboardInteractionSettingsGroup(
				uiParent* p, Settings& s )
    : uiSettingsGroup( p, s )
    , initialenabvirtualkeyboard_(false)
{
    setts_.getYN( uiVirtualKeyboard::sKeyEnabVirtualKeyboard(),
		  initialenabvirtualkeyboard_ );
    virtualkeyboardfld_ = new uiGenInput( this, tr("Enable Virtual Keyboard"),
				BoolInpSpec(initialenabvirtualkeyboard_) );
    bottomobj_ = virtualkeyboardfld_;

    uiGroup* scgrp = new uiGroup( this, "Shortcut fields" );
    eikdboxes_.setNullAllowed();
    uiLabeledComboBox* prevlcbox = 0;
    const uiShortcutsList& scl = SCMgr().getList( sSCListSelKey );
    uiLabeledComboBox* lcbox = 0;
    for ( int idx=0; idx<scl.names().size(); idx++ )
    {
	const uiKeyDesc* kd = scl.keyDescs()[idx];
	const uiString& nm = toUiString(scl.names().get( idx ));

	uiStringSet states;
	states.add( toUiString("----") )
	      .add( tr("Shift","on keyboard") )
	      .add( tr("Control","on keyboard") );
	lcbox = new uiLabeledComboBox( scgrp, states, nm );
	uiComboBox* statebox = lcbox->box();
	statebox->setCurrentItem( kd->stateStr() );
	stateboxes_ += statebox;
	if ( prevlcbox )
	    lcbox->attach( alignedBelow, prevlcbox );
	prevlcbox = lcbox;

	uiComboBox* kybox = new uiComboBox( scgrp,
				BufferStringSet(uiKeyDesc::sKeyKeyStrs()),
				BufferString("Key ",idx) );
	kybox->setCurrentItem( kd->keyStr() );
	keyboxes_ += kybox;
	kybox->attach( rightOf, lcbox );

	uiKeyDesc* nonconstkd = const_cast<uiKeyDesc*>(kd);
	mDynamicCastGet( uiExtraIntKeyDesc*, eikd, nonconstkd )
	if ( !eikd )
	    eikdboxes_ += 0;
	else
	{
	    uiLabeledSpinBox* lsb = new uiLabeledSpinBox(scgrp,
							 eikd->getLabel());
	    uiSpinBox* eikdbox = lsb->box();
	    eikdbox->setMinValue( 1 );
	    eikdbox->setValue( eikd->getIntValue() );
	    eikdboxes_ += lsb;
	    lsb->attach( rightOf, kybox );
	}
    }

    if ( lcbox )
    {
	scgrp->setHAlignObj( lcbox );
	bottomobj_ = lcbox;
    }
    scgrp->attach( alignedBelow, virtualkeyboardfld_ );
}


void uiKeyboardInteractionSettingsGroup::doCommit( uiRetVal& uirv )
{
    updateSettings( initialenabvirtualkeyboard_,
		    virtualkeyboardfld_->getBoolValue(),
		    uiVirtualKeyboard::sKeyEnabVirtualKeyboard() );

    const uiShortcutsList& oldscl = SCMgr().getList( sSCListSelKey );
    uiShortcutsList scl( oldscl );
    scl.keyDescs().erase();
    for ( int idx=0; idx<stateboxes_.size(); idx++ )
    {
	uiKeyDesc* newkd = 0;
	const int stateidx = stateboxes_[idx]->currentItem();
	const char* statetxt = sSupportedStates[stateidx];
	const char* keytxt = keyboxes_[idx]->text();
	uiLabeledSpinBox* eikdlsb = eikdboxes_[idx];
	if ( !eikdlsb )
	    newkd = new uiKeyDesc( statetxt, keytxt );
	else
	{
	    uiExtraIntKeyDesc* uieikd = new uiExtraIntKeyDesc( statetxt, keytxt,
						eikdlsb->box()->getIntValue() );
	    uieikd->setIntLabel( eikdlsb->label()->text() );
	    newkd = uieikd;
	}

	changed_ = changed_ || !newkd->isEqualTo( *oldscl.keyDescs()[idx] );
	scl.keyDescs() += newkd;
    }

    if ( changed_ )
	SCMgr().setList( scl );
}


uiMouseInteractionSettingsGroup::uiMouseInteractionSettingsGroup( uiParent* p,
								Settings& s )
    : uiSettingsGroup( p, s )
    , keybindingfld_( 0 )
    , wheeldirectionfld_( 0 )
    , initialmousewheelreversal_( false )
    , trackpadzoomspeedfld_( 0 )
    , initialzoomfactor_( 0 )
{
    TypeSet<int> sceneids;
    if ( ODMainWin()->applMgr().visServer() )
	ODMainWin()->applMgr().visServer()->getSceneIds( sceneids );

    const ui3DViewer* viewer = sceneids.size()
	? ODMainWin()->sceneMgr().get3DViewer( sceneids[0] )
	: 0;

    if ( viewer )
    {
	BufferStringSet keyset;
	viewer->getAllKeyBindings( keyset );

	keybindingfld_ = new uiGenInput( this, tr("3D Mouse Controls"),
					 StringListInpSpec( keyset ) );

	setts_.get( ui3DViewer::sKeyBindingSettingsKey(), initialkeybinding_ );
	keybindingfld_->setText( viewer->getCurrentKeyBindings() );

	setts_.getYN( SettingsAccess::sKeyMouseWheelReversal(),
		     initialmousewheelreversal_ );

	wheeldirectionfld_ = new uiGenInput( this,
	    tr("Mouse wheel direction"),
	    BoolInpSpec( !viewer->getReversedMouseWheelDirection(),
			uiStrings::sNormal(),
			uiStrings::sReversed()) );
	wheeldirectionfld_->attach( alignedBelow, keybindingfld_ );
	bottomobj_ = wheeldirectionfld_;

#ifdef __mac__

	initialzoomfactor_ = viewer->getMouseWheelZoomFactor();
	const bool istrackpad =
	  fabs(initialzoomfactor_-MouseEvent::getDefaultTrackpadZoomFactor())<
	  fabs(initialzoomfactor_-MouseEvent::getDefaultMouseWheelZoomFactor());

	trackpadzoomspeedfld_ = new uiGenInput( this,
	       tr("Optimize zoom speed for"),
	       BoolInpSpec( istrackpad, uiStrings::sTrackPad(),
					uiStrings::sMouse()) );
	trackpadzoomspeedfld_->attach( alignedBelow, wheeldirectionfld_ );
	bottomobj_ = wheeldirectionfld_;
#endif
    }
}


void uiMouseInteractionSettingsGroup::doCommit( uiRetVal& uirv )
{
    if ( !keybindingfld_ )
	return;

    TypeSet<int> sceneids;
    if ( ODMainWin()->applMgr().visServer() )
	ODMainWin()->applMgr().visServer()->getSceneIds( sceneids );

    const BufferString keybinding = keybindingfld_->text();
    const bool reversedwheel = !wheeldirectionfld_->getBoolValue();


    for ( int idx=0; idx<sceneids.size(); idx++ )
    {
	ui3DViewer* viewer = ODMainWin()->sceneMgr().get3DViewer(sceneids[idx]);
	viewer->setKeyBindings( keybinding );

	viewer->setReversedMouseWheelDirection( reversedwheel );
    }

    const ObjectSet<uiGraphicsViewBase>& allviewers =
					uiGraphicsViewBase::allInstances();

    for ( int idx=0; idx<allviewers.size(); idx++ )
    {
	const_cast<uiGraphicsViewBase*>(allviewers[idx])
		->setMouseWheelReversal( reversedwheel );
    }

    if ( trackpadzoomspeedfld_ )
    {
	const float zoomfactor = trackpadzoomspeedfld_->getBoolValue()
	    ? MouseEvent::getDefaultTrackpadZoomFactor()
	    : MouseEvent::getDefaultMouseWheelZoomFactor();

	for ( int idx=0; idx<sceneids.size(); idx++ )
	{
	    ui3DViewer* viewer =
	    ODMainWin()->sceneMgr().get3DViewer(sceneids[idx]);
	    viewer->setMouseWheelZoomFactor(zoomfactor);
	}
	/*TODO: It was not easy to find a good handling of the mouse events,
	 as it is done in many places, and differently.
	for ( int idx=0; idx<allviewers.size(); idx++ )
	{
	    const_cast<uiGraphicsViewBase*>(allviewers[idx])
		->setMouseWheelZoomFactor( zoomfactor );
	}
	 */

	updateSettings( initialzoomfactor_, zoomfactor,
			SettingsAccess::sKeyMouseWheelZoomFactor() );
    }

    updateSettings( initialkeybinding_, keybinding,
		   ui3DViewer::sKeyBindingSettingsKey() );

    updateSettings( initialmousewheelreversal_, reversedwheel,
		   SettingsAccess::sKeyMouseWheelReversal() );
}

