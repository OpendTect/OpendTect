#ifndef wellreader_h
#define wellreader_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bert Bril
 Date:		Aug 2003
 RCS:		$Id: wellreader.h,v 1.9 2004-05-11 13:08:41 bert Exp $
________________________________________________________________________


-*/

#include "wellio.h"
#include "sets.h"
#include "ranges.h"
#include <iosfwd>
class BufferStringSet;


namespace Well
{
class Data;
class Log;

class Reader : public IO
{
public:

			Reader(const char* fnm,Data&);

    bool		get() const;		//!< Just read all

    bool		getInfo() const;	//!< Read Info only
    bool		getLogs() const;	//!< Read logs only
    bool		getMarkers() const;	//!< Read Markers only
    bool		getD2T() const;		//!< Read D2T model only
    void		getLogInfo(BufferStringSet&) const;	
    						//!< Read logheaders only

    bool		getInfo(std::istream&) const;
    bool		addLog(std::istream&) const;
    bool		getMarkers(std::istream&) const;
    bool		getD2T(std::istream&) const;

    Interval<float>	getLogDahRange(const char*) const;
    			//!< If no log with this name, returns [undef,undef]

protected:

    Data&		wd;

    const char*		rdHdr(std::istream&,const char*) const;
    bool		getOldTimeWell(std::istream&) const;
    Log*		rdLogHdr(std::istream&,int) const;
    bool		getTrack(std::istream&) const;

};

}; // namespace Well

#endif
