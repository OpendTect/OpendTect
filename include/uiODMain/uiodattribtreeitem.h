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
    virtual		~uiODAttribTreeItem();

    static uiString	createDisplayName(VisID visid,int attrib);
    static void		createSelMenu(MenuItem&,VisID visid,int attrib,
				      SceneID sceneid);
    static bool		handleSelMenu(int mnuid,VisID visid,int attrib);
    static uiString	sKeySelAttribMenuTxt();
    static uiString	sKeyColSettingsMenuTxt();
    static uiString	sKeyUseColSettingsMenuTxt();

protected:

    bool		anyButtonClick(uiTreeViewItem*) override;

    void		createMenu(MenuHandler*,bool istb) override;
    void		handleMenuCB( CallBacker* ) override;
    void		updateColumnText( int col ) override;
    uiString		createDisplayName() const override;

    MenuItem		selattrmnuitem_;
    MenuItem		colsettingsmnuitem_;
    MenuItem		usecolsettingsmnuitem_;
};
