#ifndef uibasemappolygonitem_h
#define uibasemappolygonitem_h

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

mExpClass(uiBasemap) uiBasemapPolygonGroup : public uiBasemapIOObjGroup
{
public:
			uiBasemapPolygonGroup(uiParent*,bool isadd);
			~uiBasemapPolygonGroup();

    bool		acceptOK();
    bool		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);

protected:

};



mExpClass(uiBasemap) uiBasemapPolygonTreeItem : public uiBasemapTreeItem
{
public:
			uiBasemapPolygonTreeItem(const char*);
			~uiBasemapPolygonTreeItem();

    bool		usePar(const IOPar&);

protected:

    bool		showSubMenu();
    bool		handleSubMenu(int);
    const char*		parentType() const
			{ return typeid(uiBasemapTreeTop).name(); }
};



mExpClass(uiBasemap) uiBasemapPolygonItem : public uiBasemapItem
{
public:
			mDefaultFactoryInstantiation(
				uiBasemapItem,
				uiBasemapPolygonItem,
				"Polygons",
				sFactoryKeyword())

    const char*		iconName() const;
    uiBasemapGroup*	createGroup(uiParent*,bool isadd);
    uiBasemapTreeItem*	createTreeItem(const char*);
};

#endif
