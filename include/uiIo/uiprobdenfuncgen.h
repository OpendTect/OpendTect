#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiiomod.h"
#include "uidialog.h"
#include "multiid.h"

class MnemonicSelection;
class uiCheckList;
class uiMnemonicsSel;
class uiSpinBox;


/*!\brief creates probability density functions (Gaussian and Sampled) */

mExpClass(uiIo) uiProbDenFuncGen : public uiDialog
{ mODTextTranslationClass(uiProbDenFuncGen);
public:
			uiProbDenFuncGen(uiParent*,
					 const MnemonicSelection* =nullptr,
					 const BufferStringSet* nms=nullptr,
					 int defidx=0);
			~uiProbDenFuncGen();

    MultiID		newObjKey() const	{ return ioobjky_; }

protected:

    uiCheckList*	choicefld_;
    uiSpinBox*		nrdimfld_;
    ObjectSet<uiMnemonicsSel> mnsels_;
    const int		defidx_;
    int			defmaxnrdims_ = 3;

    MultiID		ioobjky_;

    void		initDlg(CallBacker*);
    void		choiceSel(CallBacker*);
    void		nrDimsChgCB(CallBacker*);
    bool		acceptOK(CallBacker*) override;

};
