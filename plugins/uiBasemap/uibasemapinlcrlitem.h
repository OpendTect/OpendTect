#ifndef uibasemapinlcrlitem_h
#define uibasemapinlcrlitem_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Henrique Mageste
 Date:		April 2015
 RCS:		$Id: $
________________________________________________________________________

-*/

#include "uibasemapmod.h"
#include "uibasemapitem.h"

class uiSelLineStyle;


mExpClass(uiBasemap) uiBasemapInlCrlGroup : public uiBasemapGroup
{
public:
			uiBasemapInlCrlGroup(uiParent*, bool isadd);
			~uiBasemapInlCrlGroup();

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


// In-line
mExpClass(uiBasemap) uiBasemapInlParentTreeItem :
					public uiBasemapParentTreeItem
{
public:
			uiBasemapInlParentTreeItem(int id)
			    : uiBasemapParentTreeItem("In-line",id)	{}
protected:

};



mExpClass(uiBasemap) uiBasemapInlTreeItem : public uiBasemapTreeItem
{
public:
			uiBasemapInlTreeItem(const char*);
			~uiBasemapInlTreeItem();
    bool		usePar(const IOPar&);

protected:
    bool		showSubMenu();
    bool		handleSubMenu(int);
    const char*		parentType() const;
};



mExpClass(uiBasemap) uiBasemapInlItem : public uiBasemapItem
{
public:
			mDefaultFactoryInstantiation(
				uiBasemapItem,
				uiBasemapInlItem,
				"Inl",
				sFactoryKeyword())

    int			defaultZValue() const;
    const char*		iconName() const;
    uiBasemapGroup*	createGroup(uiParent*, bool isadd);
    uiBasemapParentTreeItem* createParentTreeItem();
    uiBasemapTreeItem*	createTreeItem(const char*);

};


// Cross-line
mExpClass(uiBasemap) uiBasemapCrlParentTreeItem :
					public uiBasemapParentTreeItem
{
public:
			uiBasemapCrlParentTreeItem(int id)
			    : uiBasemapParentTreeItem("Cross-line",id)	{}
protected:

};



mExpClass(uiBasemap) uiBasemapCrlTreeItem : public uiBasemapTreeItem
{
public:
			uiBasemapCrlTreeItem(const char*);
			~uiBasemapCrlTreeItem();
    bool		usePar(const IOPar&);

protected:
    bool		showSubMenu();
    bool		handleSubMenu(int);
    const char*		parentType() const;
};



mExpClass(uiBasemap) uiBasemapCrlItem : public uiBasemapItem
{
public:
			mDefaultFactoryInstantiation(
				uiBasemapItem,
				uiBasemapCrlItem,
				"Crl",
				sFactoryKeyword())

    int			defaultZValue() const;
    const char*		iconName() const;
    uiBasemapGroup*	createGroup(uiParent*, bool isadd);
    uiBasemapParentTreeItem* createParentTreeItem();
    uiBasemapTreeItem*	createTreeItem(const char*);

};

#endif

