#ifndef uistratreftree_h
#define uistratreftree_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          June 2007
 RCS:           $Id: uistratreftree.h,v 1.5 2007-08-15 15:01:00 cvshelene Exp $
________________________________________________________________________

-*/

class uiParent;
class uiListView;
class uiListViewItem;
namespace Strat {
    class RefTree;
    class NodeUnitRef;
    class UnitRef;
}


/*!\brief Displays a Strat::RefTree */

class uiStratRefTree 
{
public:

			uiStratRefTree(uiParent*,const Strat::RefTree*);
			~uiStratRefTree();

    void		setTree(const Strat::RefTree*,bool force =false);

    uiListView*		listView()		{ return lv_; }
    const uiListView*	listView() const	{ return lv_; }
    void                expand(bool) const;
    void                makeTreeEditable(bool) const;

    const Strat::UnitRef*	findUnit(const char*) const;

protected:

    const Strat::RefTree* tree_;

    uiListView*		lv_;

    void		addNode(uiListViewItem*,const Strat::NodeUnitRef&,bool);

};


#endif
