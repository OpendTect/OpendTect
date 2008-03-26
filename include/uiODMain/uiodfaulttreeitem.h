#ifndef uiodfaulttreeitem_h
#define uiodfaulttreeitem_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		May 2006
 RCS:		$Id: uiodfaulttreeitem.h,v 1.2 2008-03-26 13:53:54 cvsjaap Exp $
________________________________________________________________________


-*/

#include "uioddisplaytreeitem.h"

#include "emposid.h"


namespace visSurvey { class FaultDisplay; }


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
    MenuItem			dispsticksonlymnuitem_;
    visSurvey::FaultDisplay*	faultdisplay_;
};


#endif
