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


class uiComboBox;
class uiListBox;


namespace CmdDrive
{

class CmdDriver;


mStartDeclCmdClass( Combo, UiObjectCmd )		mEndDeclCmdClass

mClass(CmdDriver) ComboActivator: public Activator
{
public:
    			ComboActivator(const uiComboBox&,int itmidx);
    void		actCB(CallBacker*);
protected:
    uiComboBox&		actcombox_;
    int			actitmidx_;
};


mStartDeclCmdClass( ListButton, UiObjectCmd )		mEndDeclCmdClass
mStartDeclCmdClass( ListClick, UiObjectCmd )		mEndDeclCmdClass
mStartDeclCmdClass( ListMenu, UiObjectCmd )		mEndDeclCmdClass

mClass(CmdDriver) ListActivator: public Activator
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

mStartDeclCmdClass( ListSelect, UiObjectCmd )		mEndDeclCmdClass

mClass(CmdDriver) ListSelectActivator: public Activator
{
public:
    			ListSelectActivator(const uiListBox&,
					    const TypeSet<int>& selset);
    void		actCB(CallBacker*);
protected:
    uiListBox&		actlist_;
    TypeSet<int>	actselset_;
};


mStartDeclCmdClass( GetComboItem, UiObjQuestionCmd )		mEndDeclCmdClass
mStartDeclCmdClass( CurComboItem, UiObjQuestionCmd )		mEndDeclCmdClass
mStartDeclCmdClass( NrComboItems, UiObjQuestionCmd )		mEndDeclCmdClass
mStartDeclCmdClass( IsComboItemOn, UiObjQuestionCmd )		mEndDeclCmdClass

mStartDeclCmdClass( NrListItems, UiObjQuestionCmd )		mEndDeclCmdClass
mStartDeclCmdClass( IsListItemOn, UiObjQuestionCmd )		mEndDeclCmdClass
mStartDeclCmdClass( IsListButtonOn, UiObjQuestionCmd )		mEndDeclCmdClass
mStartDeclCmdClass( CurListItem, UiObjQuestionCmd )		mEndDeclCmdClass
mStartDeclCmdClass( GetListItem, UiObjQuestionCmd )		mEndDeclCmdClass
mStartDeclCmdClass( NrListMenuItems, UiObjQuestionCmd )		mEndDeclCmdClass
mStartDeclCmdClass( IsListMenuItemOn, UiObjQuestionCmd )	mEndDeclCmdClass
mStartDeclCmdClass( GetListMenuItem, UiObjQuestionCmd )		mEndDeclCmdClass


mStartDeclComposerClassWithInit( Combo, CmdComposer )
protected:
    bool		itemedited_;
    BufferString	edittext_;
mEndDeclComposerClass

mStartDeclComposerClassWithInit( List, CmdComposer )
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

