#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
________________________________________________________________________


-*/

#include "uitoolsmod.h"
#include "uitreeitem.h"
#include "menuhandler.h"
#include "uistring.h"

class uiMenu;
#define mTIUiMsg() gUiMsg(getUiParent())


mExpClass(uiTools) uiODTreeItem : public uiTreeItem
{ mODTextTranslationClass(uiODTreeItem);
public:
			uiODTreeItem(const uiString&);
    virtual bool	anyButtonClick(uiTreeViewItem*);

protected:

    virtual bool	init();
    virtual const char* iconName() const		{ return 0; }

    void		addStandardItems(uiMenu&);
    void		addStandardItems(MenuHandler*);
    void		handleStandardItems(int mnuid);
    void		handleStandardMenuCB(CallBacker*);
    virtual void	removeAllItems(bool showmsg=true);

    MenuItem		showallitems_;
    MenuItem		hideallitems_;
    MenuItem		removeallitems_;
    MenuItem		expandallitems_;
    MenuItem		collapseallitems_;
};

#define cShowAllItems		200
#define cHideAllItems		199
#define cRemoveAllItems		198
#define cExpandAllItems		197
#define cCollapseAllItems	196

