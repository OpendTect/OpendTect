#ifndef uieditpdf_h
#define uieditpdf_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2010
 RCS:           $Id: uieditpdf.h,v 1.7 2010-03-04 15:29:18 cvsbert Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
class ProbDenFunc;
class uiGenInput;
class uiTable;
class uiTabStack;
class uiFlatViewMainWin;


/*! \brief Edit Probability Density Function */

mClass uiEditProbDenFunc : public uiDialog
{
public:
			uiEditProbDenFunc(uiParent*,ProbDenFunc&,bool editable);
			~uiEditProbDenFunc();

    bool		isChanged() const	{ return chgd_; }

protected:

    const ProbDenFunc&		pdf_;
    ProbDenFunc*		workpdf_;
    const bool			editable_;
    bool			chgd_;

    uiTabStack*			tabstack_;
    ObjectSet<uiGenInput>	nmflds_;
    uiTable*			tbl_;
    uiFlatViewMainWin*		flatvwwin_;
    int				curdim2_;

    void			mkTable(uiGroup*);

    void			putValsToScreen();
    bool			getValsFromScreen(bool* chg=0);

    void			viewPDF(CallBacker*);
    void			smoothReq(CallBacker*);
    void			dimNext(CallBacker*);
    void			dimPrev(CallBacker*);
    void			vwWinClose(CallBacker*);
    bool			acceptOK(CallBacker*);

};


#endif
