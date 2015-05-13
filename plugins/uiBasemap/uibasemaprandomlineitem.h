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
    bool		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);

protected:
    virtual uiObject*	lastObject();
    uiSelLineStyle*	lsfld_;
};



mExpClass(uiBasemap) uiBasemapRandomLineParentTreeItem
				: public uiBasemapParentTreeItem
{
public:
			uiBasemapRandomLineParentTreeItem(int id)
			    : uiBasemapParentTreeItem("Random line",id)
			{}

protected:
    const char*		iconName() const;
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
    const char*		parentType() const;
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
    uiBasemapGroup*	createGroup(uiParent*,bool isadd);
    uiBasemapParentTreeItem* createParentTreeItem();
    uiBasemapTreeItem*	createTreeItem(const char*);
};

#endif
