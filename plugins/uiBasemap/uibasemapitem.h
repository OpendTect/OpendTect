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
#include "multiid.h"
#include "uigroup.h"
#include "uitreeitemmanager.h"

class uiBaseMap;
class uiGenInput;
class uiIOObjSelGrp;
class uiODApplMgr;
class BaseMapMarkers;
class BaseMapObject;
class IOObjContext;
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

    virtual HelpKey	getHelpKey() const;

    virtual bool	acceptOK();
    virtual bool	fillPar(IOPar&) const;
    virtual bool	fillItemPar(int idx,IOPar&) const	{ return true; }
    virtual bool	usePar(const IOPar&);

    static const char*	sKeyNrObjs();
    static const char*	sKeyNrItems();
    static const char*	sKeyItem();

protected:
			uiBasemapGroup(uiParent*);

    void		addNameField();
    virtual uiObject*	lastObject()		{ return 0; }

    uiGenInput*		namefld_;
    BufferString	defaultname_;
};


mExpClass(uiBasemap) uiBasemapIOObjGroup : public uiBasemapGroup
{
public:
    virtual		~uiBasemapIOObjGroup();

    virtual bool	acceptOK();
    virtual bool	fillPar(IOPar&) const;
    virtual bool	fillItemPar(int idx,IOPar&) const;
    virtual bool	usePar(const IOPar&);

protected:
			uiBasemapIOObjGroup(uiParent*,const IOObjContext&,
					    bool isadd);

    virtual uiObject*	lastObject();
    void		selChg(CallBacker*);
    void		typeChg(CallBacker*);
    int			nrItems() const;
    int			nrObjsPerItem() const;

    uiIOObjSelGrp*	ioobjfld_;
    uiGenInput*		typefld_;

    TypeSet<MultiID>	mids_;
};


mExpClass(uiBasemap) uiBasemapTreeItem : public uiTreeItem
{
public:
    virtual		~uiBasemapTreeItem();

    int			ID() const		{ return id_; }
    int			uiTreeViewItemType() const;

    void		setFamilyID( int id )	{ familyid_ = id; }
    int			getFamilyID() const	{ return familyid_; }

    const IOPar&	pars() const		{ return pars_; }
    virtual bool	usePar(const IOPar&);

protected:
			uiBasemapTreeItem(const char* nm);

    bool		init();
    void		addBasemapObject(BaseMapObject&);
    BaseMapObject*	removeBasemapObject(BaseMapObject&);
    void		checkCB(CallBacker*);

    virtual bool	showSubMenu();
    virtual bool	handleSubMenu(int);

    virtual bool	isSelectable() const { return true; }
    virtual bool	isExpandable() const { return false; }

    ObjectSet<BaseMapObject> basemapobjs_;
    IOPar&		pars_;

private:
    int			id_;
    int			familyid_;
};



mExpClass(uiBasemap) uiBasemapItem : public CallBacker
{
public:
			mDefineFactoryInClass(uiBasemapItem,factory)

    int			ID() const		{ return id_; }

    virtual const char*		iconName() const		      = 0;
    virtual uiBasemapGroup*	createGroup(uiParent*,bool)	      = 0;
    virtual uiBasemapTreeItem*	createTreeItem(const char*)	      = 0;

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

    void		add(int itmid);
    void		addfromPar(const IOPar&);
    void		edit(int itmid,int treeitmid);

    void		setBasemap(uiBaseMap&);
    uiBaseMap&		getBasemap();
    void		setTreeTop(uiTreeTopItem&);

    const ObjectSet<uiBasemapItem>&	items() const	{ return basemapitems_;}
    const ObjectSet<uiBasemapTreeItem>& treeitems() const { return treeitems_;}
    uiBasemapItem*	getBasemapItem(const char*);
    uiBasemapItem*	getBasemapItem(int id);
    uiBasemapTreeItem*	getBasemapTreeItem(int id);

    void		removeSelectedItems();
    void		removeAllItems();

    void		updateMouseCursor(const Coord3&);

private:

    void		init();

    uiBaseMap*		basemap_;
    BaseMapMarkers*	basemapcursor_;
    uiTreeTopItem*	treetop_;

    ObjectSet<uiBasemapItem>		basemapitems_;
    ObjectSet<uiBasemapTreeItem>	treeitems_;
};

mGlobal(uiBasemap) uiBasemapManager& BMM();

#endif
