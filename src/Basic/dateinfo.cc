/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : 12-3-1996
 * FUNCTION : date info
-*/
 
static const char* rcsID = "$Id: dateinfo.cc,v 1.21 2011/12/23 15:24:14 cvskris Exp $";

#include "dateinfo.h"
#include "timefun.h"
#include "keystrs.h"
#include "separstr.h"
#include "staticstring.h"
#include <time.h>

	//  0   1   2   3    4    5    6    7    8    9   10   11   12
static const int normdaycount[]
	= { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365 };
static const int leapdaycount[]
	= { 0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335, 366 };


DefineEnumNames(DateInfo,DayOfWeek,2,"Week day") {
	"Sunday",
	"Monday",
	"Tuesday",
	"Wednesday",
	"Thursday",
	"Friday",
	"Saturday",
	0
};

DefineEnumNames(DateInfo,Month,3,"Month") {
	"jan", "feb", "mar", "apr", "may", "jun",
	"jul", "aug", "sep", "oct", "nov", "dec",
	0
};
static const char* fullmonths[] = {
	"January",
	"February",
	"March",
	"April",
	"May",
	"June",
	"July",
	"August",
	"September",
	"October",
	"November",
	"December",
	0
};
const char** DateInfo::sFullMonths()	{ return fullmonths; }



static const char* alldays[] =
{        "1",  "2",  "3",  "4",  "5",  "6",  "7",  "8",  "9",
  "10", "11", "12", "13", "14", "15", "16", "17", "18", "19",
  "20", "21", "22", "23", "24", "25", "26", "27", "28", "29",
  "30", "31", 0 };

const char** DateInfo::sAllDaysInMonth() { return alldays; }


DateInfo::DateInfo()
{
    static const od_int64 days1900_to_1970 = 25568;
    const int days1970 = (int)(time(0) / 86400L);
    days1900_ = days1970 + days1900_to_1970;
    calcDMY();
}


DateInfo::DateInfo( int ky )
	: days1900_(ky)
{
    calcDMY();
}


DateInfo::DateInfo( int yr, int mn, int dy )
	: years_(yr-1900)
	, days_(dy > 0 ? dy-1 : 0)
	, days1900_(0)
{
    setMonth( mn );
}


DateInfo::DateInfo( int yr, DateInfo::Month m, int dy )
	: years_(yr-1900)
	, months_((int)m)
{
    setDay( dy );
}


DateInfo::DateInfo( int yr, const char* mn, int dy )
	: years_(yr-1900)
	, months_((int)parseEnumMonth(mn))
{
    setDay( dy );
}


DateInfo::DateInfo( const char* str )
	: years_(mUdf(int))
	, months_(0)
	, days_(0)
	, days1900_(0)
{
    fromString( str );
}


void DateInfo::setUdf( bool yn )
{
    if ( yn )
	days1900_ = mUdf(int);
    else
	calcDays1900();
}


void DateInfo::setDay( int dy )
{
    if ( dy < 1 ) dy = 1; if ( dy > 31 ) dy = 31;
    days_ = dy - 1;
    if ( !isUdf() )
	calcDays1900();
}


void DateInfo::setMonth( DateInfo::Month mn )
{
    months_ = (int)mn;
    if ( !isUdf() )
	calcDays1900();
}


void DateInfo::setMonth( int mn )
{
    if ( mn < 1 ) mn = 1; if ( mn > 12 ) mn = 12;
    setMonth( (Month)(mn-1) );
}


void DateInfo::setYear( int yr )
{
    years_ = yr - 1900;
    if ( !isUdf() )
	calcDays1900();
}


DateInfo& DateInfo::operator +=( int dys )
{
    if ( !isUdf() )
	{ days1900_ += dys; calcDMY(); }
    return *this;
}


DateInfo::Month DateInfo::usrMonth2Month( int usrmnth )
{
    if ( usrmnth < 1 ) usrmnth = 1; if ( usrmnth > 12 ) usrmnth = 12;
    return (Month)(usrmnth-1);
}


void DateInfo::addMonths( int mns )
{
    if ( isUdf() || mns < 1 ) return;

    int nr = months_ + mns;
    if ( mns > 0 )
    {
	years_ += nr / 12;
	months_ = nr % 12;
    }
    else
    {
	if ( nr >= 0 )
	    months_ = nr;
	else
	{
	    years_ += nr / 12 - 1;
	    nr = (-nr) % 12;
	    months_ = 12 - nr;
	}
    }
    calcDays1900();
}


int DateInfo::weekDay() const
{
    int nr = days1900_ % 7 + 1;	// 1-jan-1900 was a Monday
    if ( nr > 6 ) nr = 0;
    return nr + 1;
}


const char* DateInfo::weekDayName() const
{
    const int nr = weekDay() - 1;
    return getDayOfWeekString((DayOfWeek) nr);
}


const char* DateInfo::fullMonthName( DateInfo::Month mnth )
{
    return fullmonths[ (int)mnth ];
}


