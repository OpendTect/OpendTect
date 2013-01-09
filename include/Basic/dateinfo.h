#ifndef dateinfo_h
#define dateinfo_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		12-3-1996
 RCS:		$Id$
________________________________________________________________________

-*/

#include "basicmod.h"
#include "enums.h"
#include "undefval.h"
class BufferString;

/*!
\ingroup Basic
\brief A date info class.
  
  Class does not work before 1900. Non-default constructors are for dates
  other than today. Constructors accept numbers as in normal usage.
  
  Parsing: toString/fromString format is [n]n-xxx-nnnn, like 14-nov-2008.
  fromStdDateString() reads the 'file' standard,like 'Wed Nov 14 12:50:15 2008'.  fromNumString() reads '2008-11-14' or '14-11-2008' or the same with slashes,
  dots or colons.
  
  The class has some kewl 'relative' printouts.
*/

mClass(Basic) DateInfo
{
public:
    enum DayOfWeek	{ Su=0, Mo, Tu, We, Th, Fr, Sa };
			DeclareEnumUtils(DayOfWeek)
    enum Month		{ Jan=0, Feb, Mar, Apr, May, Jun, Jul, Aug, Sep,
			  Oct, Nov, Dec };
			DeclareEnumUtils(Month)	//!< Uses "jan", "feb" etc.
    static const char**	sFullMonths();		//!< str = sFullMonths()[Month]
    static const char**	sAllDaysInMonth();	//!< 0 to 31

			DateInfo();		//!< Today
			DateInfo(int usryr,int usrmnth,int usrdy);
			DateInfo(int usryr,Month,int usrdy);
			DateInfo(int usryr,const char* mnth,int usrdy);
			DateInfo(const char*);

    bool		isUdf() const		{ return mIsUdf(days1900_); }
    void		setUdf(bool yn=true);

    int			day() const		{ return days_  + 1; }
    Month		month() const		{ return (Month)months_; }
    int			year() const		{ return 1900 + years_; }
    int			usrMonth() const	{ return (int)months_ + 1; }
    void		setDay(int);
    void		setMonth(int);
    void		setMonth(Month);
    void		setYear(int);

    bool		operator ==(const DateInfo&) const;
    bool		operator !=(const DateInfo&) const;
    bool		operator <(const DateInfo&) const;
    bool		operator <=(const DateInfo&) const;
    bool		operator >(const DateInfo&) const;
    bool		operator >=(const DateInfo&) const;
    DateInfo&		operator +=(int);
    DateInfo&		operator -=( int dys )	{ *this += -dys; return *this; }
    friend int		operator -(const DateInfo&,const DateInfo&);
    void		addMonths(int);

    int		 	weekDay() const;	//!< Sunday => 1
    const char*		weekDayName() const;
    const char*		monthName() const      {return getMonthString(month());}
    const char*		whenRelative(const DateInfo* di=0) const;
    static const char*	fullMonthName(int);
    static const char*	fullMonthName(Month);
    const char*		fullMonthName() const  { return fullMonthName(month());}

    void		toString(BufferString&) const;
    bool		fromString(const char*);
    bool		fromStdDateString(const char*);
    bool		fromNumString(const char*,bool yearfirst);
    void		getUsrDisp(BufferString&,bool withcurtime) const;

    static int		daysInMonth(int yr,Month);
    static int		daysInYear( int yr )	{ return yr%4 ? 365 : 366; }
    static Month	usrMonth2Month(int);

private:

    int			days1900_;
    int			years_;
    int			months_;
    int			days_;

    void		getDaysMonths(int,int,int&,int&);
    void		calcDMY();
    void		calcDays1900();
    void		getRel(const DateInfo&,BufferString&) const;
    void		getRelToday(BufferString&) const;
    void		addDay(BufferString&) const;

public:

    int			key() const		{ return days1900_; }
			DateInfo(int the_key);

};


inline bool DateInfo::operator ==( const DateInfo& di ) const
{ return days1900_ == di.days1900_; }

inline bool DateInfo::operator !=( const DateInfo& di ) const
{ return days1900_ != di.days1900_; }

inline bool DateInfo::operator <( const DateInfo& di ) const
{ return days1900_ < di.days1900_; }

inline bool DateInfo::operator <=( const DateInfo& di ) const
{ return days1900_ <= di.days1900_; }

inline bool DateInfo::operator >( const DateInfo& di ) const
{ return days1900_ > di.days1900_; }

inline bool DateInfo::operator >=( const DateInfo& di ) const
{ return days1900_ >= di.days1900_; }

inline int operator -( const DateInfo& di1, const DateInfo& di2 )
{ return di1.days1900_ - di2.days1900_; }


#endif

