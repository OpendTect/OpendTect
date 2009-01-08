#ifndef uivolprocbatchsetup_h
#define uivolprocbatchsetup_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          June 2001
 RCS:           $Id: uivolprocbatchsetup.h,v 1.2 2009-01-08 09:00:11 cvsranojay Exp $
________________________________________________________________________

-*/

#include "uibatchlaunch.h"

class IOPar;
class uiIOObjSel;
class uiPosSubSel;
class CtxtIOObj;

namespace VolProc 
{

mClass uiBatchSetup : public uiFullBatchDialog
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
