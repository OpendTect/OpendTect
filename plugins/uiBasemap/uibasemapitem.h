#ifndef uibasemapitem_h
#define uibasemapitem_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		January 2014
 RCS:		$Id: uibasemaptreeitem.h 34190 2014-04-16 20:09:04Z nanne.hemstra@dgbes.com $
________________________________________________________________________

-*/

#include "uibasemapmod.h"
#include "factory.h"
#include "uigroup.h"
#include "uitreeitemmanager.h"

class uiBaseMap;
class uiGenInput;
class uiODApplMgr;
class BaseMapObject;
class IOPar;


mExpClass(uiBasemap) uiBasemapTreeTop : public uiTreeTopItem
{
public:
                        uiBasemapTreeTop(uiTreeView*);
                        ~uiBasemapTreeTop();

protected:
    const char*         parentType() const      { return 0; }
};


mExpClass(uiBasemap) uiBasemapGroup : public uiGroup
{
public:
    virtual		~uiBasemapGroup();

    void		setItemName(const char*);
    const char*		itemName() const;

    virtual bool	fillPar(IOPar&) const;
    virtual bool	usePar(const IOPar&);

protected:
			uiBasemapGroup(uiParent*,const char*);

    void		addNameField(uiObject* attachobj);
    uiGenInput*		namefld_;
};


mExpClass(uiBasemap) uiBasemapItem : public CallBacker
{
public:
			mDefineFactoryInClass(uiBasemapItem,factory)
    virtual		~uiBasemapItem();

    void		setBasemap(uiBaseMap&);
    void		addBasemapObject(BaseMapObject&);

    void		setTreeTop(uiTreeTopItem&);
    void		addTreeItem(uiTreeItem&);

    virtual const char*	iconName() const		= 0;
    virtual void	add()				= 0;
    virtual void	edit()				= 0;

    void		show(bool yn);

    virtual void	fillPar(IOPar&) const;
    virtual bool	usePar(const IOPar&);

protected:
			uiBasemapItem();

    uiParent*		parent();
    void		checkCB(CallBacker*);

    uiODApplMgr&		applMgr();
    ObjectSet<BaseMapObject>	basemapobjs_;
    uiBaseMap*			basemap_;
    uiTreeTopItem*		treetop_;
    uiTreeItem*			treeitem_;
};



mExpClass(uiBasemap) uiBasemapTreeItem : public uiTreeItem
{
public:
    virtual		~uiBasemapTreeItem();

    int			uiTreeViewItemType() const;

protected:
			uiBasemapTreeItem(const char* nm);

    virtual bool	isSelectable() const { return true; }
    virtual bool	isExpandable() const { return false; }

    IOPar&		par_;
};

#endif
