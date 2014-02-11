#ifndef uiwelllogcalcinpdata_h
#define uiwelllogcalcinpdata_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Helene
 Date:		March 2012
 RCS:		$Id$
________________________________________________________________________

-*/

#include "uimathexpressionvariable.h"
#include "bufstringset.h"
#include "uiwelllogcalc.h"

class uiCheckBox;
namespace Well { class Log; class LogSet; }


/*!\brief group to select the well logs which will be used as input
  to compute a new log for the formula in uiWellLogCalc. */


class uiWellLogCalcInpData : public uiMathExpressionVariable
{
public:
			uiWellLogCalcInpData(uiWellLogCalc*,uiGroup*,int);
			~uiWellLogCalcInpData();

    virtual void	use(const MathExpression*);
    virtual void	setUnit(const char*);
    const Well::Log*	getLog();
    bool		getInp(uiWellLogCalc::InpData&);
    void		restrictLogChoice(const PropertyRef::StdType&);
    void		resetLogRestriction();

    const Well::LogSet* wls_;

protected:

    uiCheckBox*		udfbox_;

    Well::Log*          convertedlog_;
    bool		forceunit_;

    void		inputSel(CallBacker*);
    void		vwLog(CallBacker*);

};


#endif
