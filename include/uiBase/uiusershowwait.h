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
#include "uistring.h"
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
			~uiUserShowWait()		{ readyNow(); }

    void		setMessage(const uiString&);
    void		readyNow();

protected:

    uiStatusBar*	sb_;
    MouseCursorChanger*	mcc_;
    const int		fldidx_;

};


/*!\brief will give feedback by setting mouse cursor and possibly status bar. */


mExpClass(uiBase) uiUSWTaskRunner : public TaskRunner
{
public:

			uiUSWTaskRunner( uiParent* p, const uiString& msg,
					 int statusbarfld=0 )
			    : usw_(p,msg,statusbarfld)		{}

    virtual bool	execute(Task&);

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

protected:

    uiParent*		parent_;
    uiString		msg_;
    const int		sbfld_;

};
