#ifndef uiodvw2dtreeitem_h
#define uiodvw2dtreeitem_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		Apr 2010
________________________________________________________________________

-*/

#include "uiodmainmod.h"
#include "uitreeitemmanager.h"
#include "uistrings.h"
#include "multiid.h"

class TrcKeyZSampling;
class uiTreeView;
class uiMenu;
class uiODApplMgr;
class uiODViewer2D;
class ZAxisTransform;
class SaveableManager;
class Vw2DDataObject;

namespace Attrib { class SelSpec; }


mExpClass(uiODMain) uiODVw2DTreeItem : public uiTreeItem
{ mODTextTranslationClass(uiODVw2DTreeItem)
public:
			uiODVw2DTreeItem(const uiString&);
			~uiODVw2DTreeItem();

    bool		setZAxisTransform(ZAxisTransform*);
    const MultiID&	storedID() const	{ return storedid_; }

    void		updSampling(const TrcKeyZSampling&,bool);
    void		updSelSpec(const Attrib::SelSpec*,bool wva);

    void		fillPar(IOPar&) const;
    void		usePar(const IOPar&);

    static bool		create(uiTreeItem*,int vwrvisid,int displayid);
    static bool		create(uiTreeItem*,const uiODViewer2D&,int displayid);
    const uiODVw2DTreeItem* getVW2DItem(int displayid) const;
    virtual void	enableDisplay(bool show,bool triggervwreq)	{}

    void		addKeyBoardEvent();
    virtual const Vw2DDataObject* vw2DObject() const	{ return 0; }

protected:

    virtual bool	init();
    virtual const char*	iconName() const		{ return 0; }
    static uiString	sChangeSetup() { return m3Dots(tr("Change setup")); }

    int			displayid_;
    ZAxisTransform*	datatransform_;
    MultiID		storedid_;

    uiODApplMgr*	applMgr();
    uiODViewer2D*	viewer2D();

    void		addAction(uiMenu& mnu,uiString txt,int id,
				  const char* icon=0,bool enab=true);

    uiMenu*		createAddMenu();
    bool		isAddItem(int id,bool addall) const;
    int			getNewItemID() const;

    virtual void	insertStdSubMenu(uiMenu&);
    virtual bool	handleStdSubMenu(int menuid);

    virtual void	updateCS(const TrcKeyZSampling&,bool)	{}
    virtual void	updateSelSpec(const Attrib::SelSpec*,bool wva)	{}
    virtual void	dataTransformCB(CallBacker*)		{}
    void		keyPressedCB(CallBacker*);
    virtual void	showAllChildren();
    virtual void	hideAllChildren();
    virtual void	removeAllChildren();
    virtual void	doSave() {}
    virtual void	doSaveAs() {}
};


mExpClass(uiODMain) uiODVw2DParentTreeItem : public uiODVw2DTreeItem
{ mODTextTranslationClass(uiODVw2DParentTreeItem)
public:
			uiODVw2DParentTreeItem(const uiString&);
			~uiODVw2DParentTreeItem();
    void		setObjectManager(SaveableManager*);

    void		getVwr2DOjIDs(const MultiID& mid,
				       TypeSet<int>& vw2ids) const;
    void		getLoadedChildren(TypeSet<MultiID>&) const;
    void		showHideChildren(const MultiID&,bool);
    void		removeChildren(const MultiID&);
    void		addChildren(const TypeSet<MultiID>&);
    bool		selectChild(const MultiID&);
protected:
    virtual void	objAddedCB(CallBacker*);
    virtual void	objVanishedCB(CallBacker*);
    virtual void	objShownCB(CallBacker*);
    virtual void	objHiddenCB(CallBacker*);
    virtual void	objOrphanedCB(CallBacker*);

    virtual void	addChildItem(const MultiID&)		{}

    SaveableManager*	objectmgr_;
};


mExpClass(uiODMain) uiODVw2DTreeItemFactory : public uiTreeItemFactory
{
    public:
	virtual uiTreeItem* createForVis(const uiODViewer2D&,int visid) const
				{ return 0; }
};



mExpClass(uiODMain) uiODVw2DTreeTop : public uiTreeTopItem
{
public:
				uiODVw2DTreeTop(uiTreeView*,uiODApplMgr*,
					uiODViewer2D*,uiTreeFactorySet*);
				~uiODVw2DTreeTop();

    static const char*		viewer2dptr();
    static const char*		applmgrstr();

    bool			setZAxisTransform(ZAxisTransform*);

    void			updSampling(const TrcKeyZSampling&,bool);
    void			updSelSpec(const Attrib::SelSpec*,bool wva);
    const uiODVw2DTreeItem*	getVW2DItem(int displayid) const;

protected:

    void			addFactoryCB(CallBacker*);
    void			removeFactoryCB(CallBacker*);

    virtual const char*		parentType() const { return 0; }
    uiODApplMgr*		applMgr();
    uiODViewer2D*		viewer2D();

    uiTreeFactorySet*		tfs_;
    bool			selectWithKey(int);
};


#endif
