#ifndef timefun_h
#define timefun_h

/*@+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H.Bril
 Date:		3-5-1994
 Contents:	Time functions
 RCS:		$Id: timefun.h,v 1.15 2011/12/09 06:04:17 cvsranojay Exp $
________________________________________________________________________

-*/

#include "gendefs.h"

class QTime;

namespace Time
{

mClass Counter
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

    mGlobal bool isEarlier(const char* first, const char* second,
	    		   const char* fmt=defDateTimeFmt());
			/*! returns true if the first DateTime string is
			  earlier than the second */

} // namespace Time

#endif
