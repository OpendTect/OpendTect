#ifndef uistratreftree_h
#define uistratreftree_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          June 2007
 RCS:           $Id: uistratreftree.h,v 1.39 2012-09-07 22:08:02 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uistratmod.h"
#include "callback.h"
#include "ranges.h"
#include "stratunitref.h"

class ioPixmap;
class uiParent;
class uiTreeView;
class uiTreeViewItem;
namespace Strat {
    class RefTree;
    class NodeUnitRef;
}


/*!\brief Displays a Strat::RefTree */

mClass(uiStrat) uiStratRefTree : public CallBacker 
{
public:

			uiStratRefTree(uiParent*);
			~uiStratRefTree();

    void		setTree(Strat::RefTree&,bool force =false);
    
    const Strat::RefTree* tree() const 		{ return tree_; }

    uiTreeView*		treeView()		{ return lv_; }
    const uiTreeView*	treeView() const	{ return lv_; }
    uiTreeViewItem* 	getLVItFromFullCode(const char*) const;
    void                expand(bool) const;
    void                makeTreeEditable(bool) const;
    void		handleMenu(uiTreeViewItem*);
    void		updateUnitsPixmaps();
    void		updateLithoCol();
    void		moveUnit(bool);
    bool		canMoveUnit(bool);

    void		setEntranceDefaultTimes();
    bool		haveTimes() const;

protected:

    Strat::RefTree* 	tree_;

    uiTreeView*		lv_;

    void		rClickCB(CallBacker*);
    void		mousePressedCB(CallBacker*);

    void		insertSubUnit(uiTreeViewItem*);
    void		subdivideUnit(uiTreeViewItem*);
    void		updateUnitProperties(uiTreeViewItem*);
    void		removeUnit(uiTreeViewItem*);
    void		assignLevelBoundary(uiTreeViewItem*);
    bool		isLeaved(uiTreeViewItem*) const;

    void		setUnitLvl(const char*);

    BufferString	getFullCodeFromLVIt(const uiTreeViewItem*) const;
    void		insertUnitInLVIT(uiTreeViewItem*,int,
				    const Strat::UnitRef&) const; 

    void		addNode(uiTreeViewItem*,const Strat::NodeUnitRef&,bool);
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


    friend class 	uiStratDispToTree;
};


#endif

