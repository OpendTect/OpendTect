#ifndef tutlogtools_h
#define tutlogtools_h
/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : R.K. Singh
 * DATE     : June 2007
 * ID       : $Id: tutlogtools.h,v 1.2 2009-04-09 11:49:08 cvsranojay Exp $
-*/

#include "commondefs.h"

namespace Well { class Log; }

namespace Tut
{

mClass LogTools
{
public:

    			LogTools(const Well::Log& input,Well::Log& output);
			   		    

    bool		runSmooth(int gate);

protected:

    const Well::Log&	inplog_;
    Well::Log&		outplog_;

};

} // namespace

#endif
