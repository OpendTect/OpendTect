#ifndef timefun_h
#define timefun_h

/*@+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H.Bril
 Date:		3-5-1994
 Contents:	Time functions
 RCS:		$Id: timefun.h,v 1.16 2012-08-03 13:00:15 cvskris Exp $
________________________________________________________________________

-*/

#include "basicmod.h"
#include "gendefs.h"

class QTime;

namespace Time
{

mClass(Basic) Counter
{
public:
    		Counter();
		~Counter();
    void	start();
    int		restart();		//!< Returns elapsed time in ms
    int		elapsed() const;	//!< Returns elapsed time in ms

protected:
    QTime&	qtime_;
};


    mGlobal(Basic) int getMilliSeconds();          //!< From day start
    mGlobal(Basic) int passedSince(int);


    mGlobal(Basic) const char*	defDateTimeFmt();
    mGlobal(Basic) const char*	defDateFmt();
    mGlobal(Basic) const char*	defTimeFmt();

    mGlobal(Basic) const char*	getDateTimeString(const char* fmt=defDateTimeFmt(),
					  bool local=true);
    mGlobal(Basic) const char*	getDateString(const char* fmt=defDateFmt(),
				      bool local=true);
    mGlobal(Basic) const char*	getTimeString(const char* fmt=defTimeFmt(),
				      bool local=true);

    mGlobal(Basic) bool isEarlier(const char* first, const char* second,
	    		   const char* fmt=defDateTimeFmt());
			/*! returns true if the first DateTime string is
			  earlier than the second */

} // namespace Time

#endif

