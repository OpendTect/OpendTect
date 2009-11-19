/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nageswara
 Date:          April 2009
 RCS:		$Id: uiwaveletextraction.h,v 1.11 2009-11-19 10:21:20 cvsnageswara Exp $
________________________________________________________________________

-*/

#include "uidialog.h"

class uiGenInput;
class uiIOObjSel;
class uiIOObjSelGrp;
class uiPosProvGroup;
class uiSeisSel;
class uiSelection2DParSel;
class uiSeis3DSubSel;
class uiSelZRange;
class CtxtIOObj;
class IOPar;
class MultiID;
namespace Seis { class SelData; class TableSelData; }

mClass uiWaveletExtraction : public uiDialog
{
public:
				uiWaveletExtraction(uiParent*,bool is2d=false);
				~uiWaveletExtraction();
    MultiID			storeKey() const;

    Notifier<uiWaveletExtraction>	extractionDone;

protected:

    void			createUI();
    bool			checkWaveletSize();
    bool			check2DFlds();
    bool			acceptOK(CallBacker*);
    void			choiceSelCB(CallBacker*);
    void			inputSelCB(CallBacker*);
    bool			doProcess(const IOPar&,const IOPar&);
    bool                        readInputData(const IOPar&,const IOPar&);
    bool			fillHorizonSelData(const IOPar&,const IOPar&,
						   Seis::TableSelData&);

    CtxtIOObj&			seisctio_;
    CtxtIOObj&			wvltctio_;
    uiGenInput*			zextraction_;
    uiGenInput*			wtlengthfld_;
    uiGenInput*			wvltphasefld_;
    uiGenInput*			taperfld_;
    uiIOObjSel*			outputwvltfld_;
    uiPosProvGroup* 		surfacesel_;
    uiSeisSel*			seisselfld_;
    uiSelZRange*		zrangefld_;
    uiSeis3DSubSel*		subselfld3d_;
    uiSelection2DParSel*	linesel2dpar_;
    Seis::SelData*		sd_;

    int				wvltsize_;
    bool			is2d_;
};
