#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiodmainmod.h"
#include "uitreeitemmanager.h"
#include "uistrings.h"
#include "emposid.h"

class TrcKeyZSampling;
class uiTreeView;
class uiMenu;
class uiODApplMgr;
class uiODViewer2D;
class ZAxisTransform;

namespace Attrib { class SelSpec; }


mExpClass(uiODMain) uiODView2DTreeItem : public uiTreeItem
{ mODTextTranslationClass(uiODView2DTreeItem)
public:
			uiODView2DTreeItem(const uiString&);
			~uiODView2DTreeItem();

    bool		setZAxisTransform(ZAxisTransform*);

    void		updSampling(const TrcKeyZSampling&,bool);
    void		updSelSpec(const Attrib::SelSpec*,bool wva);

    void		fillPar(IOPar&) const;
    void		usePar(const IOPar&);

    static bool		create(uiTreeItem*,VisID,Vis2DID);
    static bool		create(uiTreeItem*,const uiODViewer2D&,Vis2DID);
    const uiODView2DTreeItem* getView2DItem(Vis2DID) const;
    void		addKeyBoardEvent(const EM::ObjectID&);

protected:

    virtual bool	init();
    virtual const char*	iconName() const		{ return 0; }
    static uiString	sChangeSetup() { return m3Dots(tr("Change setup")); }

    Vis2DID		displayid_;
    EM::ObjectID	emobjid_;
    ZAxisTransform*	datatransform_;

    uiODApplMgr*	applMgr();
    uiODViewer2D*	viewer2D();

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
    virtual void	showAllChildren();
    virtual void	hideAllChildren();
    virtual void	removeAllChildren();
    void		keyPressedCB(CallBacker*);
    void		doSave();
    void		doSaveAs();
    void		renameVisObj();

private:
    void		doStoreObject(bool);
};


mExpClass(uiODMain) uiODView2DTreeItemFactory : public uiTreeItemFactory
{
public:
    virtual uiTreeItem* createForVis(const uiODViewer2D&,Vis2DID) const = 0;
};



mExpClass(uiODMain) uiODView2DTreeTop : public uiTreeTopItem
{
public:
				uiODView2DTreeTop(uiTreeView*,uiODApplMgr*,
					uiODViewer2D*,uiTreeFactorySet*);
				~uiODView2DTreeTop();

    static const char*		viewer2dptr();
    static const char*		applmgrstr();

    bool			setZAxisTransform(ZAxisTransform*);

    void			updSampling(const TrcKeyZSampling&,bool);
    void			updSelSpec(const Attrib::SelSpec*,bool wva);
    const uiODView2DTreeItem*	getView2DItem(Vis2DID) const;

protected:

    void			addFactoryCB(CallBacker*);
    void			removeFactoryCB(CallBacker*);

    virtual const char*		parentType() const { return 0; }
    uiODApplMgr*		applMgr();
    uiODViewer2D*		viewer2D();

    uiTreeFactorySet*		tfs_;
    bool			selectWithKey(int);
};
