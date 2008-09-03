#ifndef uitabstack_h
#define uitabstack_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          17/01/2002
 RCS:           $Id: uitabstack.h,v 1.9 2008-09-03 14:53:45 cvskris Exp $
________________________________________________________________________

-*/

#include "uigroup.h"

class uiTabBar;


/* \brief Provides a way to create tabs with associated uiGroups.

    uiTabBar provides only the tabs. uiTabStack forms a system that
    combines such a tab-bar with uiGroups by which selection of a tab
    automatically makes the associated uiGroup visible.

    uiGroups that are to be added to the stack, *must* use the
    tabGroup() as a parent. Otherwise, you will get a programmer error
    message complaining that the added group "Must be child or
    child-of-child."
   
*/
class uiTabStack : public uiGroup
{
public:
			uiTabStack(uiParent*,const char* nm,
				   bool manage=true);

			// use this as parent for groups to add
    uiGroup*		tabGroup()			{ return tabgrp_; }

    void		addTab(uiGroup*,const char* txt=0);
    void		removeTab(uiGroup*);

    void		setTabEnabled(uiGroup*,bool);
    bool		isTabEnabled(uiGroup*) const;

    void		setCurrentPage(int id);
    void		setCurrentPage(uiGroup*);

    uiGroup*		currentPage() const;
    uiGroup* 		page(int id) const;
    int			idOf(uiGroup*) const;
    int			currentPageId() const;

    int			size() const;

    NotifierAccess&	selChange();

protected:

    uiTabBar*		tabbar_;
    uiGroup*		tabgrp_;

    void		tabSel(CallBacker* cb);
};


#endif
