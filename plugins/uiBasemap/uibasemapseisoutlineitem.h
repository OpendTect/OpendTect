#ifndef uibasemapseisoutlineitem_h
#define uibasemapseisoutlineitem_h

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

class uiColorInput;
class uiSelLineStyle;

mExpClass(uiBasemap) uiBasemapSeisOutlineGroup : public uiBasemapIOObjGroup
{
public:
			uiBasemapSeisOutlineGroup(uiParent*, bool isadd);
			~uiBasemapSeisOutlineGroup();

    bool		acceptOK();
    bool		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);

protected:
    virtual uiObject*	lastObject();
    uiSelLineStyle*	lsfld_;
    uiGenInput*		linespacingfld_;
    uiColorInput*	fillcolfld_;

private:
    void		setLineSpacing();
};



mExpClass(uiBasemap) uiBasemapSeisOutlineParentTreeItem
					: public uiBasemapParentTreeItem
{
public:
			uiBasemapSeisOutlineParentTreeItem(int id)
			    : uiBasemapParentTreeItem("Seismic Outline",id)
			{}

protected:
    const char*		iconName() const;
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
    const char*		parentType() const;
};



mExpClass(uiBasemap) uiBasemapSeisOutlineItem : public uiBasemapItem
{
public:
			mDefaultFactoryInstantiation(
			    uiBasemapItem,
			    uiBasemapSeisOutlineItem,
			    "Seismic Outline",
			    sFactoryKeyword())

    int			defaultZValue() const;
    uiBasemapGroup*	createGroup(uiParent*, bool isadd);
    uiBasemapParentTreeItem* createParentTreeItem();
    uiBasemapTreeItem*	createTreeItem(const char*);

};

#endif
