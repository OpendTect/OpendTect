#ifndef uibasemapitem_h
#define uibasemapitem_h

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
#include "helpview.h"
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
			mDefineFactory1ParamInClass(
				uiBasemapGroup,uiParent*,factory)
    virtual		~uiBasemapGroup();

    void		setItemName(const char*);
    const char*		itemName() const;

    virtual HelpKey	getHelpKey() const;

    virtual bool	acceptOK();
    virtual bool	fillPar(IOPar&) const;
    virtual bool	usePar(const IOPar&);

protected:
			uiBasemapGroup(uiParent*);

    void		addNameField(uiObject* attachobj);
    uiGenInput*		namefld_;
};


mExpClass(uiBasemap) uiBasemapTreeItem : public uiTreeItem
{
public:
			mDefineFactory1ParamInClass(
				uiBasemapTreeItem,const char*,factory)
    virtual		~uiBasemapTreeItem();

    int			uiTreeViewItemType() const;

    const IOPar&	pars() const		{ return pars_; }
    virtual bool	usePar(const IOPar&)	 = 0;

protected:
			uiBasemapTreeItem(const char* nm);

    bool		init();
    void		addBasemapObject(BaseMapObject&);
    void		checkCB(CallBacker*);

    virtual bool	isSelectable() const { return true; }
    virtual bool	isExpandable() const { return false; }

    ObjectSet<BaseMapObject> basemapobjs_;
    IOPar&		pars_;
};



mExpClass(uiBasemap) uiBasemapItem : public CallBacker
{
public:
			mDefineFactoryInClass(uiBasemapItem,factory)

    int			ID() const		{ return id_; }

    virtual const char*		iconName() const		= 0;
    virtual uiBasemapGroup*	createGroup(uiParent*)		= 0;
    virtual uiBasemapTreeItem*	createTreeItem(const char*)	= 0;

protected:
			uiBasemapItem();
private:
    int			id_;
};



mExpClass(uiBasemap) uiBasemapManager : public CallBacker
{
public:
			uiBasemapManager();
			~uiBasemapManager();

    void		add(const char* keyw);
    void		edit(const char* keyw,const char* treeitmname);

    void		setBasemap(uiBaseMap&);
    uiBaseMap&		getBasemap();
    void		setTreeTop(uiTreeTopItem&);

    const ObjectSet<uiBasemapItem>&	items() const	{ return basemapitems_;}

private:

    void		init();

    uiBaseMap*		basemap_;
    uiTreeTopItem*	treetop_;

    ObjectSet<uiBasemapItem>		basemapitems_;
    ObjectSet<uiBasemapTreeItem>	treeitems_;
};

mGlobal(uiBasemap) uiBasemapManager& BMM();

#endif
