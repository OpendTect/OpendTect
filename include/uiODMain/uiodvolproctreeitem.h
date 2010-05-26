#ifndef uiodvolproctreeitem_h
#define uiodvolproctreeitem_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		October 2007
 RCS:		$Id: uiodvolproctreeitem.h,v 1.1 2010-05-26 20:41:55 cvskris Exp $
________________________________________________________________________

-*/

#include "uioddatatreeitem.h"
#include "multiid.h"

namespace VolProc
{

class uiDataTreeItem : public uiODDataTreeItem
{
public:
   static void			initClass();
   				~uiDataTreeItem();
				
				uiDataTreeItem(const char* parenttype);

    static const char*	sKeyVolumeProcessing() { return "VolumeProcessing"; }
protected:
    
    bool			anyButtonClick(uiListViewItem*);
   
    static uiODDataTreeItem*	create(const Attrib::SelSpec&,const char*);
    void			createMenuCB(CallBacker*);
    void			handleMenuCB(CallBacker*);
    BufferString		createDisplayName() const;
    void			updateColumnText( int col );

    MenuItem			selmenuitem_;
    MenuItem			reloadmenuitem_;

    MultiID			mid_;

};

}; //namespace

#endif
