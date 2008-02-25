#ifndef uivolprocbatchsetup_h
#define uivolprocbatchsetup_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          June 2001
 RCS:           $Id: uivolprocbatchsetup.h,v 1.1 2008-02-25 19:14:54 cvskris Exp $
________________________________________________________________________

-*/

#include "uibatchlaunch.h"

class IOPar;
class uiIOObjSel;
class uiPosSubSel;
class CtxtIOObj;

namespace VolProc 
{

class uiBatchSetup : public uiFullBatchDialog
{

public:
                        uiBatchSetup(uiParent*,const IOPar* extraomf);
                        ~uiBatchSetup();

private:
    bool		prepareProcessing();
    bool		fillPar(IOPar&);

    const IOPar*	extraomf_;
    CtxtIOObj&		setupctxt_;
    uiIOObjSel*		setupsel_;
    uiPosSubSel*	possubsel_;
    uiIOObjSel*		outputsel_;
    CtxtIOObj&		outputctxt_;
};

}; //namespace

#endif
