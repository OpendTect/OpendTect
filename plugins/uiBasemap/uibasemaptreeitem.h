#ifndef uibasemaptreeitem_h
#define uibasemaptreeitem_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		January 2014
 RCS:		$Id$
________________________________________________________________________

-*/

#include "uibasemapmod.h"
#include "factory.h"
#include "uitreeitemmanager.h"

class IOPar;
class uiODApplMgr;
class BaseMap;
class BaseMapObject;

mExpClass(uiBasemap) uiBasemapTreeTop : public uiTreeTopItem
{
public:
			uiBasemapTreeTop(uiTreeView*,BaseMap&);
			~uiBasemapTreeTop();

    void		add(BaseMapObject&);

protected:
    const char*		parentType() const	{ return 0; }
    BaseMap&		basemap_;
};


mExpClass(uiBasemap) uiBasemapTreeItem : public uiTreeItem
{
public:
			mDefineFactoryInClass(uiBasemapTreeItem,factory)
    virtual		~uiBasemapTreeItem();

    int			uiTreeViewItemType() const;
    virtual void	fillPar(IOPar&) const;
    virtual bool	usePar(const IOPar&);

protected:
    uiODApplMgr&	applMgr();
    ObjectSet<BaseMapObject>	basemapobjs_;

};


mExpClass(uiBasemap) uiBasemapWellTreeItem : public uiBasemapTreeItem
{
public:
			mDefaultFactoryInstantiation(
				uiBasemapTreeItem,
				uiBasemapWellTreeItem,
				"Wells",
				sFactoryKeyword())

protected:

    bool		showSubMenu();
    bool		handleSubMenu(int);
    const char*		parentType() const
			{ return typeid(uiBasemapTreeTop).name(); }
};

#endif