const char* DateInfo::fullMonthName( int usrmnth )
{
    return fullMonthName( usrMonth2Month(usrmnth) );
}


int DateInfo::daysInMonth( int yr, DateInfo::Month m )
{
    const int* d = yr % 4 ? normdaycount : leapdaycount;
    return d[ (int)m + 1 ] - d[ (int)m ];
}


void DateInfo::getDaysMonths( int yr, int ndays, int& dys, int& mns )
{
    const int* d = yr % 4 ? normdaycount : leapdaycount;
    if ( ndays > d[12] || ndays <= 0 )
	{ dys = mns = 0; return; }

    mns = 0;
    while ( ndays >= d[mns+1] )
	mns++;
    dys = ndays - d[mns];
}


void DateInfo::calcDMY()
{
    int yrqtets = days1900_ / 1461;
    int qtetrest = days1900_ % 1461;
    years_ = 4 * yrqtets;
    if ( qtetrest > 1095 )	{ years_ += 3; qtetrest -= 1096; }
    else if ( qtetrest > 730 )	{ years_ += 2; qtetrest -= 731; }
    else if ( qtetrest > 365 )	{ years_ += 1; qtetrest -= 366; }

    getDaysMonths( years_, qtetrest, days_, months_ );
}


void DateInfo::calcDays1900()
{
    days1900_ = (years_/4) * 1461;
    int rest = years_ % 4;
    if ( rest )
    {
	days1900_ += 366; rest--;
	if ( rest )
	    days1900_ += rest * 365;
    }

    if ( months_ )
    {
	const int* nrdys4month = years_ % 4 ? normdaycount : leapdaycount;
	days1900_ += nrdys4month[ months_ ];
    }
    days1900_ += days_;
}



const char* DateInfo::whenRelative( const DateInfo* di ) const
{
    static StaticStringManager stm;
    BufferString& ret = stm.getString();

    if ( di )	getRel( *di, ret );
    else	getRelToday( ret );

    return ret.buf();
}


#define mRet(s) { bs = s; return; }

void DateInfo::getRel( const DateInfo& reld, BufferString& bs ) const
{
    if ( isUdf() || reld.isUdf() )
	mRet(sKey::Undef)
    else if ( reld.days1900_ == days1900_ )
	mRet("that day")

    const int nrdays = days1900_ - reld.days1900_;
    bs.setEmpty();
    if ( nrdays < 0 )
    {
	if ( nrdays == -1 )	mRet("the previous day")
	if ( nrdays == -7 )	mRet("one week earlier")
	if ( nrdays == -14 )	mRet("two weeks earlier")
	if ( nrdays == -21 )	mRet("three weeks earlier")
	if ( nrdays > -21 )
	    { bs += (-nrdays); bs += " days earlier"; return; }
    }
    else
    {
	if ( nrdays == 1 )	mRet("the next day")
	if ( nrdays == 7 )	mRet("one week later")
	if ( nrdays == 14 )	mRet("two weeks later")
	if ( nrdays == 21 )	mRet("three weeks later")
	if ( nrdays < 21 )
	    { bs += nrdays; bs += " days later"; return; }
    }

    if ( reld.months_ == months_ && reld.days_ == days_ )
    {
	int nryrs = years_ - reld.years_;
	bs = "exactly "; bs += abs(nryrs); bs += " year";
	if ( abs(nryrs) != 1 ) bs += "s";
	bs += nryrs > 0 ? " later" : " earlier";
	return;
    }

    if ( reld.years_ == years_ )
    {
	int nrmnths = months_ - reld.months_;
	if ( nrmnths > -2 && nrmnths < 2 )
	{
	    if ( reld.days_ == days_ )
		mRet(nrmnths<0?"one month earlier":"one month later")
	    bs = "the "; addDay( bs ); bs += " of ";
	    bs += nrmnths ? (nrmnths > 0 ? "the following" : "the previous")
			  : "that";
	    bs += " month";
	    return;
	}
    }

    bs = fullMonthName();
    bs += " the "; addDay( bs );
    if ( reld.years_ != years_ ) { bs += ", "; bs += year(); }
}


