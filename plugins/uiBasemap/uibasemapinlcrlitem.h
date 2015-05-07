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

mExpClass(uiBasemap) uiBasemapInlParentTreeItem :
					public uiBasemapParentTreeItem
{
public:
			uiBasemapInlParentTreeItem(int id)
			    : uiBasemapParentTreeItem("In-line",id)	{}

protected:
    virtual bool	showSubMenu();
    const char*		iconName() const;
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
    virtual bool	showSubMenu();
    const char*		iconName() const;
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
    uiBasemapGroup*	createGroup(uiParent*, bool isadd);
    uiBasemapParentTreeItem* createParentTreeItem();
    uiBasemapTreeItem*	createTreeItem(const char*);
};

#endif
