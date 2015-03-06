#ifndef uibasemaprandomlineitem_h
#define uibasemaprandomlineitem_h

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

class uiSelLineStyle;

mExpClass(uiBasemap) uiBasemapRandomLineGroup : public uiBasemapIOObjGroup
{
public:
			uiBasemapRandomLineGroup(uiParent*,bool isadd);
			~uiBasemapRandomLineGroup();

    bool		acceptOK();
    bool		fillItemPar(int idx,IOPar&) const;
    bool		usePar(const IOPar&);

protected:
    virtual uiObject*	lastObject();
    uiSelLineStyle*	lsfld_;
};



mExpClass(uiBasemap) uiBasemapRandomLineTreeItem : public uiBasemapTreeItem
{
public:
			uiBasemapRandomLineTreeItem(const char*);
			~uiBasemapRandomLineTreeItem();

    bool		usePar(const IOPar&);

protected:

    bool		showSubMenu();
    bool		handleSubMenu(int);
    const char*		parentType() const
			{ return typeid(uiBasemapTreeTop).name(); }
};



mExpClass(uiBasemap) uiBasemapRandomLineItem : public uiBasemapItem
{
public:
			mDefaultFactoryInstantiation(
				uiBasemapItem,
				uiBasemapRandomLineItem,
				"Random lines",
				sFactoryKeyword())

    int			defaultZValue() const;
    const char*		iconName() const;
    uiBasemapGroup*	createGroup(uiParent*,bool isadd);
    uiBasemapTreeItem*	createTreeItem(const char*);
};

#endif
