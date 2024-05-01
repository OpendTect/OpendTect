#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiodmainmod.h"
#include "uioddatatreeitem.h"


/*! Implementation of uiODDataTreeItem for standard attribute displays. */

mExpClass(uiODMain) uiODAttribTreeItem : public uiODDataTreeItem
{ mODTextTranslationClass(mODTextTranslationClass)
public:
			uiODAttribTreeItem(const char* parenttype);

    static uiString	createDisplayName(const VisID&,int attrib);
    static void		createSelMenu(MenuItem&,const VisID&,int attrib,
				      const SceneID&);
    static bool		handleSelMenu(int mnuid,const VisID&,int attrib);
    static uiString	sKeySelAttribMenuTxt();
    static uiString	sKeyColSettingsMenuTxt();
    static uiString	sKeyUseColSettingsMenuTxt();

protected:
			~uiODAttribTreeItem();

    bool		anyButtonClick(uiTreeViewItem*) override;

    void		createMenu(MenuHandler*,bool istb) override;
    void		handleMenuCB( CallBacker* ) override;
    void		updateColumnText( int col ) override;
    uiString		createDisplayName() const override;

    MenuItem		selattrmnuitem_;
    MenuItem		colsettingsmnuitem_;
    MenuItem		usecolsettingsmnuitem_;
};
