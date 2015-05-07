#ifndef uibasemapcontouritem_h
#define uibasemapcontouritem_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Henrique Mageste
 Date:		February 2015
 RCS:		$Id$
________________________________________________________________________

-*/

#include "uibasemapmod.h"
#include "uibasemapitem.h"

class uiSelLineStyle;

mExpClass(uiBasemap) uiBasemapContourParentTreeItem :
					public uiBasemapParentTreeItem
{
public:
			uiBasemapContourParentTreeItem(int id)
			    : uiBasemapParentTreeItem("Contour",id)	{}

protected:
    const char*		iconName() const;
};



mExpClass(uiBasemap) uiBasemapContourTreeItem : public uiBasemapTreeItem
{
public:
			uiBasemapContourTreeItem(const char*);
			~uiBasemapContourTreeItem();
    bool		usePar(const IOPar&);

protected:
    bool		showSubMenu();
    bool		handleSubMenu(int);
    const char*		parentType() const;
};



mExpClass(uiBasemap) uiBasemapContourGroup : public uiBasemapGroup
{
public:
			uiBasemapContourGroup(uiParent*, bool isadd);
			~uiBasemapContourGroup();

    bool		acceptOK();
    bool		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);

protected:
    void		valChgCB(CallBacker*);
    uiObject*		lastObject();

    MultiID		mid_;

    uiIOObjSelGrp*	ioobjfld_;
    uiGenInput*		spacingfld_;
    uiSelLineStyle*	lsfld_;

private:
    void		selChg(CallBacker*);
    void		setParameters();
};



mExpClass(uiBasemap) uiBasemapContourItem : public uiBasemapItem
{
public:
			mDefaultFactoryInstantiation(
				uiBasemapItem,
				uiBasemapContourItem,
				"Contour",
				sFactoryKeyword())

    int			defaultZValue() const;
    uiBasemapGroup*	createGroup(uiParent*, bool isadd);
    uiBasemapParentTreeItem* createParentTreeItem();
    uiBasemapTreeItem*	createTreeItem(const char*);

};

#endif
