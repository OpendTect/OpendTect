#ifndef uiseiswvltgen_h
#define uiseiswvltgen_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Mar 2009
 RCS:           $Id: uiseiswvltgen.h,v 1.2 2009-07-22 16:01:23 cvsbert Exp $
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
