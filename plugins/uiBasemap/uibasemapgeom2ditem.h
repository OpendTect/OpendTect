#ifndef uibasemapgeom2ditem_h
#define uibasemapgeom2ditem_h

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

class uiIOObjSelGrp;

mExpClass(uiBasemap) uiBasemapGeom2DGroup : public uiBasemapGroup
{
public:
			uiBasemapGeom2DGroup(uiParent*);
			~uiBasemapGeom2DGroup();

    bool		acceptOK();
    bool		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);

protected:

    uiIOObjSelGrp*	geom2dfld_;
};



mExpClass(uiBasemap) uiBasemapGeom2DTreeItem : public uiBasemapTreeItem
{
public:
			uiBasemapGeom2DTreeItem(const char*);
			~uiBasemapGeom2DTreeItem();

    bool		usePar(const IOPar&);

protected:

    bool		showSubMenu();
    bool		handleSubMenu(int);
    const char*		parentType() const
			{ return typeid(uiBasemapTreeTop).name(); }
};



mExpClass(uiBasemap) uiBasemapGeom2DItem : public uiBasemapItem
{
public:
			mDefaultFactoryInstantiation(
				uiBasemapItem,
				uiBasemapGeom2DItem,
				"Geom2Ds",
				sFactoryKeyword())

    const char*		iconName() const;
    uiBasemapGroup*	createGroup(uiParent*);
    uiBasemapTreeItem*	createTreeItem(const char*);
};

#endif
