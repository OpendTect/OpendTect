#ifndef wellreader_h
#define wellreader_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bert Bril
 Date:		Aug 2003
 RCS:		$Id: wellreader.h,v 1.7 2004-04-28 21:30:58 bert Exp $
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

    bool		getInfo(std::istream&) const;
    bool		addLog(std::istream&) const;
    bool		getMarkers(std::istream&) const;
    bool		getD2T(std::istream&) const;

protected:

    Data&		wd;

    const char*		rdHdr(std::istream&,const char*) const;
    bool		getOldTimeWell(std::istream&) const;
    Log*		rdLogHdr(std::istream&,int) const;
    bool		getTrack(std::istream&) const;

};

}; // namespace Well

#endif
