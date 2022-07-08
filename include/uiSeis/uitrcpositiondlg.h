#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        H. Huck
 Date:          July 2010
________________________________________________________________________

-*/

#include "uiseismod.h"
#include "uidialog.h"
#include "uigroup.h"
#include "trckeyzsampling.h"
#include "linekey.h"
#include "multiid.h"
#include "datapackbase.h"

class uiLabeledSpinBox;
class uiSpinBox;
class uiLabeledComboBox;
class uiToolButton;
class uiSlider;
class uiComboBox;
class uiGenInput;
class PickRetriever;
class FlatDataPack;
namespace PosInfo { class Line2DData; }

class uiFlatDPPosSel : public uiGroup
{
public:
				uiFlatDPPosSel(uiParent*,
					       const DataPack::FullID&);
				~uiFlatDPPosSel();
    double			getPos() const;
protected:
    uiSlider*			possldr_;
    uiComboBox*			altdimnmflds_;
    uiGenInput*			posvalfld_;
    RefMan<FlatDataPack>	fdp_;

    void			sldrPosChangedCB(CallBacker*);
};

mExpClass(uiSeis) uiTrcPositionDlg: public uiDialog
{  mODTextTranslationClass(uiTrcPositionDlg);
public:
				uiTrcPositionDlg(uiParent*,
						 const DataPack::FullID&);
				uiTrcPositionDlg(uiParent*,
						 const TrcKeyZSampling&,
			      			 bool,const MultiID&);
				~uiTrcPositionDlg();

    TrcKeyZSampling		getTrcKeyZSampling() const;
    LineKey			getLineKey() const;
    uiLabeledSpinBox*		trcnrfld_;
    uiLabeledSpinBox*		inlfld_;
    uiSpinBox*			crlfld_;
    uiLabeledComboBox*		linesfld_;
    uiFlatDPPosSel*		fdpposfld_;
    MultiID			mid_;

protected:
    void			lineSel(CallBacker*);
    void			getPosCB(CallBacker*);
    void			pickRetrievedCB(CallBacker*);
    bool			getSelLineGeom(PosInfo::Line2DData&);

    StepInterval<float>		zrg_;
    uiToolButton*		getposbut_;
    PickRetriever*		pickretriever_;
};

