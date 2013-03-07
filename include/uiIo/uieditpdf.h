#ifndef uieditpdf_h
#define uieditpdf_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2010
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uiiomod.h"
#include "uidialog.h"

class ProbDenFunc;
class uiGenInput;
class uiTable;
class uiTabStack;
class uiPDF1DViewWin;
class uiFlatViewMainWin;

/*!
\brief User interface to edit probability density function.
*/

mExpClass(uiIo) uiEditProbDenFunc : public uiDialog
{
public:
			uiEditProbDenFunc(uiParent*,ProbDenFunc&,bool editable);
			~uiEditProbDenFunc();

    bool		isChanged() const	{ return chgd_; }

protected:

    ProbDenFunc&		pdf_;
    const ProbDenFunc&		inpdf_;
    const bool			editable_;
    bool			chgd_;

    uiTabStack*			tabstack_;
    ObjectSet<uiGenInput>	nmflds_;
    uiTable*			tbl_;
    uiFlatViewMainWin*		vwwinnd_;
    uiPDF1DViewWin*		vwwin1d_;

    const int			nrdims_;
    int				curdim2_;

    void			mkTable(uiGroup*);

    bool			getNamesFromScreen();
    void			putValsToScreen();
    bool			getValsFromScreen(bool* chg=0);
    void			setToolTips();
    void			updateUI();

    void			viewPDF(CallBacker*);
    void			vwWinClose(CallBacker*);
    void			tabChg(CallBacker*);
    void			smoothReq(CallBacker*);
    void			dimNext(CallBacker*);
    void			dimPrev(CallBacker*);
    bool			acceptOK(CallBacker*);

};


#endif

