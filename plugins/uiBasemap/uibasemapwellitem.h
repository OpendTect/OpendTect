#ifndef uibasemapwellitem_h
#define uibasemapwellitem_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		January 2014
 RCS:		$Id: uibasemapwellitem.h 34190 2014-04-16 20:09:04Z nanne.hemstra@dgbes.com $
________________________________________________________________________

-*/

#include "uibasemapmod.h"
#include "uibasemapitem.h"

class uiIOObjSelGrp;

mExpClass(uiBasemap) uiBasemapWellGroup : public uiBasemapGroup
{
public:
			uiBasemapWellGroup(uiParent*,const char* nm);
			~uiBasemapWellGroup();

    bool		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);

protected:
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
    uiTreeItem*		getTreeItem(const char* nm);

    void		add();
    void		edit();

protected:

};


mExpClass(uiBasemap) uiBasemapWellTreeItem : public uiBasemapTreeItem
{
public:
			uiBasemapWellTreeItem(const char*);

protected:

    bool		showSubMenu();
    bool		handleSubMenu(int);
    const char*		parentType() const
			{ return typeid(uiBasemapTreeTop).name(); }
};

#endif
