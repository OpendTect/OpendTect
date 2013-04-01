#ifndef uivolprocbatchsetup_h
#define uivolprocbatchsetup_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          June 2001
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uivolumeprocessingmod.h"
#include "uibatchlaunch.h"

class CtxtIOObj;
class IOObj;
class IOPar;

class uiGenInput;
class uiIOObjSel;
class uiPosSubSel;
class uiVelocityDesc;


namespace VolProc 
{

class Chain;

mExpClass(uiVolumeProcessing) uiBatchSetup : public uiFullBatchDialog
{

public:
                        uiBatchSetup(uiParent*,const IOObj* setupsel=0);
                        ~uiBatchSetup();

protected:

    bool		prepareProcessing();
    bool		fillPar(IOPar&);
    bool		retrieveChain();

    uiIOObjSel*		setupsel_;
    uiPushButton*	editsetup_;
    uiPosSubSel*	possubsel_;
    uiIOObjSel*		outputsel_;
    Chain*		chain_;

    void		setupSelCB(CallBacker*);
    void		editPushCB(CallBacker*);
};

} // namespace VolProc

#endif

