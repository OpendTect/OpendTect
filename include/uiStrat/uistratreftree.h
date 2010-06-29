#ifndef uistratreftree_h
#define uistratreftree_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          June 2007
 RCS:           $Id: uistratreftree.h,v 1.21 2010-06-29 10:43:54 cvsbruno Exp $
________________________________________________________________________

-*/

#include "callback.h"
#include "ranges.h"
#include "stratunitref.h"

class ioPixmap;
class uiParent;
class uiListView;
class uiListViewItem;
class uiStratMgr;
namespace Strat {
    class RefTree;
    class NodeUnitRef;
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
    const uiStratMgr&	stratmgr() const	{ return *uistratmgr_; }
    void                expand(bool) const;
    void                makeTreeEditable(bool) const;
    void		updateLvlsPixmaps();
    void		updateLithoCol();
    void		moveUnit(bool);
    bool		canMoveUnit(bool);

protected:

    const Strat::RefTree* tree_;

    uiListView*		lv_;
    uiStratMgr*		uistratmgr_;

    void		rClickCB(CallBacker*);
    void		repoChangedCB(CallBacker*);

    void		insertSubUnit(uiListViewItem*);
    void		doInsertSubUnit(uiListViewItem*,Strat::UnitRef::Props&) 					const;
    void		makeNewTreeItem(uiListViewItem*);
    void		removeUnit(uiListViewItem*);
    void		updateUnitProperties(uiListViewItem*);

    void 		setUnconformities(const Strat::NodeUnitRef&,bool,int);
    void 		doSetUnconformities(CallBacker*);

    void		addNode(uiListViewItem*,const Strat::NodeUnitRef&,bool);
    ioPixmap*		createUnitPixmap(const Strat::UnitRef*) const;
    			//becomes yours!

    BufferString	getCodeFromLVIt(const uiListViewItem*) const;

    friend class 	uiStratTreeWriter;
};


#endif
