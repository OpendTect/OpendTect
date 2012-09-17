#ifndef uistratreftree_h
#define uistratreftree_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          June 2007
 RCS:           $Id: uistratreftree.h,v 1.36 2012/02/15 15:30:33 cvsbruno Exp $
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
    uiListViewItem* 	getLVItFromFullCode(const char*) const;
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
    void		mousePressedCB(CallBacker*);

    void		insertSubUnit(uiListViewItem*);
    void		subdivideUnit(uiListViewItem*);
    void		updateUnitProperties(uiListViewItem*);
    void		removeUnit(uiListViewItem*);
    void		assignLevelBoundary(uiListViewItem*);
    bool		isLeaved(uiListViewItem*) const;

    void		setUnitLvl(const char*);

    BufferString	getFullCodeFromLVIt(const uiListViewItem*) const;
    void		insertUnitInLVIT(uiListViewItem*,int,
				    const Strat::UnitRef&) const; 

    void		addNode(uiListViewItem*,const Strat::NodeUnitRef&,bool);
    ioPixmap*		createUnitPixmap(const Color& col) const;
    			//becomes yours!
    Strat::NodeUnitRef* replaceUnit(Strat::NodeUnitRef&,bool byleaved);
    void		addLithologies(Strat::LeavedUnitRef&,const TypeSet<int>&);

    void		setNodesDefaultTimes(const Strat::NodeUnitRef&);
    void 		getAvailableTime(const Strat::NodeUnitRef&,
	    					Interval<float>&) const;
    void		ensureUnitTimeOK(Strat::NodeUnitRef&);
    int			getChildIdxFromTime(const Strat::NodeUnitRef&,
	    						float)const;


    friend class 	uiStratDispToTreeTransl;
};


#endif
