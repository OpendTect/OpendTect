#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nageswara
 Date:          April 2009
________________________________________________________________________

-*/

#include "uiseismod.h"
#include "uidialog.h"

class IOObj;
namespace Seis { class SelData; class TableSelData; }
class uiGenInput;
class uiIOObjSel;
class uiWaveletIOObjSel;
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
    DBKey			storeKey() const;
    bool			is2D() const;

    Notifier<uiWaveletExtraction> extractionDone;

protected:

    void			createCommonUIFlds();
    bool			checkWaveletSize();
    bool			check2DFlds();
    bool			acceptOK();
    void			choiceSelCB(CallBacker*);
    void			inputSelCB(CallBacker*);
    void			lineSelCB(CallBacker*);
    bool			doProcess(const IOObj&,const IOPar&,
					  const IOPar&);
    bool                        get3DSelData(const IOPar&,const IOPar&);
    bool			fillHorizonSelData(const IOPar&,const IOPar&,
						   Seis::TableSelData&);

    uiGenInput*			zextraction_;
    uiGenInput*			wtlengthfld_;
    uiGenInput*			wvltphasefld_;
    uiGenInput*			taperfld_;
    uiWaveletIOObjSel*		outputwvltfld_;
    uiPosProvGroup*		surfacesel_;
    uiSeisSel*			seisselfld_;
    uiSeis2DMultiLineSel*	linesel2dfld_;
    uiSeis3DSubSel*		subselfld3d_;
    uiSelZRange*		zrangefld_;
    Seis::SelData*		seldata_;

    Interval<float>		relzwin_;
    float			datastep_;
    int				wvltsize_;

};
