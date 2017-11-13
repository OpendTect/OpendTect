#pragma once

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		Apr 2010
________________________________________________________________________

-*/

#include "uiodmainmod.h"
#include "uiodprmantreeitem.h"
#include "uistrings.h"
#include "dbkey.h"
#include "odpresentationmgr.h"

class TrcKeyZSampling;
class uiTreeView;
class uiMenu;
class uiODApplMgr;
class uiODViewer2D;
class Vw2DDataObject;

namespace Attrib { class SelSpec; }


mExpClass(uiODMain) uiODVw2DTreeItem : public uiPresManagedTreeItem
{ mODTextTranslationClass(uiODVw2DTreeItem)
public:

			uiODVw2DTreeItem(const uiString&);
			~uiODVw2DTreeItem();

    void		updSampling(const TrcKeyZSampling&,bool);
    void		updSelSpec(const Attrib::SelSpec*,bool wva);

    void		fillPar(IOPar&) const;
    void		usePar(const IOPar&);

    static bool		create(uiTreeItem*,const uiODViewer2D&,int displayid);
    const uiODVw2DTreeItem* getVW2DItem(int displayid) const;

    void		addKeyBoardEvent();
    virtual const Vw2DDataObject* vw2DObject() const	{ return 0; }
    virtual ViewerID	viewerID() const;

protected:

    virtual const char*	iconName() const		{ return 0; }
    static uiString	sChangeSetup() { return m3Dots(tr("Change setup")); }

    int			displayid_;

    uiODApplMgr*	applMgr();
    uiODViewer2D*	viewer2D();
    const uiODViewer2D* viewer2D() const;

    void		addAction(uiMenu& mnu,uiString txt,int id,
				  const char* icon=0,bool enab=true);

    uiMenu*		createAddMenu();
    bool		isAddItem(int id,bool addall) const;
    uiMenu*		createRemoveMenu();
    bool		isRemoveItem(int id,bool removeall) const;
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


mExpClass(uiODMain) uiODVw2DParentTreeItem
				: public uiPresManagedParentTreeItem
{ mODTextTranslationClass(uiODVw2DParentTreeItem)
public:
			uiODVw2DParentTreeItem(const uiString&);
			~uiODVw2DParentTreeItem();
    bool		init();

    virtual ViewerID	viewerID() const;

    void		getVwr2DOjIDs(const DBKey& mid,
				       TypeSet<int>& vw2ids) const;
protected:

    uiODViewer2D*	viewer2D();
    const uiODViewer2D* viewer2D() const;
    uiODApplMgr*	applMgr();

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
