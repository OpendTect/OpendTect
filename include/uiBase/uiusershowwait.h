#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Sep 2017
________________________________________________________________________

-*/

#include "uibasemod.h"
#include "taskrunner.h"
#include "uistringset.h"
class uiParent;
class uiStatusBar;
class MouseCursorChanger;


/*!\brief tells user something is happening.

  Sets mouse cursor and puts something in status bar (if available). Will
  automatically clean up on destruction.

  Useful when something may take some time but there is no progress or when you
  can't put it in an Executor.

*/

mExpClass(uiBase) uiUserShowWait
{
public:

			uiUserShowWait(const uiParent*,const uiString&,
					int statusbarfld=0);
			~uiUserShowWait();

    void		setMessage(const uiString&);
    void		readyNow();
    const uiParent*	parent() const			{ return parent_; }
    uiStatusBar*	statusBar()			{ return sb_; }

protected:

    const uiParent*	parent_;
    uiStatusBar*	sb_;
    MouseCursorChanger*	mcc_;
    const int		fldidx_;
    uiStringSet		prevmessages_;

};


/*!\brief will give feedback by setting mouse cursor and possibly status bar. */


mExpClass(uiBase) uiUSWTaskRunner : public TaskRunner
{
public:

			uiUSWTaskRunner( uiParent* p, const uiString& msg,
					 int statusbarfld=0 )
			    : usw_(p,msg,statusbarfld)		{}

    virtual bool	execute(Task&);
    virtual void	emitErrorMessage(const uiString&,bool) const;

protected:

    uiUserShowWait	usw_;

};



/*!\brief if feedback is needed, will give it through mouse cursor and
  possibly status bar. */


mExpClass(uiBase) uiUSWTaskRunnerProvider : public TaskRunnerProvider
{
public:

			uiUSWTaskRunnerProvider( uiParent* p, uiString s,
						 int sbfld=0 )
			    : parent_(p), msg_(s), sbfld_(sbfld)	{}

    virtual TaskRunner&	runner() const;
    virtual void	emitErrorMessage(const uiString&,bool) const;

protected:

    uiParent*		parent_;
    uiString		msg_;
    const int		sbfld_;

};
