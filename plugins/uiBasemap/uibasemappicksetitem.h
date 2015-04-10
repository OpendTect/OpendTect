#ifndef uibasemappicksetitem_h
#define uibasemappicksetitem_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		January 2014
 RCS:		$Id$
________________________________________________________________________

-*/

#include "uibasemapmod.h"
#include "uibasemapitem.h"

mExpClass(uiBasemap) uiBasemapPickSetParentTreeItem :
					public uiBasemapParentTreeItem
{
public:
			uiBasemapPickSetParentTreeItem(int id)
			    : uiBasemapParentTreeItem("Pick Set",id)	{}
protected:

};



mExpClass(uiBasemap) uiBasemapPickSetTreeItem : public uiBasemapTreeItem
{
public:
			uiBasemapPickSetTreeItem(const char*);
			~uiBasemapPickSetTreeItem();

    bool		usePar(const IOPar&);

protected:

    bool		showSubMenu();
    bool		handleSubMenu(int);
    const char*		parentType() const;
};



mExpClass(uiBasemap) uiBasemapPickSetGroup : public uiBasemapIOObjGroup
{
public:
			uiBasemapPickSetGroup(uiParent*,bool isadd);
			~uiBasemapPickSetGroup();

    bool		acceptOK();
    bool		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);

protected:

};



mExpClass(uiBasemap) uiBasemapPickSetItem : public uiBasemapItem
{
public:
			mDefaultFactoryInstantiation(
				uiBasemapItem,
				uiBasemapPickSetItem,
				"PickSets",
				sFactoryKeyword())

    int			defaultZValue() const;
    const char*		iconName() const;
    uiBasemapGroup*	createGroup(uiParent*,bool isadd);
    uiBasemapParentTreeItem* createParentTreeItem();
    uiBasemapTreeItem*	createTreeItem(const char*);
};

#endif
