#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		October 2007
________________________________________________________________________

-*/

#include "uiodmainmod.h"
#include "uioddatatreeitem.h"
#include "dbkey.h"

namespace VolProc
{

mExpClass(uiODMain) uiDataTreeItem : public uiODDataTreeItem
{ mODTextTranslationClass(uiDataTreeItem);
public:
   static void			initClass();
				uiDataTreeItem(const char* parenttype,
					       const DBKey* setupmid=0);
				~uiDataTreeItem();

    bool			selectSetup();

    static const char*		sKeyVolumeProcessing()
				{ return "VolumeProcessing"; }

protected:

    bool			anyButtonClick(uiTreeViewItem*);

    static uiODDataTreeItem*	create(const Attrib::SelSpec&,const char*);
    void			createMenu(MenuHandler* menu ,bool istoolbar);
    void			handleMenuCB(CallBacker*);
    uiString			createDisplayName() const;
    void			updateColumnText(int col);

    MenuItem			selmenuitem_;
    MenuItem			reloadmenuitem_;
    MenuItem			editmenuitem_;

    DBKey			mid_;

};

} // namespace VolProc
