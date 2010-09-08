#ifndef uistratreftree_h
#define uistratreftree_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          June 2007
 RCS:           $Id: uistratreftree.h,v 1.28 2010-09-08 13:27:39 cvsbruno Exp $
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
    class UnitRepository;
}


/*!\brief Displays a Strat::RefTree */

mClass uiStratRefTree : public CallBacker 
{
public:

			uiStratRefTree(uiParent*,Strat::UnitRepository&);
			~uiStratRefTree();

    void		setTree(const Strat::RefTree*,bool force =false);

    uiListView*		listView()		{ return lv_; }
    const uiListView*	listView() const	{ return lv_; }
    void                expand(bool) const;
    void                makeTreeEditable(bool) const;
    void		handleMenu(uiListViewItem*);
    void		updateUnitsPixmaps();
    void		updateLithoCol();
    void		moveUnit(bool);
    bool		canMoveUnit(bool);
    void		setBottomLvl();

protected:

    const Strat::RefTree* tree_;

    uiListView*		lv_;
    Strat::UnitRepository& repos_;

    void		rClickCB(CallBacker*);
    void		repoChangedCB(CallBacker*);

    void		insertSubUnit(uiListViewItem*);
    void		subdivideUnit(uiListViewItem*);
    Strat::UnitRef*	createNewUnit(const char* code,int parid) const; 
    Strat::UnitRef*	getUnitFromLVIT(uiListViewItem*) const; 
    void		insertUnitInLVIT(uiListViewItem*,
	    				const Strat::UnitRef&) const; 
    void		makeNewTreeItem(uiListViewItem*);
    void		removeUnit(uiListViewItem*);
    void		updateUnitProperties(uiListViewItem*);

    void		setUnitLvl(int);

    void 		setUnconformities(const Strat::NodeUnitRef&,bool,int);
    void 		doSetUnconformities(CallBacker*);

    void		addNode(uiListViewItem*,const Strat::NodeUnitRef&,bool);
    ioPixmap*		createUnitPixmap(const Color& col) const;
    			//becomes yours!

    BufferString	getCodeFromLVIt(const uiListViewItem*) const;

    friend class 	uiStratDispToTreeTransl;
};


#endif
