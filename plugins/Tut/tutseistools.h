
#ifndef tutseistools_h
#define tutseistools_h
/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : R.K. Singh
 * DATE     : Mar 2007
 * ID       : $Id: tutseistools.h,v 1.1 2007-05-09 15:58:49 cvsbert Exp $
-*/

#include "gendefs.h"
class SeisTrc;


namespace Tut
{

class SeisTools
{
public:

    			SeisTools(float factr=1,float shft=0);

    inline float	factor() const		{ return factor_; }
    inline void		setFactor( float f )	{ factor_ = f; }
    inline float	shift() const		{ return shift_; }
    inline void		setShift( float s )	{ shift_ = s; }

    void		apply(SeisTrc&) const;

protected:

    float		factor_;
    float		shift_;

};

} // namespace

#endif
