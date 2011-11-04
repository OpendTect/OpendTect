#ifndef uiodvolproctreeitem_h
#define uiodvolproctreeitem_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		October 2007
 RCS:		$Id: uiodvolproctreeitem.h,v 1.3 2011-11-04 08:22:04 cvskris Exp $
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

    static const char*	sKeyVolumeProcessing() { return "VolumeProcessing"; }
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
