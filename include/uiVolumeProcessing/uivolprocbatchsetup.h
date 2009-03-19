#ifndef uivolprocbatchsetup_h
#define uivolprocbatchsetup_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          June 2001
 RCS:           $Id: uivolprocbatchsetup.h,v 1.3 2009-03-19 13:27:11 cvsbert Exp $
________________________________________________________________________

-*/

#include "uibatchlaunch.h"
class IOObj;
class IOPar;
class CtxtIOObj;
class uiIOObjSel;
class uiPosSubSel;
class uiGenInput;


namespace VolProc 
{

mClass uiBatchSetup : public uiFullBatchDialog
{

public:
                        uiBatchSetup(uiParent*,const IOPar* extraomf,
				     const IOObj* initial=0);
                        ~uiBatchSetup();

protected:

    bool		prepareProcessing();
    bool		fillPar(IOPar&);

    const IOPar*	extraomf_;
    CtxtIOObj&		setupctxt_;
    CtxtIOObj&		outputctxt_;

    uiIOObjSel*		setupsel_;
    uiPosSubSel*	possubsel_;
    uiIOObjSel*		outputsel_;
    uiGenInput*		outputtypefld_;

};

}; //namespace

#endif
