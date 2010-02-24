#ifndef timefun_h
#define timefun_h

/*@+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H.Bril
 Date:		3-5-1994
 Contents:	Time functions
 RCS:		$Id: timefun.h,v 1.12 2010-02-24 10:49:34 cvsnanne Exp $
________________________________________________________________________

-*/

#include "gendefs.h"

#ifdef __cpp__

class QTime;

namespace Time
{

class Counter
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


    mGlobal int getMilliSeconds();          //!< From day start
    mGlobal int passedSince(int);


    mGlobal const char*	defDateTimeFmt();
    mGlobal const char*	defDateFmt();
    mGlobal const char*	defTimeFmt();

    mGlobal const char*	getDateTimeString(const char* fmt=defDateTimeFmt(),
					  bool local=true);
    mGlobal const char*	getDateString(const char* fmt=defDateFmt(),
				      bool local=true);
    mGlobal const char*	getTimeString(const char* fmt=defTimeFmt(),
				      bool local=true);

}
#endif


#ifdef __cpp__
extern "C" {
#endif

mGlobal int Time_getMilliSeconds(void);		/*!< From day start */
mGlobal int Time_passedSince(int); 		/*!< in millisecs */

mGlobal const char* Time_getFullDateString(void); /*!< full date/time */
mGlobal const char* Time_getTimeString(void);  	/*!< "hh::mm::ss" */

#ifdef __cpp__
}
#endif

#endif
