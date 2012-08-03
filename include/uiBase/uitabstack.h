#ifndef uitabstack_h
#define uitabstack_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          17/01/2002
 RCS:           $Id: uitabstack.h,v 1.13 2012-08-03 13:00:54 cvskris Exp $
________________________________________________________________________

-*/

#include "uibasemod.h"
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
mClass(uiBase) uiTabStack : public uiGroup
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
    uiGroup* 		page(int idx) const;
    int			indexOf(uiGroup*) const;
    int			currentPageId() const;

    int			size() const;

    NotifierAccess&	selChange();

protected:

    uiTabBar*		tabbar_;
    uiGroup*		tabgrp_;

    void		tabSel(CallBacker* cb);
};


#endif

