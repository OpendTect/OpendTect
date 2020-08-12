#pragma once

/*+
___________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Y.C. Liu 
 Date:		03-28-2007
 RCS:		$Id$
__________________________________________________________________________

-*/

#include "callback.h"

class uiParent;
class uiToolBar;

namespace VolProc
{

class ProcessingChain;

mClass(VolProcTest) Manager : public CallBacker
{
public:
    static Manager&	get(uiParent*);

    			Manager(uiParent*);
    			~Manager();

protected:
    void		buttonClickCB(CallBacker*);

    uiToolBar*		toolbar_;
    int			showsetupidx_;
    ProcessingChain&	chain_;
};


};

