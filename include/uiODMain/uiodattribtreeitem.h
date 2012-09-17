#ifndef uiodattribtreeitem_h
#define uiodattribtreeitem_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: uiodattribtreeitem.h,v 1.9 2011/11/04 08:22:04 cvskris Exp $
________________________________________________________________________


-*/

#include "uioddatatreeitem.h"

namespace Attrib { class SelSpec; };


/*! Implementation of uiODDataTreeItem for standard attribute displays. */

mClass uiODAttribTreeItem : public uiODDataTreeItem
{
public:
    			uiODAttribTreeItem( const char* parenttype );
			~uiODAttribTreeItem();
    static BufferString	createDisplayName( int visid, int attrib );
    static void		createSelMenu(MenuItem&,int visid,int attrib,
	    			      int sceneid);
    static bool		handleSelMenu(int mnuid,int visid,int attrib);
    static const char*	sKeySelAttribMenuTxt();
    static const char*	sKeyColSettingsMenuTxt();
protected:

    bool		anyButtonClick(uiListViewItem*);

    void		createMenu(MenuHandler*,bool istb);
    void		handleMenuCB( CallBacker* );
    void		updateColumnText( int col );
    BufferString	createDisplayName() const;
    
    MenuItem		selattrmnuitem_;
    MenuItem		colsettingsmnuitem_;
};


#endif
