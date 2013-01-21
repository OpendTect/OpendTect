#ifndef uidlggroup_h
#define uidlggroup_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          13/8/2000
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "uidialog.h"
#include "uigroup.h"
#include "uitabstack.h"


/*!
Dialog that either can be used standalone (with uiSingleGroupDlg) or
in a tabstack (uiTabStackDlg) */


mExpClass(uiTools) uiDlgGroup : public uiGroup
{
public:
    			uiDlgGroup(uiParent* p,const char* nm)
			    : uiGroup( p, nm )		{}

    virtual bool	acceptOK()			{ return true; }
    			/*!<Commit changes. Return true if success. */
    virtual bool	rejectOK()			{ return true; }
    			/*!<Revert eventual changes allready done, so the
			    object is in it's original state. */
    virtual bool	revertChanges()			{ return true; }
    			/*!<Revert eventual changes allready done, so the
			    object is in it's original state. The difference
			    betweeen revertChanges() and rejectOK() is that
			    revertChanges() may be called after a successful
			    acceptOK() call, if another tab in the stack fails.
			*/
    virtual const char*	errMsg() const			{ return 0; }

    virtual const char*	helpID() const			{ return 0; }
    			
};

/*! Dialog with one uiDlgGroup. */

mExpClass(uiTools) uiSingleGroupDlg : public uiDialog
{
public:
		uiSingleGroupDlg( uiParent* p,const uiDialog::Setup& st )
		    : uiDialog(p,st)
		    , grp_(0)			{}
    void	setGroup( uiDlgGroup* grp )	{ grp_ = grp; }


    const char*	helpID() const
			{ const char* hid = grp_->helpID();
			  if ( !hid ) hid = uiDialog::helpID(); return hid; }

protected:
    bool	acceptOK(CallBacker*)		{ return grp_->acceptOK(); }
    bool	rejectOK(CallBacker*)		{ return grp_->rejectOK(); }
	
    uiDlgGroup*	grp_;
};


/*! Dialog with multiple uiDlgGroup in a tabstack. */
mExpClass(uiTools) uiTabStackDlg : public uiDialog
{
public:
			uiTabStackDlg(uiParent*,const uiDialog::Setup&);
			~uiTabStackDlg();

    uiParent*		tabParent();
    uiObject*		tabObject()		{ return (uiObject*)tabstack_; }
    void		addGroup(uiDlgGroup*);

    int			nrGroups() const	{ return groups_.size(); }
    uiDlgGroup&		getGroup(int idx)	{ return *groups_[idx]; }
    const uiDlgGroup&	getGroup(int idx) const { return *groups_[idx]; }
    void		showGroup(int idx);
    int			currentGroupID()	
    			{ return tabstack_->currentPageId(); }

    const char*		helpID() const;
    			/*!<Returns the help id for the current group, or 0 if
			    no help available for any group, "" if there is
			    help for one or more groups, but not the currently
			    seleceted one. */
			

protected:

    void			selChange(CallBacker*);

    virtual bool		acceptOK(CallBacker*);
    virtual bool		rejectOK(CallBacker*);

    bool 			canrevert_;
    ObjectSet<uiDlgGroup>	groups_;
    uiTabStack*			tabstack_;
};

#endif

