#ifndef volprocmgr_h
#define volprocmgr_h

/*+
___________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Y.C. Liu 
 Date:		03-28-2007
 RCS:		$Id: volprocmgr.h,v 1.1 2007-03-30 21:00:56 cvsyuancheng Exp $
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
