#ifndef uiodfaulttreeitem_h
#define uiodfaulttreeitem_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		May 2006
 RCS:		$Id: uiodfaulttreeitem.h,v 1.6 2008-10-01 03:44:37 cvsnanne Exp $
________________________________________________________________________


-*/

#include "uioddisplaytreeitem.h"

#include "emposid.h"


namespace visSurvey { class FaultDisplay; class Fault2DDisplay; }


mDefineItem( FaultParent, TreeItem, TreeTop, mShowMenu );


class uiODFaultTreeItemFactory : public uiODTreeItemFactory
{
public:
    const char*		name() const { return typeid(*this).name(); }
    uiTreeItem*		create() const
    			{ return new uiODFaultParentTreeItem; }
    uiTreeItem*		create(int visid,uiTreeItem*) const;
};


class uiODFaultTreeItem : public uiODDisplayTreeItem
{
public:
    			uiODFaultTreeItem( int, bool dummy );
    			uiODFaultTreeItem( const EM::ObjectID& emid );
    			~uiODFaultTreeItem();

protected:
    void		prepareForShutdown();
    void		createMenuCB(CallBacker*);
    void		handleMenuCB(CallBacker*);
    void		colorChCB(CallBacker*);

    bool		init();
    const char*		parentType() const
			{return typeid(uiODFaultParentTreeItem).name();}

    EM::ObjectID		emid_;
    MenuItem			savemnuitem_;
    MenuItem			saveasmnuitem_;
    MenuItem			displaymnuitem_;
    MenuItem			displayplanemnuitem_;
    MenuItem			displaystickmnuitem_;
    MenuItem			displayintersectionmnuitem_;
    MenuItem			singlecolmnuitem_;
    MenuItem			removeselectedmnuitem_;
    visSurvey::FaultDisplay*	faultdisplay_;
};


mDefineItem( Fault2DParent, TreeItem, TreeTop, mShowMenu );


class uiODFault2DTreeItemFactory : public uiODTreeItemFactory
{
public:
    const char*		name() const { return typeid(*this).name(); }
    uiTreeItem*		create() const
    			{ return new uiODFault2DParentTreeItem; }
    uiTreeItem*		create(int visid,uiTreeItem*) const;
};


class uiODFault2DTreeItem : public uiODDisplayTreeItem
{
public:
    			uiODFault2DTreeItem(int,bool dummy);
    			uiODFault2DTreeItem(const EM::ObjectID&);
    			~uiODFault2DTreeItem();

protected:
    void		prepareForShutdown();
    void		createMenuCB(CallBacker*);
    void		handleMenuCB(CallBacker*);
    void		colorChCB(CallBacker*);

    bool		init();
    const char*		parentType() const
			{return typeid(uiODFault2DParentTreeItem).name();}

    EM::ObjectID		emid_;
    MenuItem			savemnuitem_;
    MenuItem			saveasmnuitem_;
    MenuItem			removeselectedmnuitem_;
    visSurvey::Fault2DDisplay*	fault2ddisplay_;
};


#endif
