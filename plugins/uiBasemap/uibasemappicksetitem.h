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

mExpClass(uiBasemap) uiBasemapPickSetGroup : public uiBasemapIOObjGroup
{
public:
			uiBasemapPickSetGroup(uiParent*);
			~uiBasemapPickSetGroup();

    bool		acceptOK();
    bool		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);

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
    const char*		parentType() const
			{ return typeid(uiBasemapTreeTop).name(); }
};



mExpClass(uiBasemap) uiBasemapPickSetItem : public uiBasemapItem
{
public:
			mDefaultFactoryInstantiation(
				uiBasemapItem,
				uiBasemapPickSetItem,
				"PickSets",
				sFactoryKeyword())

    const char*		iconName() const;
    uiBasemapGroup*	createGroup(uiParent*);
    uiBasemapTreeItem*	createTreeItem(const char*);
};

#endif
