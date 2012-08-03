#ifndef uiwelllogcalc_h
#define uiwelllogcalc_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		June 2009
 RCS:		$Id: uiwelllogcalc.h,v 1.13 2012-08-03 13:01:20 cvskris Exp $
________________________________________________________________________

-*/

#include "uiwellmod.h"
#include "uidialog.h"
#include "bufstringset.h"
#include "multiid.h"

class uiGenInput;
class uiCheckBox;
class uiComboBox;
class uiMathExpression;
class MathExpression;
class uiWellLogCalcInpData;
namespace Well { class Log; class LogSet; }

/*! \brief Dialog for marker specifications */

mClass(uiWell) uiWellLogCalc : public uiDialog
{
public:
				uiWellLogCalc(uiParent*,const Well::LogSet&,
						const BufferStringSet&,
						const TypeSet<MultiID>&);
				~uiWellLogCalc();

    bool			haveNewLogs() const	{ return havenew_; }

protected:

    uiMathExpression*		formfld_;
    uiGenInput*			nmfld_;
    uiGenInput*			srfld_;
    uiCheckBox*			ftbox_;
    uiComboBox*			unfld_;
    ObjectSet<uiWellLogCalcInpData> inpdataflds_;

    int				nrvars_;
    int				nrspecvars_;
    TypeSet<int>		recvaridxs_;
    TypeSet<int>		specvaridxs_;
    BoolTypeSet			isspecvar_;
    TypeSet<float>		startvals_;
    MathExpression*		expr_;
    bool			havenew_;
    float			zsampintv_;
    const BufferStringSet&	lognms_;
    const Well::LogSet&		wls_;
    const TypeSet<MultiID>	wellids_;
    BufferStringSet		inputunits_;
    BufferString		rpoutunit_;

    friend class		uiWellLogCalcInpData;

    struct InpData
    {
			InpData( const Well::Log* w=0, int s=0, bool n=false )
			    : wl_(w), shift_(s), noudf_(n), specidx_(-1)
			    , cstval_(mUdf(float)), iscst_(false) {}
	bool		operator ==( const InpData& id ) const
			{ return shift_ == id.shift_
			    && iscst_ ? mIsEqual(cstval_,id.cstval_, 1e-3)
				      : wl_ == id.wl_; }
	const Well::Log* wl_;
	int		shift_;
	bool		noudf_;
	int		specidx_;
	float		cstval_;
	bool		iscst_;
    };

    void			getMathExpr();
    void			setCurWls(const Well::LogSet&);
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

