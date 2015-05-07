#ifndef uibasemapfaultitem_h
#define uibasemapfaultitem_h

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

mExpClass(uiBasemap) uiBasemapFaultParentTreeItem :
					public uiBasemapParentTreeItem
{
public:
			uiBasemapFaultParentTreeItem(int id)
			    : uiBasemapParentTreeItem("Fault",id)	{}

protected:
    virtual bool	showSubMenu();
    const char*		iconName() const;
};



mExpClass(uiBasemap) uiBasemapFaultTreeItem : public uiBasemapTreeItem
{
public:
			uiBasemapFaultTreeItem(const char*);
			~uiBasemapFaultTreeItem();
    bool		usePar(const IOPar&);

protected:
    bool		showSubMenu();
    bool		handleSubMenu(int);
    const char*		parentType() const;
};



mExpClass(uiBasemap) uiBasemapFaultItem : public uiBasemapItem
{
public:
			mDefaultFactoryInstantiation(
				uiBasemapItem,
				uiBasemapFaultItem,
				"Fault",
				sFactoryKeyword())

    int			defaultZValue() const;
    uiBasemapGroup*	createGroup(uiParent*, bool isadd);
    uiBasemapParentTreeItem* createParentTreeItem();
    uiBasemapTreeItem*	createTreeItem(const char*);

};

#endif

