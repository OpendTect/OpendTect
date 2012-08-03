#ifndef wellreader_h
#define wellreader_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert Bril
 Date:		Aug 2003
 RCS:		$Id: wellreader.h,v 1.17 2012-08-03 13:00:46 cvskris Exp $
________________________________________________________________________


-*/

#include "wellmod.h"
#include "wellio.h"
#include "sets.h"
#include "ranges.h"
#include <iosfwd>
class BufferStringSet;


namespace Well
{
class Data;
class Log;

mClass(Well) Reader : public IO
{
public:

			Reader(const char* fnm,Data&);

    bool		get() const;		//!< Just read all

    bool		getInfo() const;	//!< Read Info only
    bool		getTrack() const;	//!< Read Track only
    bool		getLogs() const;	//!< Read logs only
    bool		getMarkers() const;	//!< Read Markers only
    bool		getD2T() const;		//!< Read D2T model only
    bool		getCSMdl() const;	//!< Read Checkshot model only
    bool		getDispProps() const;	//!< Read display props only

    bool		getInfo(std::istream&) const;
    bool		addLog(std::istream&) const;
    bool		getMarkers(std::istream&) const;
    bool		getD2T(std::istream&) const;
    bool		getCSMdl(std::istream&) const;
    bool		getDispProps(std::istream&) const;

    void		getLogInfo(BufferStringSet&) const;
    Interval<float>	getLogDahRange(const char*) const;
    			//!< If no log with this name, returns [undef,undef]
    Interval<float>	getAllLogsDahRange() const;
    			//!< If no log returns [undef,undef]

protected:

    Data&		wd;

    bool		getOldTimeWell(std::istream&) const;
    void		readLogData(Log&,std::istream&,int) const;
    bool		getTrack(std::istream&) const;
    bool		doGetD2T(std::istream&,bool csmdl) const;
    bool		doGetD2T(bool) const;

    static Log*		rdLogHdr(std::istream&,int&,int);

};

}; // namespace Well

#endif

