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
class uiToolButton;
namespace Well { class Log; class LogSet; }
namespace Math { class Formula; }


/*!\brief group to select the well logs which will be used as input
  to compute a new log for the formula in uiWellLogCalc. */


class uiWellLogCalcInpData : public uiMathExpressionVariable
{
public:
			uiWellLogCalcInpData(uiWellLogCalc*,uiGroup*,int);
			~uiWellLogCalcInpData();

    void		setProp(const PropertyRef::StdType&);
    const Well::Log*	getLog();
    bool		getInp(uiWellLogCalc::InpData&);

    const Well::LogSet* wls_;

protected:

    uiCheckBox*		udfbox_;
    uiToolButton*	vwbut_;

    Well::Log*          loginsi_;

    void		showHideVwBut(CallBacker*);
    void		vwLog(CallBacker*);

};


#endif
