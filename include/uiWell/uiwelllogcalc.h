#ifndef uiwelllogcalc_h
#define uiwelllogcalc_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bert
 Date:		June 2009
 RCS:		$Id: uiwelllogcalc.h,v 1.1 2009-06-17 11:57:44 cvsbert Exp $
________________________________________________________________________

-*/

#include "uidialog.h"

class uiGenInput;
class MathExpression;
class uiLabeledComboBox;
namespace Well { class Log; class LogSet; }

/*! \brief Dialog for marker specifications */

mClass uiWellLogCalc : public uiDialog
{
public:
				uiWellLogCalc(uiParent*,Well::LogSet&);
				~uiWellLogCalc();

protected:

    uiGenInput*			formfld_;
    uiGenInput*			dahrgfld_;
    uiGenInput*			nmfld_;
    ObjectSet<uiLabeledComboBox> varselflds_;
    Well::LogSet&		wls_;

    int				nrvars_;
    TypeSet<int>		recvars_;
    TypeSet<float>		recstartvals_;
    MathExpression*		expr_;

    void			getMathExpr();
    void			dispVarInps(int);
    bool			getInpsAndShifts(ObjectSet<const Well::Log>&,
					TypeSet<int>&);
    bool			getRecInfo();
    bool			calcLog(Well::Log&,ObjectSet<const Well::Log>&,
					TypeSet<int>&);

    void			formSet(CallBacker*);
    void			inpSel(CallBacker*);
    bool			acceptOK(CallBacker*);

};

#endif
