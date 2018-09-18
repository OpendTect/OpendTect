#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          December 2007
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "taskrunner.h"

class uiTaskRunner;
class uiParent;


mExpClass(uiTools) uiTaskRunnerProvider : public TaskRunnerProvider
{
public:

			uiTaskRunnerProvider(const uiParent*);

    virtual TaskRunner&	runner() const;

    virtual void	emitErrorMessage(const uiString&,bool wrn=false) const;

protected:

    uiParent*		parent_;

};
