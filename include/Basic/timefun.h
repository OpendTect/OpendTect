#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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


    mGlobal(Basic) int getMilliSeconds();		//!< From day start
    mGlobal(Basic) int passedSince(int);


    mGlobal(Basic) const char*	defDateTimeFmt();
    mGlobal(Basic) const char*	defDateTimeTzFmt();	//!< With time zone
    mGlobal(Basic) const char*	defDateFmt();
    mGlobal(Basic) const char*	defTimeFmt();

    mGlobal(Basic) const char*	getISODateTimeString(bool local=false);
    mGlobal(Basic) const char*	getDateTimeString(const char* fmt
					    =defDateTimeFmt(),bool local=true);
    mGlobal(Basic) const char*	getDateString(const char* fmt=defDateFmt(),
					      bool local=true);
    mGlobal(Basic) const char*	getTimeString(const char* fmt=defTimeFmt(),
					      bool local=true);
    mGlobal(Basic) const char*	getLocalDateTimeFromString(const char*);

    mGlobal(Basic) bool isEarlier(const char* first, const char* second,
			   const char* fmt=defDateTimeFmt());
			/*! returns true if the first DateTime string is
			  earlier than the second */
    mGlobal(Basic) const char*	getTimeString(od_int64 timeins,int precision);
			/*! returns time as 1d:2h:35m:15s */

} // namespace Time
