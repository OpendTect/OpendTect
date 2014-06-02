#ifndef uidpsrefineseldlg_h
#define uidpsrefineseldlg_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Satyaki Maitra
 Date:          June 2011
 RCS:           $Id$:
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
				Math::Expression* mathObject() { 
                                return mathobj_; }

protected:

    int				cColIds(int dcolid);
    void			updateDisplay();
    void			setPlotter();

    void			checkMathExpr(CallBacker*);
    void			parsePush(CallBacker*);
    bool			acceptOK(CallBacker*);

    uiDataPointSetCrossPlotter&	plotter_;
    BufferString		mathexprstring_;
    BufferStringSet		colnms_;
    Math::Expression*		mathobj_;
    TypeSet<int>		dcolids_;

    uiGenInput*			inpfld_;
    uiPushButton*		setbut_;
    uiTable*			vartable_;
};

#endif

