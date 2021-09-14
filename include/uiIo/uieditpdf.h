#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2010
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uiiomod.h"
#include "uigroup.h"
#include "uidialog.h"

class ProbDenFunc;
class uiGenInput;
class uiTable;
class uiTabStack;
class uiPDF1DViewWin;
class uiFlatViewMainWin;
class uiComboBox;
class uiPushButton;
class uiListBox;
class uiToolButton;
class Gaussian1DProbDenFunc;
class Gaussian2DProbDenFunc;
class GaussianNDProbDenFunc;


/*!\brief Base class for edit probability density function editors. */

mExpClass(uiIo) uiEditProbDenFunc : public uiGroup
{ mODTextTranslationClass(uiEditProbDenFunc);
public:
			uiEditProbDenFunc(uiParent*,ProbDenFunc&,bool editable);

    virtual bool	commitChanges()			= 0;
    inline bool		isChanged() const		{ return chgd_; }

protected:

    ProbDenFunc&	pdf_;
    const ProbDenFunc&	inpdf_;
    const int		nrdims_;
    const bool		editable_;
    bool		chgd_;

public:

    void		cleanup();

};


/*!\brief Dialog to edit probability density functions. */

mExpClass(uiIo) uiEditProbDenFuncDlg : public uiDialog
{ mODTextTranslationClass(uiEditProbDenFuncDlg);
public:
			uiEditProbDenFuncDlg(uiParent*,ProbDenFunc&,bool edit,
						bool isnew=false);

    bool		isChanged() const	{ return edfld_->isChanged(); }

protected:

    uiEditProbDenFunc*	edfld_;

    bool		acceptOK(CallBacker*);

public:

    void		cleanup();

};


/*!\brief Group to edit SampledProbDenFunc's. */

mExpClass(uiIo) uiEditSampledProbDenFunc : public uiEditProbDenFunc
{ mODTextTranslationClass(uiEditSampledProbDenFunc);
public:
			uiEditSampledProbDenFunc(uiParent*,ProbDenFunc&,bool);
			~uiEditSampledProbDenFunc();

    virtual bool	commitChanges();

protected:

    int			curdim2_;

    uiTabStack*		tabstack_;
    ObjectSet<uiGenInput> nmflds_;
    uiTable*		tbl_;
    uiFlatViewMainWin*	vwwinnd_;
    uiPDF1DViewWin*	vwwin1d_;

    void		mkTable(uiGroup*);

    bool		getNamesFromScreen();
    void		putValsToScreen();
    bool		getValsFromScreen(bool* chg=0);
    void		setToolTips();
    void		updateUI();

    void		viewPDF(CallBacker*);
    void		vwWinClose(CallBacker*);
    void		tabChg(CallBacker*);
    void		smoothReq(CallBacker*);
    void		dimNext(CallBacker*);
    void		dimPrev(CallBacker*);

};


/*!\brief Group to edit Gaussian PPDF's. */


class uiEditGaussianProbDenFunc : public uiEditProbDenFunc
{ mODTextTranslationClass(uiEditGaussianProbDenFunc);
public:

			uiEditGaussianProbDenFunc(uiParent*,ProbDenFunc&,
						bool editable,bool isnew=false);

    virtual bool	commitChanges();

protected:

    Gaussian1DProbDenFunc* pdf1d_;
    Gaussian2DProbDenFunc* pdf2d_;
    GaussianNDProbDenFunc* pdfnd_;

    uiTabStack*		tabstack_;
    uiGenInput*		ccfld_;
    ObjectSet<uiGenInput> nmflds_;
    ObjectSet<uiGenInput> expflds_;
    ObjectSet<uiGenInput> stdflds_;
    uiComboBox*		var1fld_;
    uiComboBox*		var2fld_;
    uiPushButton*	addsetbut_;
    uiListBox*		defcorrsfld_;
    uiToolButton*	rmbut_;

    float		getCC() const;
    void		mkCorrTabFlds(uiGroup*);
    int			findCorr() const;
    void		updateCorrList(int);

    void		initGrp(CallBacker*);
    void		tabChg(CallBacker*);
    void		corrSel(CallBacker*);
    void		varSel(CallBacker*);
    void		addSetPush(CallBacker*);
    void		rmPush(CallBacker*);

};



