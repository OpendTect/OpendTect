#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiodmainmod.h"
#include "uioddatatreeitem.h"
#include "multiid.h"

namespace VolProc
{

mExpClass(uiODMain) uiDataTreeItem : public uiODDataTreeItem
{ mODTextTranslationClass(uiDataTreeItem);
public:
   static void			initClass();
				uiDataTreeItem(const char* parenttype,
					       const MultiID* setupmid=0);
   				~uiDataTreeItem();
				
    bool			selectSetup();

    static const char*		sKeyVolumeProcessing()
				{ return "VolumeProcessing"; }

protected:
    
    bool			anyButtonClick(uiTreeViewItem*) override;
   
    static uiODDataTreeItem*	create(const Attrib::SelSpec&,const char*);
    void			createMenu(MenuHandler* menu,
					   bool istoolbar) override;
    void			handleMenuCB(CallBacker*) override;
    uiString			createDisplayName() const override;
    void			updateColumnText( int col ) override;

    MenuItem			selmenuitem_;
    MenuItem			reloadmenuitem_;
    MenuItem			editmenuitem_;

    MultiID			mid_;

};

} // namespace VolProc
