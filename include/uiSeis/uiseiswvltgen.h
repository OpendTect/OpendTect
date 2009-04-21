#ifndef uiseiswvltgen_h
#define uiseiswvltgen_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Mar 2009
 RCS:           $Id: uiseiswvltgen.h,v 1.1 2009-04-21 13:55:59 cvsbruno Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
#include "multiid.h"
class CtxtIOObj;
class uiGenInput;
class uiIOObjSel;


mClass uiSeisWvltGen : public uiDialog
{
public:
			uiSeisWvltGen(uiParent*);
			~uiSeisWvltGen();

    MultiID		storeKey() const;

protected:

    bool		acceptOK(CallBacker*);

    CtxtIOObj&		ctio_;
    uiGenInput*		isrickfld_;
    uiGenInput*		freqfld_;
    uiGenInput*		srfld_;
    uiGenInput*		peakamplfld_;
    uiIOObjSel*		wvltfld_;

};


#endif
