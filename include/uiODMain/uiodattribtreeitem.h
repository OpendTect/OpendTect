#ifndef uiodattribtreeitem_h
#define uiodattribtreeitem_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: uiodattribtreeitem.h,v 1.5 2009-05-20 14:56:06 cvshelene Exp $
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
	    			      int sceneid,bool ismulticomp=false);
    static bool		handleSelMenu(int mnuid,int visid,int attrib);
    static const char*	sKeySelAttribMenuTxt();
    static const char*	sKeyMultCompMenuTxt();
    static const char*	sKeyColSettingsMenuTxt();
protected:

    bool		anyButtonClick(uiListViewItem*);

    void		createMenuCB( CallBacker* );
    void		handleMenuCB( CallBacker* );
    void		updateColumnText( int col );
    BufferString	createDisplayName() const;
    bool		handleMultCompSelMenu(int,int,int) const;
    
    MenuItem		selattrmnuitem_;
    MenuItem		multcompmnuitem_;
    MenuItem		colsettingsmnuitem_;
};


#endif
