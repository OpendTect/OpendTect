#ifndef uibasemaphorizon3ditem_h
#define uibasemaphorizon3ditem_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Henrique Mageste
 Date:		December 2014
 RCS:		$Id$
________________________________________________________________________

-*/

#include "uibasemapmod.h"
#include "uibasemapitem.h"


mExpClass(uiBasemap) uiBasemapHorizon3DGroup : public uiBasemapGroup
{
public:
			uiBasemapHorizon3DGroup(uiParent*,bool isadd);
			~uiBasemapHorizon3DGroup();

    bool		acceptOK();
    bool		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);

protected:
    virtual uiObject*	lastObject();
    void		selChg( CallBacker* );

    uiIOObjSelGrp*	ioobjfld_;
    TypeSet<MultiID>	mids_;
};


mExpClass(uiBasemap) uiBasemapHorizon3DTreeItem : public uiBasemapTreeItem
{
public:
			uiBasemapHorizon3DTreeItem(const char*);
			~uiBasemapHorizon3DTreeItem();
    bool		usePar(const IOPar&);

protected:
    bool		showSubMenu();
    bool		handleSubMenu(int);
    const char*		parentType() const
			{ return typeid(uiBasemapTreeTop).name(); }
};


mExpClass(uiBasemap) uiBasemapHorizon3DItem : public uiBasemapItem
{
public:
			mDefaultFactoryInstantiation(
				uiBasemapItem,
				uiBasemapHorizon3DItem,
				"Horizon3D",
				sFactoryKeyword())

    const char*		iconName() const;
    uiBasemapGroup*	createGroup(uiParent*, bool isadd);
    uiBasemapTreeItem*	createTreeItem(const char*);

};

#endif
