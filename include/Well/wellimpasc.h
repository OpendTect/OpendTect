#ifndef wellimpasc_h
#define wellimpasc_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Bert Bril
 Date:		Aug 2003
 RCS:		$Id: wellimpasc.h,v 1.1 2003-08-22 16:40:34 bert Exp $
________________________________________________________________________


-*/

#include "sets.h"
#include "bufstring.h"
#include "ranges.h"
#include <iosfwd>

namespace Well
{
class Data;

class AscImporter
{
public:

			AscImporter( Data& d ) : wd(d)	{}

    const char*		getTrack(const char*,bool first_is_surface) const;
    const char*		getD2T(const char*,bool istvd) const;

    class LasFileInfo
    {
    public:
				LasFileInfo()
				    : zrg(mUndefValue,-mUndefValue)
				    , undefval(-999.25)		{}

	ObjectSet<BufferString>	lognms;
	Interval<float>		zrg;
	float			undefval;
    };

    const char*		getLogInfo(const char* lasfnm,LasFileInfo&);
    const char*		getLogs(const char* lasfnm,const LasFileInfo&) const;

protected:

    Data&		wd;

};

}; // namespace Well

#endif
