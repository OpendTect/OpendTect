#ifndef dateinfo_h
#define dateinfo_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		12-3-96
 RCS:		$Id: dateinfo.h,v 1.8 2009-07-22 16:01:13 cvsbert Exp $
________________________________________________________________________

-*/

#include "enums.h"
#include "bufstring.h"
#include "undefval.h"

/*!\brief Clumsy date info class. Has nice 'relative' printouts.

Class does not work before 1996 or after 2099. Other constructors
are for dates other than today. Constructors accept numbers as in normal usage.

*/


mClass DateInfo
{
public:
    enum Day		{ Su=0, Mo, Tu, We, Th, Fr, Sa };
			DeclareEnumUtils(Day)
    enum Month		{ Jan=0, Feb, Mar, Apr, May, Jun, Jul, Aug, Sep,
			  Oct, Nov, Dec };
			DeclareEnumUtils(Month)

			DateInfo();		//!< Today.
			DateInfo(int key_);
			DateInfo(int yr,Month,int dy);
			DateInfo(int yr,int mnth,int dy);
			DateInfo(int yr,const char* mnth,int dy);

    bool		isUdf() const		{ return !mIsUdf(days96_); }
    void		setUdf()		{ days96_ = mUdf(int); }
    int			key() const		{ return days96_; }
    int			day() const		{ return day_  + 1; }
    Month		month() const		{ return month_; }
    int			year() const		{ return year_ + 1996; }
    void		setDay(int);
    void		setMonth(Month);
    void		setYear(int);

    int			operator ==( const DateInfo& di ) const
			{ return days96_ == di.days96_; }
    int			operator <( const DateInfo& di ) const
			{ return days96_ < di.days96_; }
    int			operator <=( const DateInfo& di ) const
			{ return days96_ <= di.days96_; }
    DateInfo&		operator +=(int);
    DateInfo&		operator -=(int);
    friend int		operator -(const DateInfo&,const DateInfo&);
    void		addMonths(int);

    const char*		weekDayName() const;
    const char*		monthName() const	{ return eString(Month,month_);}
    const char*		whenRelative(const DateInfo* di=0) const;
    static int		daysInMonth(int yr,Month);
    static int		daysInYear( int yr )	{ return yr%4 ? 365 : 366; }

    void		toString(BufferString&) const;
    bool		fromString(const char*);
    void		getFullDisp(BufferString&,bool withtime=true) const;

private:

    int			days96_;
    int			day_;
    Month		month_;
    int			year_;

    void		getDayMonth(int,int,int&,Month&);
    void		calcDMY();
    void		calcDays96();
    void		getRel(const DateInfo&) const;
    void		getRelToday() const;
    void		addDay() const;

};


inline int operator -( const DateInfo& di1, const DateInfo& di2 )
{ return di1.days96_ - di2.days96_; }


#endif
