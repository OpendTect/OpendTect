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
			CheckShotCorr( Well::Log&,const Well::D2TModel&,
					bool isvelocitylog = false );
			~CheckShotCorr();

    Well::Log&		csLog()		{ return cslog_; }

protected:

    Well::Log& 		log_;
    Well::Log& 		cslog_;
    void 		calibrateLog2CheckShot(const Well::Log&);
};

}; //namespace WellTie
#endif
