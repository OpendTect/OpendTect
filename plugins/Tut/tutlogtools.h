#ifndef tutlogtools_h
#define tutlogtools_h
/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : R.K. Singh
 * DATE     : June 2007
 * ID       : $Id: tutlogtools.h,v 1.4 2012-08-03 13:01:32 cvskris Exp $
-*/

#include "tutmod.h"
#include "commondefs.h"

namespace Well { class Log; }

namespace Tut
{

mClass(Tut) LogTools
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

