#ifndef statrand_h
#define statrand_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert Bril
 Date:          Sep 2006
 RCS:           $Id: statrand.h,v 1.1 2006-09-21 12:02:46 cvsbert Exp $
________________________________________________________________________

-*/

#include "gendefs.h"

namespace Stats
{

class RandGen
{
public:

    static void		init(int seed=0);
    			//!< If no seed passed, will generate one if needed
    static double	get();
    			//!< Uniform [0-1]
    static double	getNormal(double expectation,double stdev);
    			//!< Normally distributed
    static int		getIndex(int sz);
    			//!< random index in the range [0,sz>

private:

    static int		seed_;

};

}; // namespace Stats

#endif
