#ifndef uitrcpositiondlg_h
#define uitrcpositiondlg_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        H. Huck
 Date:          July 2010
 RCS:           $Id: uitrcpositiondlg.h,v 1.6 2012-08-03 13:01:09 cvskris Exp $
________________________________________________________________________

-*/

#include "uiseismod.h"
#include "uidialog.h"
#include "cubesampling.h"
#include "linekey.h"
#include "multiid.h"

class uiLabeledSpinBox;
class uiSpinBox;
class uiLabeledComboBox;
class uiToolButton;
class PickRetriever;
namespace PosInfo { class Line2DData; }

mClass(uiSeis) uiTrcPositionDlg: public uiDialog
{                                                                               
public:                                                                         
				uiTrcPositionDlg(uiParent*,const CubeSampling&,
			      			 bool,const MultiID&);
				~uiTrcPositionDlg();

    CubeSampling		getCubeSampling() const;
    LineKey			getLineKey() const;
    uiLabeledSpinBox*		trcnrfld_;
    uiLabeledSpinBox*		inlfld_;
    uiSpinBox*			crlfld_;
    uiLabeledComboBox*		linesfld_;
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

#endif

