#pragma once
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Satyaki Maitra
 Date:          June 2011
________________________________________________________________________

-*/

#include "uiiocommon.h"
#include "uidialog.h"

class uiDataPointSetCrossPlotter;
class uiTable;
class SelectionGrp;

mClass(uiIo) uiDPSSelGrpDlg : public uiDialog
{ mODTextTranslationClass(uiDPSSelGrpDlg);
public:

			uiDPSSelGrpDlg(uiDataPointSetCrossPlotter&,
				       const BufferStringSet&);
protected:

    int					curselgrp_;
    uiTable*				tbl_;
    uiDataPointSetCrossPlotter&		plotter_;
    ObjectSet<SelectionGrp>&		selgrps_;

    void				setCurSelGrp(CallBacker*);
    void				changeSelGrbNm(CallBacker*);
    void				addSelGrp(CallBacker*);
    void				importSelectionGrps(CallBacker*);
    void				exportSelectionGrps(CallBacker*);
    void				remSelGrp(CallBacker*);
    void				changeColCB(CallBacker*);
    void				calcSelectedness(CallBacker*);

    void				setColorCell(int,const Color&);

};
