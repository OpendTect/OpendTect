#ifndef wellwriter_h
#define wellwriter_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bert Bril
 Date:		Aug 2003
 RCS:		$Id: wellwriter.h,v 1.3 2003-11-07 12:21:52 bert Exp $
________________________________________________________________________


-*/

#include "wellio.h"
#include <iosfwd>

namespace Well
{
class Data;
class Log;

class Writer : public IO
{
public:

			Writer(const char* fnm,const Data&);

    bool		put() const;		//!< Just write all

    bool		putInfo() const;	//!< Write Info only
    bool		putLogs() const;	//!< Write logs only
    bool		putMarkers() const;	//!< Write Markers only
    bool		putD2T() const;		//!< Write D2T model only

    bool		putInfo(ostream&) const;	//!< Write to a stream
    bool		putLog(ostream&,const Log&) const;
    bool		putMarkers(ostream&) const;	//!< Write to a stream
    bool		putD2T(ostream&) const;		//!< Write to a stream

protected:

    const Data&		wd;

    bool		wrHdr(ostream&,const char*) const;
    bool		putTrack(ostream&) const;

};

}; // namespace Well

#endif
