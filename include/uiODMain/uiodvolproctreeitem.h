#ifndef uiodvolproctreeitem_h
#define uiodvolproctreeitem_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		October 2007
 RCS:		$Id: uiodvolproctreeitem.h,v 1.4 2012/06/20 19:23:02 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uioddatatreeitem.h"
#include "multiid.h"

namespace VolProc
{

mClass uiDataTreeItem : public uiODDataTreeItem
{
public:
   static void			initClass();
   				~uiDataTreeItem();
				
				uiDataTreeItem(const char* parenttype);

    bool			selectSetup();

    static const char*		sKeyVolumeProcessing()
				{ return "VolumeProcessing"; }

protected:
    
    bool			anyButtonClick(uiListViewItem*);
   
    static uiODDataTreeItem*	create(const Attrib::SelSpec&,const char*);
    void			createMenu(MenuHandler* menu ,bool istoolbar);
    void			handleMenuCB(CallBacker*);
    BufferString		createDisplayName() const;
    void			updateColumnText( int col );

    MenuItem			selmenuitem_;
    MenuItem			reloadmenuitem_;
    MenuItem			editmenuitem_;

    MultiID			mid_;

};

}; //namespace

#endif
