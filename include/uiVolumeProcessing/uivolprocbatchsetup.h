#ifndef uivolprocbatchsetup_h
#define uivolprocbatchsetup_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          June 2001
 RCS:           $Id: uivolprocbatchsetup.h,v 1.4 2009-03-19 16:12:28 cvsbert Exp $
________________________________________________________________________

-*/

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
    uiGenInput*		outisvelfld_;
    uiVelocityDesc*	uiveldesc_;

    void		outTypChg(CallBacker*);
    void		outSel(CallBacker*);

};

}; //namespace

#endif
