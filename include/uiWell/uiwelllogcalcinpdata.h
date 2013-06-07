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

class uiGenInput;                                                               
class uiCheckBox;                                                               
namespace Well { class Log; class LogSet; }

/*! \ Brief: UI utility to select the well logs which will be used as input
  to compute a new log from a formula*/

class uiWellLogCalcInpData : public uiMathExpressionVariable
{
public:
				uiWellLogCalcInpData(uiWellLogCalc*,
						     uiGroup*,int);
				~uiWellLogCalcInpData();

    virtual void		use(const MathExpression*);
    const Well::Log*		getLog();
    bool			getInp(uiWellLogCalc::InpData&);

    const Well::LogSet* wls_;                                                   

protected:

    uiCheckBox*         udfbox_;                                                
    Well::Log*          convertedlog_;                                          
    bool                lognmsettodef_;

    void		inputSel(CallBacker*);

public:
    void                        restrictLogChoice(const PropertyRef::StdType&);
};

#endif
