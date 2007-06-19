#ifndef tutlogtools_h
#define tutlogtools_h
/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : R.K. Singh
 * DATE     : June 2007
 * ID       : $Id: tutlogtools.h,v 1.1 2007-06-19 09:57:58 cvsraman Exp $
-*/

#include "bufstring.h"
#include "welllog.h"
#include "welllogset.h"

namespace Tut
{

class LogTools
{
public:

    			LogTools(const Well::Log& inp,Well::Log& outp)
			    : inplog_(inp)
			    , outplog_(outp)
    			{}			    

    bool		runSmooth(int gate);

protected:

    const Well::Log&	inplog_;
    Well::Log&		outplog_;

};

} // namespace

#endif
