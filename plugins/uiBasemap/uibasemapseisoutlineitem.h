#ifndef uibasemapoutline_h
#define uibasemapoutline_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Henrique Mageste
 Date:		November 2014
 RCS:		$Id$
________________________________________________________________________

-*/

#include "uibasemapmod.h"
#include "uibasemapitem.h"

class uiSelLineStyle;

mExpClass(uiBasemap) uiBasemapSeisOutlineGroup : public uiBasemapIOObjGroup
{
public:
			uiBasemapSeisOutlineGroup(uiParent*, bool isadd);
			~uiBasemapSeisOutlineGroup();

    bool		acceptOK();
    bool		fillItemPar(int idx,IOPar&) const;
    bool		usePar(const IOPar&);

protected:
    virtual uiObject*	lastObject();
    uiSelLineStyle*	lsfld_;
};


mExpClass(uiBasemap) uiBasemapSeisOutlineTreeItem : public uiBasemapTreeItem
{
public:
			uiBasemapSeisOutlineTreeItem(const char*);
			~uiBasemapSeisOutlineTreeItem();
    bool		usePar(const IOPar&);

protected:
    bool		showSubMenu();
    bool		handleSubMenu(int);
    const char*		parentType() const
			{ return typeid(uiBasemapTreeTop).name(); }
};


mExpClass(uiBasemap) uiBasemapSeisOutlineItem : public uiBasemapItem
{
public:
			mDefaultFactoryInstantiation(
				uiBasemapItem,
				uiBasemapSeisOutlineItem,
				"Seismic Outline",
				sFactoryKeyword())

    const char*		iconName() const;
    uiBasemapGroup*	createGroup(uiParent*, bool isadd);
    uiBasemapTreeItem*	createTreeItem(const char*);

};

#endif
