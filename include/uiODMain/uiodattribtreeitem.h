#ifndef uiodattribtreeitem_h
#define uiodattribtreeitem_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: uiodattribtreeitem.h,v 1.2 2006-05-09 11:00:53 cvsbert Exp $
________________________________________________________________________


-*/

#include "uioddatatreeitem.h"

namespace Attrib { class SelSpec; };


/*! Implementation of uiODDataTreeItem for standard attribute displays. */

class uiODAttribTreeItem : public uiODDataTreeItem
{
public:
    			uiODAttribTreeItem( const char* parenttype );
			~uiODAttribTreeItem();
    static BufferString	createDisplayName( int visid, int attrib );
    static void		createSelMenu(MenuItem&,int visid,int attrib,
	    			      int sceneid);
    static bool		handleSelMenu(int mnuid,int visid,int attrib);
    static const char*	sKeySelAttribMenuTxt();
protected:

    bool		anyButtonClick(uiListViewItem*);

    void		createMenuCB( CallBacker* );
    void		handleMenuCB( CallBacker* );
    void		updateColumnText( int col );
    BufferString	createDisplayName() const;

    MenuItem		selattrmnuitem_;
};


#endif
