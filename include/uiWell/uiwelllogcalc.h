#ifndef uiwelllogcalc_h
#define uiwelllogcalc_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bert
 Date:		June 2009
 RCS:		$Id: uiwelllogcalc.h,v 1.2 2009-06-17 15:07:39 cvsbert Exp $
________________________________________________________________________

-*/

#include "uidialog.h"

class uiGenInput;
class uiCheckBox;
class MathExpression;
class uiLabeledComboBox;
namespace Well { class Log; class LogSet; }

/*! \brief Dialog for marker specifications */

mClass uiWellLogCalc : public uiDialog
{
public:
				uiWellLogCalc(uiParent*,Well::LogSet&);
				~uiWellLogCalc();

    bool			haveNewLogs() const	{ return havenew_; }

protected:

    uiGenInput*			formfld_;
    uiGenInput*			nmfld_;
    uiGenInput*			dahrgfld_;
    uiCheckBox*			ftbox_;
    ObjectSet<uiLabeledComboBox> varselflds_;
    Well::LogSet&		wls_;

    int				nrvars_;
    TypeSet<int>		recvars_;
    TypeSet<float>		startvals_;
    MathExpression*		expr_;
    bool			havenew_;
    StepInterval<float>		dahrg_;

    void			getMathExpr();
    void			dispVarInps(int);
    bool			getInpsAndShifts(ObjectSet<const Well::Log>&,
					TypeSet<int>&);
    bool			getRecInfo();
    bool			calcLog(Well::Log&,ObjectSet<const Well::Log>&,
					TypeSet<int>&);

    void			initWin(CallBacker*);
    void			feetSel(CallBacker*);
    void			formSet(CallBacker*);
    void			inpSel(CallBacker*);
    bool			acceptOK(CallBacker*);

};

#endif
