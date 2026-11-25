#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uibasemod.h"
#include "keyenum.h"
#include "uigroup.h"

class uiCheckBox;
class uiMenu;
class uiPushButton;
class uiToolButton;

/*!\brief Menu, check-all box, and read/save buttons for a checked list.

  The strip does not own the items. Check all, invert, and show-checked-only
  are notifiers. The host performs them and then calls updateState().
  uiListBox and uiStringTableViewGrp are the hosts.

  Ctrl-A checks all, Ctrl-Z clears all, and Ctrl-M opens the menu.
  handleShortcut() does that when this strip is on screen.

*/


mExpClass(uiBase) uiCheckSelGrp : public uiGroup
{ mODTextTranslationClass(uiCheckSelGrp);
public:
			uiCheckSelGrp(uiParent*,const char* nm="CheckGroup");
			~uiCheckSelGrp();
			mOD_DisableCopy(uiCheckSelGrp)

    void		setPrimaryCheckBox(bool yn);
    bool		allShown() const		{ return allshown_; }

    void		offerReadWriteSelection(const CallBack& readcb,
						const CallBack& writecb);
    void		updateState(int nrchecked,int nritems);
    void		showMenu();
    bool		handleShortcut(OD::KeyboardKey key,bool ctrl);

    Notifier<uiCheckSelGrp>		beforeShowMenu;
    CNotifier<uiCheckSelGrp,bool>	checkAllClicked;
    Notifier<uiCheckSelGrp>		invertSelection;
    CNotifier<uiCheckSelGrp,bool>	showCheckedOnly;

private:

    void		menuCB(CallBacker*);
    void		checkCB(CallBacker*);
    void		retrieveCB(CallBacker*);
    void		saveCB(CallBacker*);
    void		updateButtons();

    uiMenu&		menu_;
    uiPushButton*	menubut_;
    uiCheckBox*		cb_;
    uiToolButton*	openbut_;
    uiToolButton*	savebut_;
    CallBack		opencb_;
    CallBack		savecb_;
    bool		allshown_			= true;
    int			nrchecked_			= 0;
    int			nritems_			= 0;
};
