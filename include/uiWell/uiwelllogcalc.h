#ifndef uiwelllogcalc_h
#define uiwelllogcalc_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		June 2009
 RCS:		$Id: uiwelllogcalc.h,v 1.7 2012-02-03 10:47:29 cvsbert Exp $
________________________________________________________________________

-*/

#include "uidialog.h"

class uiGenInput;
class uiCheckBox;
class uiComboBox;
class uiMathExpression;
class MathExpression;
class uiWellLogCalcInpData;
namespace Well { class Log; class LogSet; }

/*! \brief Dialog for marker specifications */

mClass uiWellLogCalc : public uiDialog
{
public:
				uiWellLogCalc(uiParent*,Well::LogSet&);
				~uiWellLogCalc();

    bool			haveNewLogs() const	{ return havenew_; }

protected:

    uiMathExpression*		formfld_;
    uiGenInput*			nmfld_;
    uiGenInput*			dahrgfld_;
    uiCheckBox*			ftbox_;
    uiComboBox*			unfld_;
    ObjectSet<uiWellLogCalcInpData> inpdataflds_;
    Well::LogSet&		wls_;

    int				nrvars_;
    int				nrspecvars_;
    TypeSet<int>		recvaridxs_;
    TypeSet<int>		specvaridxs_;
    BoolTypeSet			isspecvar_;
    TypeSet<float>		startvals_;
    MathExpression*		expr_;
    bool			havenew_;
    StepInterval<float>		dahrg_;

    friend class		uiWellLogCalcInpData;

    struct InpData
    {
			InpData( const Well::Log* w=0, int s=0, bool n=false )
			    : wl_(w), shift_(s), noudf_(n), specidx_(-1) {}
	bool		operator ==( const InpData& id ) const
					{ return wl_ == id.wl_; }
	const Well::Log* wl_;
	int		shift_;
	bool		noudf_;
	int		specidx_;
    };

    void			getMathExpr();
    bool			getInpData(TypeSet<InpData>&);
    bool			getRecInfo();
    bool			calcLog(Well::Log&,const TypeSet<InpData>&);

    void			initWin(CallBacker*);
    void			rockPhysReq(CallBacker*);
    void			feetSel(CallBacker*);
    void			formSet(CallBacker*);
    void			inpSel(CallBacker*);
    bool			acceptOK(CallBacker*);

};

#endif
