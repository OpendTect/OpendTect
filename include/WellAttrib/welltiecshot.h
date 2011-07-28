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

#include "commondefs.h"

namespace Well { class Log; class D2TModel; }
namespace WellTie
{

mClass CheckShotCorr  
{
public:
			CheckShotCorr( Well::Log&, float startdah,
				const Well::D2TModel&,bool isvelocitylog);

    static void		calibrateLog2Log( const Well::Log& calibrationlog,
	    				 	Well::Log& calibratedlog );
};

}; //namespace WellTie
#endif
