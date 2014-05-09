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
			mDefaultFactoryInstantiation1Param(
				uiBasemapGroup,
				uiBasemapWellGroup,
				uiParent*,
				"Wells",
				sFactoryKeyword())

			~uiBasemapWellGroup();

    bool		acceptOK();
    bool		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);

    static const char*	sKeyNrWells();

protected:
			uiBasemapWellGroup(uiParent*);

    uiIOObjSelGrp*	wellsfld_;
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
};


mExpClass(uiBasemap) uiBasemapWellTreeItem : public uiBasemapTreeItem
{
public:
			mDefaultFactoryInstantiation1Param(
				uiBasemapTreeItem,
				uiBasemapWellTreeItem,
				const char*,
				"Wells",
				sFactoryKeyword())

    bool		usePar(const IOPar&);

protected:
			uiBasemapWellTreeItem(const char*);

    bool		showSubMenu();
    bool		handleSubMenu(int);
    const char*		parentType() const
			{ return typeid(uiBasemapTreeTop).name(); }
};

#endif
