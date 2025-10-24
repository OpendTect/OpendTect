#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uitutmod.h"

#include "menuhandler.h"
#include "tutexternalattribrandom.h"
#include "uioddatatreeitem.h"


namespace ExternalAttrib
{

/* Class that holds the external attrib's tree-item. Can easily be complemented
   with menu handling by implementing more intelligent createMenu/handleMenu. */

mClass(uiTut) uiRandomTreeItem : public uiODDataTreeItem
{
public:
			uiRandomTreeItem(const char* parenttype);

private:
			~uiRandomTreeItem();

    bool		anyButtonClick(uiTreeViewItem*) override;

    void		createMenu(MenuHandler*,bool istb) override;
    void		handleMenuCB(CallBacker*) override;
    uiString		createDisplayName() const override;
    void		updateColumnText(int col) override;

    MenuItem		generatemenuitem_;

    static uiODDataTreeItem*	create(const Attrib::SelSpec&,
				       const char* parenttype);

public:

    static void		initClass();
};

} // namespace ExternalAttrib
