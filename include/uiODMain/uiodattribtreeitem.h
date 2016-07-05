#ifndef uiodattribtreeitem_h
#define uiodattribtreeitem_h

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
{ mODTextTranslationClass(mODTextTranslationClass);
public:
    			uiODAttribTreeItem( const char* parenttype );
			~uiODAttribTreeItem();

    static uiString	createDisplayName( int visid, int attrib );
    static void		createSelMenu(MenuItem&,int visid,int attrib,
	    			      int sceneid);
    static bool		handleSelMenu(int mnuid,int visid,int attrib);
    static uiString	sKeySelAttribMenuTxt();
    static uiString	sKeyColSettingsMenuTxt();

protected:

    bool		anyButtonClick(uiTreeViewItem*);

    void		createMenu(MenuHandler*,bool istb);
    void		handleMenuCB(CallBacker*);
    void		updateColumnText(int col);
    uiString		createDisplayName() const;

    MenuItem		selattrmnuitem_;
    MenuItem		colsettingsmnuitem_;
};

#endif
