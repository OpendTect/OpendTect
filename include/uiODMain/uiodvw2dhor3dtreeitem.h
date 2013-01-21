#ifndef uiodvw2dhor3dtreeitem_h
#define uiodvw2dhor3dtreeitem_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		May 2010
 RCS:		$Id$
________________________________________________________________________

-*/

#include "uiodmainmod.h"
#include "uiodvw2dtreeitem.h"

#include "emposid.h"

class Vw2DHorizon3D;
class uiODViewer2D;


mExpClass(uiODMain) uiODVw2DHor3DParentTreeItem : public uiODVw2DTreeItem
{
public:
    				uiODVw2DHor3DParentTreeItem();
				~uiODVw2DHor3DParentTreeItem();

    bool			showSubMenu();

protected:

    bool			init();
    bool                        handleSubMenu(int);
    const char*			parentType() const
				{ return typeid(uiODVw2DTreeTop).name(); }
    void			tempObjAddedCB(CallBacker*);
};


mExpClass(uiODMain) uiODVw2DHor3DTreeItemFactory : public uiODVw2DTreeItemFactory
{
public:
    const char*		name() const		{ return typeid(*this).name(); }
    uiTreeItem*         create() const
			{ return new uiODVw2DHor3DParentTreeItem(); }
    uiTreeItem*         createForVis(const uiODViewer2D&,int visid) const;
};


mExpClass(uiODMain) uiODVw2DHor3DTreeItem : public uiODVw2DTreeItem
{
public:
    			uiODVw2DHor3DTreeItem(const EM::ObjectID&);
    			uiODVw2DHor3DTreeItem(int id,bool dummy);
			~uiODVw2DHor3DTreeItem();

    bool		select();
    bool		showSubMenu();

protected:

    bool		init();
    const char*		parentType() const
    			{ return typeid(uiODVw2DHor3DParentTreeItem).name(); }
    bool		isSelectable() const			{ return true; }


    void		updateSelSpec(const Attrib::SelSpec*,bool wva);
    void		updateCS(const CubeSampling&,bool upd);
    void                checkCB(CallBacker*);
    void		deSelCB(CallBacker*);
    void		mousePressInVwrCB(CallBacker*);
    void		mouseReleaseInVwrCB(CallBacker*);
    void		msRelEvtCompletedInVwrCB(CallBacker*);
    void		displayMiniCtab();

    int			cPixmapWidth()				{ return 16; }
    int			cPixmapHeight()				{ return 10; }
    void		emobjChangeCB(CallBacker*);
    
    EM::ObjectID        emid_;
    Vw2DHorizon3D*	horview_;
    bool		oldactivevolupdated_;
    bool		trackerefed_;
    void                emobjAbtToDelCB(CallBacker*);
};

#endif

