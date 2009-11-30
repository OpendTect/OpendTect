#ifndef welltiecshot_h
#define welltiecshot_h

/*+
________________________________________________________________________

(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:        Bruno
Date:          January 2009
RCS:           $Id: welltiecshot.h,v 1.1 2009-01-19 13:02:33 cvsbruno Exp
$
________________________________________________________________________

-*/

#include "uidialog.h"

class DataPointSet;
namespace Well
{
    class Data;
    class Log;
    class D2TModel;
}

class uiFileInput;
class uiFunctionDisplay;
class uiGenInput;
class uiLabel;

namespace WellTie
{
    class DataHolder;
    class GeoCalculator;

mClass CheckShotCorr  
{
public:
		    CheckShotCorr(WellTie::DataHolder&);
		    ~CheckShotCorr() {};			

    const Well::Log& 		corrLog()              { return *log_; }


protected:

    Well::Log* 			log_;
    Well::D2TModel* 		cs_;

    void 			setCSToLogScale(TypeSet<float>&,double, 
						WellTie::GeoCalculator&);
    void 			calibrateLogToCS(const TypeSet<float>&,
						WellTie::GeoCalculator&);
};

}; //namespace WellTie
#endif
