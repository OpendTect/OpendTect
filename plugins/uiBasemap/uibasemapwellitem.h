#ifndef uibasemapwellitem_h
#define uibasemapwellitem_h

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

mExpClass(uiBasemap) uiBasemapWellGroup : public uiBasemapGroup
{
public:
			uiBasemapWellGroup(uiParent*);
			~uiBasemapWellGroup();

    bool		acceptOK();
    bool		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);

    static const char*	sKeyNrWells();

protected:

    uiIOObjSelGrp*	wellsfld_;
};



mExpClass(uiBasemap) uiBasemapWellTreeItem : public uiBasemapTreeItem
{
public:
			uiBasemapWellTreeItem(const char*);
			~uiBasemapWellTreeItem();

    bool		usePar(const IOPar&);

protected:

    bool		showSubMenu();
    bool		handleSubMenu(int);
    const char*		parentType() const
			{ return typeid(uiBasemapTreeTop).name(); }
};



mExpClass(uiBasemap) uiBasemapWellItem : public uiBasemapItem
{
public:
			mDefaultFactoryInstantiation(
				uiBasemapItem,
				uiBasemapWellItem,
				"Wells",
				sFactoryKeyword())

    const char*		iconName() const;
    uiBasemapGroup*	createGroup(uiParent*);
    uiBasemapTreeItem*	createTreeItem(const char*);
};

#endif
