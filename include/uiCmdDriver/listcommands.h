#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uicmddrivermod.h"
#include "command.h"
#include "cmdcomposer.h"

#include "uicombobox.h"
#include "uilistbox.h"


namespace CmdDrive
{

mStartDeclCmdClass( uiCmdDriver, Combo, UiObjectCmd )		mEndDeclCmdClass

mExpClass(uiCmdDriver) ComboActivator: public Activator
{
public:
			ComboActivator(const uiComboBox&,int itmidx);
    void		actCB(CallBacker*) override;
protected:
    uiComboBox&		actcombox_;
    int			actitmidx_;
};


mStartDeclCmdClass( uiCmdDriver, ListButton, UiObjectCmd )	mEndDeclCmdClass
mStartDeclCmdClass( uiCmdDriver, ListClick, UiObjectCmd )	mEndDeclCmdClass
mStartDeclCmdClass( uiCmdDriver, ListMenu, UiObjectCmd )	mEndDeclCmdClass

mExpClass(uiCmdDriver) ListActivator: public Activator
{
public:
			ListActivator(const uiListBox&,int itmidx,
				      const BufferStringSet& clicktags);
    void		actCB(CallBacker*) override;
protected:
    uiListBox&		actlist_;
    int			actitmidx_;

    BufferStringSet	actclicktags_;
};

mStartDeclCmdClass( uiCmdDriver, ListSelect, UiObjectCmd )	mEndDeclCmdClass

mExpClass(uiCmdDriver) ListSelectActivator: public Activator
{
public:
			ListSelectActivator(const uiListBox&,
					    const TypeSet<int>& selset);
    void		actCB(CallBacker*) override;
protected:
    uiListBox&		actlist_;
    TypeSet<int>	actselset_;
};


mStartDeclCmdClass( uiCmdDriver, GetComboItem, UiObjQuestionCmd )
    mEndDeclCmdClass
mStartDeclCmdClass( uiCmdDriver, CurComboItem, UiObjQuestionCmd )
    mEndDeclCmdClass
mStartDeclCmdClass( uiCmdDriver, NrComboItems, UiObjQuestionCmd )
    mEndDeclCmdClass
mStartDeclCmdClass( uiCmdDriver, IsComboItemOn, UiObjQuestionCmd )
    mEndDeclCmdClass

mStartDeclCmdClass( uiCmdDriver, NrListItems, UiObjQuestionCmd )
    mEndDeclCmdClass
mStartDeclCmdClass( uiCmdDriver, IsListItemOn, UiObjQuestionCmd )
    mEndDeclCmdClass
mStartDeclCmdClass( uiCmdDriver, IsListButtonOn, UiObjQuestionCmd )
    mEndDeclCmdClass
mStartDeclCmdClass( uiCmdDriver, CurListItem, UiObjQuestionCmd )
    mEndDeclCmdClass
mStartDeclCmdClass( uiCmdDriver, GetListItem, UiObjQuestionCmd )
    mEndDeclCmdClass
mStartDeclCmdClass( uiCmdDriver, NrListMenuItems, UiObjQuestionCmd )
    mEndDeclCmdClass
mStartDeclCmdClass( uiCmdDriver, IsListMenuItemOn, UiObjQuestionCmd )
    mEndDeclCmdClass
mStartDeclCmdClass( uiCmdDriver, GetListMenuItem, UiObjQuestionCmd )
    mEndDeclCmdClass


mStartDeclComposerClassWithInit( uiCmdDriver, Combo, CmdComposer, uiComboBox )
protected:
    bool		itemedited_;
    BufferString	edittext_;
mEndDeclComposerClass

mStartDeclComposerClassWithInit( uiCmdDriver, List, CmdComposer, uiListBoxObj )
public:
    void		updateInternalState() override;

protected:
    void		reInit();
    void		storeListState();
    void		labelStoredStateOld();
    void		labelStoredStateNew();

    void		writeListSelect();
    int			writeListSelect(bool differential,bool virtually=false);
    void		writeListSelect(int firstidx,int lastidx,
					int blockstate,bool clear);
    void		writeListButton();
    void		writeListMenu(const CmdRecEvent&);
    void		writeListClick();

    int			stagenr_;
    int			clickedidx_;
    bool		leftclicked_;
    bool		selchanged_;
    bool		listcmdsflushed_;

    TypeSet<int>	selecteditems_;
    TypeSet<int>	checkeditems_;
    TypeSet<int>	wascheckeditems_;
    TypeSet<int>	isselecteditems_;
    TypeSet<int>	ischeckeditems_;

mEndDeclComposerClass

} // namespace CmdDrive
