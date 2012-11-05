#ifndef listcommands_h
#define listcommands_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Jaap Glas
 Date:          February 2009
 RCS:           $Id$
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

mClass(uiCmdDriver) ComboActivator: public Activator
{
public:
    			ComboActivator(const uiComboBox&,int itmidx);
    void		actCB(CallBacker*);
protected:
    uiComboBox&		actcombox_;
    int			actitmidx_;
};


mStartDeclCmdClass( uiCmdDriver, ListButton, UiObjectCmd )	mEndDeclCmdClass
mStartDeclCmdClass( uiCmdDriver, ListClick, UiObjectCmd )	mEndDeclCmdClass
mStartDeclCmdClass( uiCmdDriver, ListMenu, UiObjectCmd )	mEndDeclCmdClass

mClass(uiCmdDriver) ListActivator: public Activator
{
public:
    			ListActivator(const uiListBox&,int itmidx,
				      const BufferStringSet& clicktags);
    void		actCB(CallBacker*);
protected:
    uiListBox&		actlist_;
    int			actitmidx_;

    BufferStringSet	actclicktags_;
};

mStartDeclCmdClass( uiCmdDriver, ListSelect, UiObjectCmd )	mEndDeclCmdClass

mClass(uiCmdDriver) ListSelectActivator: public Activator
{
public:
    			ListSelectActivator(const uiListBox&,
					    const TypeSet<int>& selset);
    void		actCB(CallBacker*);
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

mStartDeclComposerClassWithInit( uiCmdDriver, List, CmdComposer, uiListBox )
public:
    virtual void	updateInternalState();

protected:
    void		reInit();
    void		storeListState();
    void		labelStoredStateOld();
    void		labelStoredStateNew();

    void		writeListSelect();
    int 		writeListSelect(bool differential,bool virtually=false);
    void		writeListSelect(int firstidx,int lastidx,
					int blockstate,bool clear);
    void		writeListButton();
    void		writeListMenu(const CmdRecEvent&);
    void		writeListClick();

    int			stagenr_;
    int			clickedidx_;
    bool		leftclicked_;
    bool		ctrlclicked_;
    bool		selchanged_;
    bool		listcmdsflushed_;

    TypeSet<int>	selecteditems_; 
    TypeSet<int>	checkeditems_;
    TypeSet<int>	wasselecteditems_; 
    TypeSet<int>	wascheckeditems_;
    TypeSet<int>	isselecteditems_;
    TypeSet<int>	ischeckeditems_; 

mEndDeclComposerClass

}; // namespace CmdDrive

#endif

