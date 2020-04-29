#pragma once

/*@+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		3-5-1994
 Contents:	Time functions
________________________________________________________________________

-*/

#include "basicmod.h"
#include "gendefs.h"

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


    mGlobal(Basic) int		getMilliSeconds();
				//!< From day start.
    mGlobal(Basic) int		passedSince(int);
				//!< ... since the getMilliSeconds() before.
    mGlobal(Basic) od_int64	getFileTimeInSeconds();
				//!< Can be compared to File::getTimeInSeconds()

    mGlobal(Basic) const char*	getISOUTCDateTimeString();
				//!< Human readable but valid time stamp
    mGlobal(Basic) int		compareISOUTCDateTimeStrings(const char*,
							     const char*);
				//!< -1, 0 or 1. 0 means equal.

    inline bool		isEarlierStamp( const char* ts1, const char* ts2 )
			{ return compareISOUTCDateTimeStrings( ts1, ts2 ) < 0; }

    // For presentation only:

    mGlobal(Basic) const char*	defDateTimeFmt();
    mGlobal(Basic) const char*	defDateFmt();
    mGlobal(Basic) const char*	defTimeFmt();

    // Convert ISOUTC string into user-specific date string
    mGlobal(Basic) const char*	getUsrDateTimeStringFromISOUTC(
					    const char* isostr,
					    const char* fmt=defDateTimeFmt(),
					    bool local=true);
    mGlobal(Basic) const char*	getUsrDateStringFromISOUTC(const char* isostr,
					    const char* fmt=defDateFmt(),
					    bool local=true);
    mGlobal(Basic) const char*	getUsrTimeStringFromISOUTC(const char* isostr,
					    const char* fmt=defTimeFmt(),
					    bool local=true);

    // Convenience: get user-formatted strings for current time
    mGlobal(Basic) const char*	getUsrDateTimeString(const char* fmt
					    =defDateTimeFmt(),bool local=true);
    mGlobal(Basic) const char*	getUsrDateString(const char* fmt=defDateFmt(),
					      bool local=true);
    mGlobal(Basic) const char*	getUsrTimeString(const char* fmt=defTimeFmt(),
					      bool local=true);

    mGlobal(Basic) const char*	getUsrFileDateTime(const char* fnm,
					    bool modif=true,
					    const char* fmt=defDateTimeFmt(),
					    bool local=true);

} // namespace Time
