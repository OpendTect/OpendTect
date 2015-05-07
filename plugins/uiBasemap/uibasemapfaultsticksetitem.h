#ifndef uibasemapfaultsticksetitem_h
#define uibasemapfaultsticksetitem_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Henrique Mageste
 Date:		May 2015
 RCS:		$Id: $
________________________________________________________________________

-*/

#include "uibasemapmod.h"
#include "uibasemapitem.h"

mExpClass(uiBasemap) uiBasemapFaultStickSetParentTreeItem :
					public uiBasemapParentTreeItem
{
public:
			uiBasemapFaultStickSetParentTreeItem(int id)
			    : uiBasemapParentTreeItem("FaultStickSet",id) {}

protected:
    virtual bool	showSubMenu();
    const char*		iconName() const;
};



mExpClass(uiBasemap) uiBasemapFaultStickSetTreeItem : public uiBasemapTreeItem
{
public:
			uiBasemapFaultStickSetTreeItem(const char*);
			~uiBasemapFaultStickSetTreeItem();
    bool		usePar(const IOPar&);

protected:
    bool		showSubMenu();
    bool		handleSubMenu(int);
    const char*		parentType() const;
};



mExpClass(uiBasemap) uiBasemapFaultStickSetItem : public uiBasemapItem
{
public:
			mDefaultFactoryInstantiation(
				uiBasemapItem,
				uiBasemapFaultStickSetItem,
				"FaultStickSet",
				sFactoryKeyword())

    int			defaultZValue() const;
    uiBasemapGroup*	createGroup(uiParent*, bool isadd);
    uiBasemapParentTreeItem* createParentTreeItem();
    uiBasemapTreeItem*	createTreeItem(const char*);

};

#endif

