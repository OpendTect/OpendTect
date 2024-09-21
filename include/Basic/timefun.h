#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "basicmod.h"
#include "gendefs.h"

#include <ctime>

mFDQtclass(QElapsedTimer)

namespace Time
{

    mExpClass(Basic) Counter
    {
    public:

			Counter();
			~Counter();
	void		start();
	int		restart();		//!< Returns elapsed time in ms
	int		elapsed() const;	//!< Returns elapsed time in ms

    protected:

	mQtclass(QElapsedTimer*)	qelapstimer_;

    };

    mExpClass(Basic) FileTimeSet
    {
    public:
				FileTimeSet();
				FileTimeSet(const FileTimeSet&);
				~FileTimeSet();

	FileTimeSet&		operator =(const FileTimeSet&);

	std::timespec		getModificationTime() const;
	std::timespec		getAccessTime() const;
	std::timespec		getCreationTime() const;
	bool			hasModificationTime() const;
	bool			hasAccessTime() const;
	bool			hasCreationTime() const;

	FileTimeSet&		setModificationTime(const std::timespec&);
	FileTimeSet&		setAccessTime(const std::timespec&);
	FileTimeSet&		setCreationTime(const std::timespec&);

    private:
	std::timespec	modtime_;
	std::timespec	acctime_;
	std::timespec	crtime_;
    };


    mGlobal(Basic) od_int64 getMilliSeconds();		//!< From epoch (POSIX)
    mGlobal(Basic) od_int64 passedSince(od_int64);
    mGlobal(Basic) od_int64 toMSecs(const std::timespec&);
    mGlobal(Basic) std::timespec fromMSecs(od_int64 msecs);
    mGlobal(Basic) std::timespec getPosixFromNTFS(od_uint64);
    mGlobal(Basic) od_uint64 getNTFSFromPosix(const std::timespec&);
			/*!< NTFS time origin:; January 1, 1600, 0:00
			     returned value is in 100th of nanosecond */

    mGlobal(Basic) const char*	defDateTimeFmt();
    mGlobal(Basic) const char*	defDateTimeTzFmt();	//!< With time zone
    mGlobal(Basic) const char*	defDateFmt();
    mGlobal(Basic) const char*	defTimeFmt();

    mGlobal(Basic) const char*	getISODateTimeString(bool local=false);
    mGlobal(Basic) const char*	getDateTimeString(const char* fmt
					    =defDateTimeFmt(),bool local=true);
    mGlobal(Basic) const char*	getDateTimeString(od_int64 timeinms,
					    const char* fmt=defDateTimeFmt(),
					    bool local=true);
				//!< Provide time in milliseconds since epoch
    mGlobal(Basic) const char*	getDateTimeString(const std::timespec&,
					    const char* fmt=defDateTimeFmt(),
					    bool local=true);
    mGlobal(Basic) const char*	getDateString(const char* fmt=defDateFmt(),
					      bool local=true);
    mGlobal(Basic) const char*	getTimeString(const char* fmt=defTimeFmt(),
					      bool local=true);
    mGlobal(Basic) const char*	getLocalDateTimeFromString(const char*);

    mGlobal(Basic) bool isEarlier(const char* first, const char* second,
			   const char* fmt=defDateTimeFmt());
			/*! returns true if the first DateTime string is
			  earlier than the second */
    mGlobal(Basic) const char*	getTimeDiffString(od_int64 deltatimeins,
						  int precision);
			/*! returns time as 1d:2h:35m:15s */

} // namespace Time
