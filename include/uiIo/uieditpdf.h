#ifndef uieditpdf_h
#define uieditpdf_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2010
 RCS:           $Id: uieditpdf.h,v 1.6 2010-03-02 15:39:39 cvsbert Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
class ProbDenFunc;
class uiGenInput;
class uiTable;
class uiTabStack;
class uiFlatViewMainWin;
template <class T> class ArrayND;
template <class T> class Array1D;


/*! \brief Edit Probability Density Function */

mClass uiEditProbDenFunc : public uiDialog
{
public:
			uiEditProbDenFunc(uiParent*,ProbDenFunc&,bool editable);
			~uiEditProbDenFunc();

    bool		isChanged() const	{ return chgd_; }

protected:

    ProbDenFunc&		pdf_;
    const bool			editable_;
    bool			chgd_;

    uiTabStack*			tabstack_;
    ObjectSet<uiGenInput>	nmflds_;
    ObjectSet<uiTable>		tbls_;
    uiFlatViewMainWin*		flatvwwin_;

    void			putToScreen(const ArrayND<float>&);
    bool			getFromScreen(ArrayND<float>&,bool* chg=0);

    void			viewPDF(CallBacker*);
    void			smoothReq(CallBacker*);
    void			vwWinClose(CallBacker*);
    bool			acceptOK(CallBacker*);

};


#endif
