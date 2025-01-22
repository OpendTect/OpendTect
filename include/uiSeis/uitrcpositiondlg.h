#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiseismod.h"

#include "datapackbase.h"
#include "multiid.h"
#include "pickretriever.h"
#include "trckeyzsampling.h"
#include "uidialog.h"
#include "uigroup.h"

class uiLabeledSpinBox;
class uiSpinBox;
class uiLabeledComboBox;
class uiToolButton;
class uiSlider;
class uiComboBox;
class uiGenInput;
class FlatDataPack;
namespace PosInfo { class Line2DData; }

class uiFlatDPPosSel : public uiGroup
{
public:
				uiFlatDPPosSel(uiParent*,
					       const DataPack::FullID&);
				~uiFlatDPPosSel();

    double			getPos() const;

private:

    uiSlider*			possldr_	= nullptr;
    uiComboBox*			altdimnmflds_	= nullptr;
    uiGenInput*			posvalfld_	= nullptr;
    RefMan<FlatDataPack>	fdp_;

    void			initGrp(CallBacker*);
    void			sldrPosChangedCB(CallBacker*);
};

mExpClass(uiSeis) uiTrcPositionDlg : public uiDialog
{  mODTextTranslationClass(uiTrcPositionDlg);
public:
				uiTrcPositionDlg(uiParent*,
						 const TrcKeyZSampling&,
						 bool,const MultiID&);
				uiTrcPositionDlg(uiParent*,
						 const DataPack::FullID&);
				~uiTrcPositionDlg();

    TrcKeyZSampling		getTrcKeyZSampling() const;
    Pos::GeomID			getGeomID() const;

    uiLabeledSpinBox*		trcnrfld_			= nullptr;
    uiLabeledSpinBox*		inlfld_				= nullptr;
    uiSpinBox*			crlfld_				= nullptr;
    uiLabeledComboBox*		linesfld_			= nullptr;
    uiFlatDPPosSel*		fdpposfld_			= nullptr;
    MultiID			mid_;

private:

    void			initDlg(CallBacker*);
    void			lineSel(CallBacker*);
    void			getPosCB(CallBacker*);
    void			pickRetrievedCB(CallBacker*);
    bool			getSelLineGeom(PosInfo::Line2DData&);

    StepInterval<float>		zrg_;
    uiToolButton*		getposbut_			= nullptr;
    WeakPtr<PickRetriever>	pickretriever_;
};
