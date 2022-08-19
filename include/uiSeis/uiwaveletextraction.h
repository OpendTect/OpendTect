#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiseismod.h"
#include "uidialog.h"

class IOObj;
namespace Seis { class SelData; class TableSelData; }
class uiGenInput;
class uiIOObjSel;
class uiPosProvGroup;
class uiSeisSel;
class uiSeis2DMultiLineSel;
class uiSeis3DSubSel;
class uiSelZRange;


mExpClass(uiSeis) uiWaveletExtraction : public uiDialog
{ mODTextTranslationClass(uiWaveletExtraction)
public:
				uiWaveletExtraction(uiParent*,bool is2d);
				~uiWaveletExtraction();
    MultiID			storeKey() const;

    Notifier<uiWaveletExtraction>	extractionDone;

protected:

    void			createCommonUIFlds();
    bool			checkWaveletSize();
    bool			check2DFlds();
    bool			acceptOK(CallBacker*) override;
    void			choiceSelCB(CallBacker*);
    void			inputSelCB(CallBacker*);
    void			lineSelCB(CallBacker*);
    bool			doProcess(const IOObj&,const IOObj&,
					  const IOPar&,const IOPar&);
    bool                        getSelData(const IOPar&,const IOPar&);
    bool			fillHorizonSelData(const IOPar&,const IOPar&,
						   Seis::TableSelData&);

    uiGenInput*			zextraction_;
    uiGenInput*			wtlengthfld_;
    uiGenInput*			wvltphasefld_;
    uiGenInput*			taperfld_;
    uiIOObjSel*			outputwvltfld_;
    uiPosProvGroup*		surfacesel_;
    uiSeisSel*			seisselfld_;
    uiSeis2DMultiLineSel*	linesel2dfld_;
    uiSeis3DSubSel*		subselfld3d_;
    uiSelZRange*		zrangefld_;
    Seis::SelData*		seldata_;

    float			datastep_;
    int				wvltsize_;
};
