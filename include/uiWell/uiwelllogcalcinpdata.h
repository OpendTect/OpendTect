#ifndef uiwelllogcalcinpdata_h
#define uiwelllogcalcinpdata_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Helene
 Date:		March 2012
 RCS:		$Id: uiwelllogcalcinpdata.h,v 1.1 2012/03/23 10:39:11 cvshelene Exp $
________________________________________________________________________

-*/

#include "uiforminputsel.h"
#include "bufstringset.h"
#include "uiwelllogcalc.h"

class uiGenInput;                                                               
class uiCheckBox;                                                               
namespace Well { class Log; class LogSet; }

/*! \ Brief: UI utility to select the well logs which will be used as input
  to compute a new log from a formula*/

class uiWellLogCalcInpData : public uiFormInputSel
{
public:
				uiWellLogCalcInpData(uiWellLogCalc*,
						     uiGroup*,int);
				~uiWellLogCalcInpData();

    virtual void		use(MathExpression*);
    const Well::Log*		getLog();
    bool			getInp(uiWellLogCalc::InpData&);

    const Well::LogSet* wls_;                                                   

protected:

    uiCheckBox*         udfbox_;                                                
    Well::Log*          convertedlog_;                                          
    bool                lognmsettodef_;
};

#endif
