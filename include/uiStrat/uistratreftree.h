#ifndef uistratreftree_h
#define uistratreftree_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          June 2007
 RCS:           $Id: uistratreftree.h,v 1.29 2010-09-27 11:05:19 cvsbruno Exp $
________________________________________________________________________

-*/

#include "callback.h"
#include "ranges.h"
#include "stratunitref.h"

class ioPixmap;
class uiParent;
class uiListView;
class uiListViewItem;
namespace Strat {
    class RefTree;
    class NodeUnitRef;
}


/*!\brief Displays a Strat::RefTree */

mClass uiStratRefTree : public CallBacker 
{
public:

			uiStratRefTree(uiParent*);
			~uiStratRefTree();

    void		setTree(Strat::RefTree&,bool force =false);
    
    const Strat::RefTree* tree() const 		{ return tree_; }

    uiListView*		listView()		{ return lv_; }
    const uiListView*	listView() const	{ return lv_; }
    void                expand(bool) const;
    void                makeTreeEditable(bool) const;
    void		handleMenu(uiListViewItem*);
    void		updateUnitsPixmaps();
    void		updateLithoCol();
    void		moveUnit(bool);
    bool		canMoveUnit(bool);

    void		setEntranceDefaultTimes();
    bool		haveTimes() const;

protected:

    Strat::RefTree* 	tree_;

    uiListView*		lv_;

    void		rClickCB(CallBacker*);

    void		insertSubUnit(uiListViewItem*);
    void		subdivideUnit(uiListViewItem*);
    void		updateUnitProperties(uiListViewItem*);
    void		removeUnit(uiListViewItem*);

    void		setUnitLvl(const char*);

    BufferString	getCodeFromLVIt(const uiListViewItem*) const;
    void		insertUnitInLVIT(uiListViewItem*,
				    const Strat::UnitRef&) const; 

    void		addNode(uiListViewItem*,const Strat::NodeUnitRef&,bool);
    ioPixmap*		createUnitPixmap(const Color& col) const;
    			//becomes yours!
    void		updateParentUnit(const char*);

    void		setNodesDefaultTimes(const Strat::NodeUnitRef&);
    friend class 	uiStratDispToTreeTransl;
};


#endif
