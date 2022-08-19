#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

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