void DateInfo::getRelToday( BufferString& bs ) const
{
    if ( isUdf() ) mRet(sKey::Undef)

    DateInfo today;

    if ( today.days1900_ == days1900_ ) mRet("today")
    int nrdays = days1900_ - today.days1900_;
    if ( nrdays < 0 )
    {
	if ( nrdays == -1 ) mRet("yesterday")
	if ( nrdays == -2 ) mRet("the day before yesterday")
	if ( nrdays == -7 ) mRet("One week ago")
	if ( nrdays == -14 ) mRet("two weeks ago")
	if ( nrdays == -21 ) mRet("three weeks ago")
	if ( nrdays > -21 ) { bs = (-nrdays); bs += " days ago"; return; }
    }
    else
    {
	if ( nrdays == 1 ) mRet("tomorrow")
	if ( nrdays == 2 ) mRet("the day after tomorrow")
	if ( nrdays == 7 ) mRet("next week")
	if ( nrdays == 14 ) mRet("two weeks from now")
	if ( nrdays == 21 ) mRet("three weeks from now")
	if ( nrdays < 21 ) { bs = (-nrdays); bs += " days from now"; return; }
    }

    if ( today.months_ == months_ && today.days_ == days_ )
    {
	bs = "exactly ";
	int nryrs = years_ - today.years_;
	bs += abs(nryrs);
	bs += " years ";
	bs += nryrs > 0 ? "from now" : "ago";
	return;
    }

    if ( today.years_ == years_ )
    {
	int nrmnths = months_ - today.months_;
	if ( nrmnths > -2 && nrmnths < 2 )
	{
	    if ( today.days_ == days_ )
		mRet(nrmnths<0?"one month ago":"one month from now")
	    bs = "the "; addDay( bs ); bs += " of ";
	    bs += nrmnths ? (nrmnths > 0 ? "next" : "last") : "this";
	    bs += " month";
	    return;
	}
    }

    bs = day(); bs += " ";
    bs += fullMonthName();
    if ( today.years_ != years_ ) { bs += " "; bs += year(); }
}


void DateInfo::addDay( BufferString& bs ) const
{
    bs += day();
    const int ranknr = days_ > 12 ? days_%10 : days_;
    switch ( ranknr )
    {
    case 0: bs += "st"; break; case 1: bs += "nd"; break;
    case 2: bs += "rd"; break; default: bs += "th"; break;
    }
}


void DateInfo::toString( BufferString& str ) const
{
    if ( isUdf() )
	{ str += sKey::Undef; return; }

    str += day(); str += "-";
    str += DateInfo::getMonthString(month()); str += "-";
    str += year();
}


bool DateInfo::fromString( const char* inp )
{
    setUdf(); if ( !inp || !*inp ) return false;
    SeparString ss( inp, '-' );
    const char* dayptr = ss[0];
    if ( !isdigit(*dayptr) ) return false;

    days_ = toInt( dayptr );
    if ( days_ > 0 ) days_--;
    Month monthvar;
    parseEnumMonth( ss[1], monthvar );
    months_ = (int) monthvar;
    days1900_ = 0; setYear( toInt(ss[2]) );

    return true;
}


bool DateInfo::fromStdDateString( const char* inp )
{
    setUdf(); if ( !inp || !*inp ) return false;

    char buf[1024];
    const char* ptr = getNextWord( inp, buf );
    if ( !ptr ) return false;
    ptr = getNextWord( ptr, buf );
    if ( !ptr ) return false;

    buf[0] = tolower( buf[0] );
    Month monthvar;
    parseEnumMonth( buf, monthvar );
    months_ = (int) monthvar;

    ptr = getNextWord( ptr, buf );
    if ( !ptr ) return false;
    days_ = toInt( buf );
    if ( days_ > 0 ) days_--;

    ptr = strrchr( ptr, ' ' );
    if ( !ptr ) return false;
    days1900_ = 0; setYear( toInt(ptr+1) );

    calcDays1900();
    return true;
}


bool DateInfo::fromNumString( const char* inp, bool yrfirst )
{
    setUdf(); if ( !inp || !*inp ) return false;
    SeparString* ssptr = 0;
    if ( strchr(inp,'-') )
	ssptr = new SeparString( inp, '-' );
    else if ( strchr(inp,'/') )
	ssptr = new SeparString( inp, '/' );
    else if ( strchr(inp,'.') )
	ssptr = new SeparString( inp, '.' );
    else if ( strchr(inp,':') )
	ssptr = new SeparString( inp, ':' );
    else
	return false;
    const SeparString& ss = *ssptr;

#define mGetV(nr) \
    const char* ptr##nr = ss[nr]; \
    if ( !ptr##nr || !isdigit(*ptr##nr) ) { delete ssptr; return false; } \
    const int v##nr = toInt( ptr##nr )

    mGetV(0); mGetV(1); mGetV(2);
    delete ssptr;

    if ( yrfirst )
	{ years_ = v0; days_ = v2; }
    else
	{ years_ = v2; days_ = v0; }

    if ( days_ > 0 ) days_--;
    years_ -= 1900;
    days1900_ = 0; setMonth( v1 );

    return true;
}


void DateInfo::getUsrDisp( BufferString& disp, bool withcurtime ) const
{
    if ( isUdf() ) { disp = sKey::Undef; return; }

    disp = fullMonthName();
    disp += day(); disp += " ";
    disp += year(); disp += " ";

    if ( withcurtime )
    {
	const int cursecs = Time::getMilliSeconds() / 1000;
	const int hrs = cursecs / 3600;
	const int mins = (cursecs - hrs * 3600) / 60;
	const int secs = cursecs - hrs * 3600 - mins * 60;

	if ( hrs < 10 ) disp += "0";  disp += hrs; disp += ":";
	if ( mins < 10 ) disp += "0"; disp += mins; disp += ":";
	if ( secs < 10 ) disp += "0"; disp += secs;
    }
}
