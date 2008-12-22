#ifndef statrand_h
#define statrand_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert Bril
 Date:          Sep 2006
 RCS:           $Id: statrand.h,v 1.2 2008-12-22 04:13:28 cvsranojay Exp $
________________________________________________________________________

-*/

#include "gendefs.h"

namespace Stats
{

mClass RandGen
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
