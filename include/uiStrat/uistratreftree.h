#ifndef uistratreftree_h
#define uistratreftree_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          June 2007
 RCS:           $Id: uistratreftree.h,v 1.12 2009-01-08 07:26:59 cvsranojay Exp $
________________________________________________________________________

-*/

#include "callback.h"

class ioPixmap;
class uiParent;
class uiListView;
class uiListViewItem;
class uiStratMgr;
namespace Strat {
    class RefTree;
    class NodeUnitRef;
    class UnitRef;
}


/*!\brief Displays a Strat::RefTree */

mClass uiStratRefTree : public CallBacker 
{
public:

			uiStratRefTree(uiParent*,uiStratMgr*);
			~uiStratRefTree();

    void		setTree(const Strat::RefTree*,bool force =false);

    uiListView*		listView()		{ return lv_; }
    const uiListView*	listView() const	{ return lv_; }
    void                expand(bool) const;
    void                makeTreeEditable(bool) const;
    void		updateLvlsPixmaps();

protected:

    const Strat::RefTree* tree_;

    uiListView*		lv_;
    uiStratMgr*		uistratmgr_;

    void		rClickCB(CallBacker*);

    void		insertSubUnit(uiListViewItem*);
    void		removeUnit(uiListViewItem*);
    void		addNode(uiListViewItem*,const Strat::NodeUnitRef&,bool);
    ioPixmap*		createLevelPixmap(const Strat::UnitRef*) const;
    			//becomes yours!

    BufferString	getCodeFromLVIt(const uiListViewItem*) const;
    void		selBoundary();
};


#endif
