#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
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

    bool		anyButtonClick(uiTreeViewItem*);

    void		createMenu(MenuHandler*,bool istb);
    void		handleMenuCB( CallBacker* );
    void		updateColumnText( int col );
    uiString		createDisplayName() const;

    MenuItem		selattrmnuitem_;
    MenuItem		colsettingsmnuitem_;
    MenuItem		usecolsettingsmnuitem_;
};
