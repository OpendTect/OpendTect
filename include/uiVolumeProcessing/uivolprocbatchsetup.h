#ifndef uivolprocbatchsetup_h
#define uivolprocbatchsetup_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          June 2001
 RCS:           $Id: uivolprocbatchsetup.h,v 1.7 2012-08-03 13:01:19 cvskris Exp $
________________________________________________________________________

-*/

#include "uivolumeprocessingmod.h"
#include "uibatchlaunch.h"
class IOObj;
class IOPar;
class CtxtIOObj;
class uiIOObjSel;
class uiPosSubSel;
class uiGenInput;
class uiVelocityDesc;


namespace VolProc 
{

class Chain;

mClass(uiVolumeProcessing) uiBatchSetup : public uiFullBatchDialog
{

public:
                        uiBatchSetup(uiParent*, const IOObj* setupsel = 0 );
                        ~uiBatchSetup();

protected:

    bool		prepareProcessing();
    bool		fillPar(IOPar&);

    uiIOObjSel*		setupsel_;
    uiPushButton*	editsetup_;
    uiPosSubSel*	possubsel_;
    uiIOObjSel*		outputsel_;
    Chain*		chain_;

    void		setupSelCB(CallBacker*);
    void		editPushCB(CallBacker*);
};

}; //namespace

#endif

