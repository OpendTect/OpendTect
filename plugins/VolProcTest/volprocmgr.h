#ifndef volprocmgr_h
#define volprocmgr_h

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

class Manager : public CallBacker
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

#endif
