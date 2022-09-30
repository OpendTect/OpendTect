#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiiomod.h"
#include "uidialog.h"
#include "bufstringset.h"

class uiDataPointSetCrossPlotter;

class uiGenInput;
class uiPushButton;
class uiTable;
namespace Math { class Expression; }


mExpClass(uiIo) uiDPSRefineSelDlg : public uiDialog
{ mODTextTranslationClass(uiDPSRefineSelDlg);
public:
				uiDPSRefineSelDlg(uiDataPointSetCrossPlotter&);
				~uiDPSRefineSelDlg();

    Math::Expression*		mathObject()
				{  return mathobj_; }

protected:

    int				cColIds(int dcolid);
    void			updateDisplay();
    void			setPlotter();

    void			checkMathExpr(CallBacker*);
    void			parsePush(CallBacker*);
    bool			acceptOK(CallBacker*) override;

    uiDataPointSetCrossPlotter& plotter_;
    BufferString		mathexprstring_;
    BufferStringSet		colnms_;
    Math::Expression*		mathobj_;
    TypeSet<int>		dcolids_;

    uiGenInput*			inpfld_;
    uiPushButton*		setbut_;
    uiTable*			vartable_;
};
