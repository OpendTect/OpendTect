#pragma once

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
#include "emposid.h"

class TrcKeyZSampling;
class uiTreeView;
class uiMenu;
class uiODApplMgr;
class uiODViewer2D;
class ZAxisTransform;

namespace Attrib { class SelSpec; }


mExpClass(uiODMain) uiODVw2DTreeItem : public uiTreeItem
{ mODTextTranslationClass(uiODVw2DTreeItem)
public:
			uiODVw2DTreeItem(const uiString&);
			~uiODVw2DTreeItem();

    bool		setZAxisTransform(ZAxisTransform*);

    void		updSampling(const TrcKeyZSampling&,bool);
    void		updSelSpec(const Attrib::SelSpec*,bool wva);

    void		fillPar(IOPar&) const;
    void		usePar(const IOPar&);

    static bool		create(uiTreeItem*,int vwrvisid,int displayid);
    static bool		create(uiTreeItem*,const uiODViewer2D&,int displayid);
    const uiODVw2DTreeItem* getVW2DItem(int displayid) const;
    void		addKeyBoardEvent(const EM::ObjectID&);

protected:

    virtual bool	init();
    virtual const char*	iconName() const		{ return 0; }
    static uiString	sChangeSetup() { return m3Dots(tr("Change setup")); }

    int			displayid_;
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
