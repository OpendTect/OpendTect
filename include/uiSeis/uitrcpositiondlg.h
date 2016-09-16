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
#include "dbkey.h"
#include "datapackbase.h"

class uiLabeledSpinBox;
class uiSpinBox;
class uiSeis2DLineNameSel;
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
				uiTrcPositionDlg(uiParent*,const TrcKeyZSampling&,
						 bool,const DBKey&);
				~uiTrcPositionDlg();

    TrcKeyZSampling		getTrcKeyZSampling() const;
    Pos::GeomID			geomID() const;
    void			setGeomID(Pos::GeomID);

    uiLabeledSpinBox*		trcnrfld_;
    uiLabeledSpinBox*		inlfld_;
    uiSpinBox*			crlfld_;
    uiSeis2DLineNameSel*	linesfld_;
    uiFlatDPPosSel*		fdpposfld_;
    DBKey			mid_;

protected:
    void			lineSel(CallBacker*);
    void			getPosCB(CallBacker*);
    void			pickRetrievedCB(CallBacker*);

    StepInterval<float>		zrg_;
    uiToolButton*		getposbut_;
    PickRetriever*		pickretriever_;
};
