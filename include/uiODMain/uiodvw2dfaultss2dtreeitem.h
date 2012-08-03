#ifndef uiodvw2dfaultss2dtreeitem_h
#define uiodvw2dfaultss2dtreeitem_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		June 2010
 RCS:		$Id: uiodvw2dfaultss2dtreeitem.h,v 1.6 2012-08-03 13:01:04 cvskris Exp $
________________________________________________________________________

-*/

#include "uiodmainmod.h"
#include "uiodvw2dtreeitem.h"

#include "emposid.h"

class VW2DFaultSS2D;
class uiODViewer2D;


mClass(uiODMain) uiODVw2DFaultSS2DParentTreeItem : public uiODVw2DTreeItem
{
public:
    				uiODVw2DFaultSS2DParentTreeItem();
				~uiODVw2DFaultSS2DParentTreeItem();

    bool			showSubMenu();

protected:

    bool			init();
    bool			handleSubMenu(int);
    const char*			parentType() const
				{ return typeid(uiODVw2DTreeTop).name(); }
    void			tempObjAddedCB(CallBacker*);
};


mClass(uiODMain) uiODVw2DFaultSS2DTreeItemFactory : public uiODVw2DTreeItemFactory
{
public:
    const char*		name() const		{ return typeid(*this).name(); }
    uiTreeItem*		create() const
			{ return new uiODVw2DFaultSS2DParentTreeItem(); }
    uiTreeItem*         createForVis(const uiODViewer2D&,int visid) const;
};


mClass(uiODMain) uiODVw2DFaultSS2DTreeItem : public uiODVw2DTreeItem
{
public:
    			uiODVw2DFaultSS2DTreeItem(const EM::ObjectID&);
			uiODVw2DFaultSS2DTreeItem(int dispid,bool dummy);
			~uiODVw2DFaultSS2DTreeItem();

    bool		showSubMenu();
    bool		select();

protected:

    bool		init();
    const char*		parentType() const
			{ return typeid(uiODVw2DFaultSS2DParentTreeItem).name(); }
    bool		isSelectable() const			{ return true; }

    void		deSelCB(CallBacker*);
    void		checkCB(CallBacker*);
    void		emobjAbtToDelCB(CallBacker*);
    void		displayMiniCtab();

    const int		cPixmapWidth()				{ return 16; }
    const int		cPixmapHeight()				{ return 10; }
    void		emobjChangeCB(CallBacker*);

    EM::ObjectID	emid_;
    VW2DFaultSS2D*	fssview_;
};


#endif

