#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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
			uiDlgGroup(uiParent*,const uiString& caption);
			~uiDlgGroup();

    void		setCaption( const uiString& c ) { caption_ = c; }
    const uiString&	getCaption() const		{ return caption_; }

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
    virtual const char* errMsg() const			{ return 0; }

    virtual HelpKey	helpKey() const			{ return mNoHelpKey; }

protected:
    uiString		caption_;

};

/*! Dialog with one uiDlgGroup. */

mExpClass(uiTools) uiSingleGroupDlg : public uiDialog
{
public:
			uiSingleGroupDlg(uiParent*,const uiDialog::Setup&);
			~uiSingleGroupDlg();

    void		setGroup( uiDlgGroup* grp )	{ grp_ = grp; }

    HelpKey		helpKey() const override;

protected:
    bool		acceptOK(CallBacker*) override
			{ return grp_->acceptOK(); }
    bool		rejectOK(CallBacker*) override
			{ return grp_->rejectOK(); }

    uiDlgGroup*		grp_	= nullptr;
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

    HelpKey		helpKey() const override;
    			/*!<Returns the help id for the current group, or 0 if
			    no help available for any group, "" if there is
			    help for one or more groups, but not the currently
			    seleceted one. */


protected:

    void			selChange(CallBacker*);

    virtual bool		acceptOK(CallBacker*) override;
    virtual bool		rejectOK(CallBacker*) override;

    bool 			canrevert_;
    ObjectSet<uiDlgGroup>	groups_;
    uiTabStack*			tabstack_;
};
