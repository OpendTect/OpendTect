#ifndef wellreader_h
#define wellreader_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bert Bril
 Date:		Aug 2003
 RCS:		$Id: wellreader.h,v 1.6 2003-11-07 12:21:52 bert Exp $
________________________________________________________________________


-*/

#include "wellio.h"
#include "sets.h"
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

    bool		getInfo(istream&) const;	//!< Read from a stream
    bool		addLog(istream&) const;		//!< Read from a stream
    bool		getMarkers(istream&) const;	//!< Read from a stream
    bool		getD2T(istream&) const;		//!< Read from a stream

protected:

    Data&		wd;

    const char*		rdHdr(istream&,const char*) const;
    bool		getOldTimeWell(istream&) const;
    Log*		rdLogHdr(istream&,int) const;
    bool		getTrack(istream&) const;

};

}; // namespace Well

#endif
