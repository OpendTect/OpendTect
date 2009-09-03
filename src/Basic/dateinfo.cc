/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : 12-3-1996
 * FUNCTION : date info
-*/
 
static const char* rcsID = "$Id: dateinfo.cc,v 1.9 2009-09-03 11:33:56 cvsbert Exp $";

#include "dateinfo.h"
#include "timefun.h"
#include "keystrs.h"
#include "separstr.h"
#include <time.h>

static int normdaycount[] = { 31,59,90,120,151,181,212,243,273,304,334,365 };
static int leapdaycount[] = { 31,60,91,121,152,182,213,244,274,305,335,366 };

DefineEnumNames(DateInfo,Day,2,"Week day") {
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


DateInfo::DateInfo()
{
    days96_ = (time(0) - 820454400L)/86400L;
    calcDMY();
}


DateInfo::DateInfo( int yr, int mn, int dy )
	: year_(yr-1996)
	, month_((Month)(mn-1))
	, day_(dy-1)
{
    calcDays96();
}


DateInfo::DateInfo( int yr, DateInfo::Month m, int dy )
	: year_(yr-1996)
	, month_(m)
	, day_(dy-1)
{
    calcDays96();
}


DateInfo::DateInfo( int yr, const char* mn, int dy )
	: year_(yr-1996)
	, month_(eEnum(DateInfo::Month,mn))
	, day_(dy-1)
{
    calcDays96();
}


void DateInfo::setDay( int dy )
{
    day_ = dy - 1;
    if ( isUdf() ) return;
    calcDays96();
}


void DateInfo::setMonth( DateInfo::Month mn )
{
    month_ = mn;
    if ( isUdf() ) return;
    calcDays96();
}


void DateInfo::setYear( int yr )
{
    year_ = yr - 1996;
    if ( isUdf() ) return;
    calcDays96();
}


DateInfo& DateInfo::operator +=( int dys )
{
    if ( !isUdf() )
    {
	days96_ += dys;
	calcDMY();
    }
    return *this;
}


DateInfo& DateInfo::operator -=( int dys )
{
    if ( !isUdf() )
    {
	days96_ -= dys;
	calcDMY();
    }
    return *this;
}


void DateInfo::addMonths( int mns )
{
    if ( isUdf() || mns < 1 ) return;

    int nr = (int)month_ + mns;
    if ( mns > 0 )
    {
	year_ += nr / 12;
	month_ = (Month)(nr % 12);
    }
    else
    {
	if ( nr >= 0 )
	    month_ = (Month)nr;
	else
	{
	    year_ += nr / 12 - 1;
	    nr = (-nr) % 12;
	    month_ = (Month)(12 - nr);
	}
    }
    calcDays96();
}


const char* DateInfo::weekDayName() const
{
    int nr = days96_ % 7 + 1;
    if ( nr > 6 ) nr = 0;
    return eString(Day,nr);
}


int DateInfo::daysInMonth( int yr, DateInfo::Month m )
{
    int* d = yr % 4 ? normdaycount : leapdaycount;
    int nr = d[m];
    if ( m != Jan ) nr -= d[m-1];
    return nr;
}


void DateInfo::getDayMonth( int yr, int ndays, int& dy, DateInfo::Month& mn )
{
    int* d = yr % 4 ? normdaycount : leapdaycount;
    int mnth = 0;
    while ( ndays >= d[mnth] ) mnth++;
    dy = mnth ? ndays - d[mnth-1] : ndays;
    mn = (Month)mnth;
}


void DateInfo::calcDMY()
{
    int yrqtets = days96_ / 1461;
    int qtetrest = days96_ % 1461;
    year_ = 4 * yrqtets;
    if ( qtetrest > 1095 )	{ year_ += 3; qtetrest -= 1096; }
    else if ( qtetrest > 730 )	{ year_ += 2; qtetrest -= 731; }
    else if ( qtetrest > 365 )	{ year_ += 1; qtetrest -= 366; }

    getDayMonth( year_, qtetrest, day_, month_ );
}


void DateInfo::calcDays96()
{
    days96_ = (year_/4) * 1461;
    int rest = year_ % 4;
    if ( rest )	{ days96_ += 366; rest--; }
    if ( rest )	{ days96_ += 365; rest--; }
    if ( rest )	days96_ += 365;
    if ( month_ != Jan )
    {
	int* d = year_ % 4 ? normdaycount : leapdaycount;
	days96_ += d[((int)month_)-1];
    }
    days96_ += day_;
}


static BufferString buf;

const char* DateInfo::whenRelative( const DateInfo* di ) const
{

    if ( di )	getRel( *di );
    else	getRelToday();

    return buf.buf();
}


#define mRet(s) { buf = s; return; }

void DateInfo::getRel( const DateInfo& reld ) const
{
    if ( isUdf() || reld.isUdf() )
	mRet(sKey::Undef)
    else if ( reld.days96_ == days96_ )
	mRet("that day")

    const int diff = days96_ - reld.days96_;
    buf.setEmpty();
    if ( diff < 0 )
    {
	if ( diff == -1 ) mRet("the previous day")
	if ( diff == -7 ) mRet("one week earlier")
	if ( diff == -14 ) mRet("two weeks earlier")
	if ( diff == -21 ) mRet("three weeks earlier")
	if ( diff > -21 ) { buf += (-diff); buf += " days earlier"; return; }
    }
    else
    {
	if ( diff == 1 ) mRet("the next day")
	if ( diff == 7 ) mRet("one week later")
	if ( diff == 14 ) mRet("two weeks later")
	if ( diff == 21 ) mRet("three weeks later")
	if ( diff < 21 ) { buf += diff; buf += " days later"; return; }
    }

    if ( reld.month_ == month_ && reld.day_ == day_ )
    {
	buf = "exactly ";
	int difference = year_ - reld.year_;
	buf += abs(difference);
	buf += " year";
	if ( abs(difference) != 1 ) buf += "s";
	buf += difference > 0 ? " later" : " earlier";
	return;
    }

    if ( reld.year_ == year_ )
    {
	int difference = month_ - reld.month_;
	if ( difference > -2 && difference < 2 )
	{
	    if ( reld.day_ == day_ )
		mRet(difference<0?"one month earlier":"one month later")
	    buf = "the ";
	    addDay();
	    buf += " of ";
	    buf += difference ? (difference > 0 ? "the following"
		    				: "the previous") : "that";
	    buf += " month";
	    return;
	}
    }

    buf = monthName();
    buf += " the ";
    addDay();
    if ( reld.year_ != year_ ) { buf += ", "; buf += year(); }
}


void DateInfo::getRelToday() const
{
    if ( isUdf() ) mRet(sKey::Undef)

    DateInfo today;

    if ( today.days96_ == days96_ ) mRet("today")
    int diff = days96_ - today.days96_;
    if ( diff < 0 )
    {
	if ( diff == -1 ) mRet("yesterday")
	if ( diff == -2 ) mRet("the day before yesterday")
	if ( diff == -7 ) mRet("One week ago")
	if ( diff == -14 ) mRet("two weeks ago")
	if ( diff == -21 ) mRet("three weeks ago")
	if ( diff > -21 ) { buf = (-diff); buf += " days ago"; return; }
    }
    else
    {
	if ( diff == 1 ) mRet("tomorrow")
	if ( diff == 2 ) mRet("the day after tomorrow")
	if ( diff == 7 ) mRet("next week")
	if ( diff == 14 ) mRet("two weeks from now")
	if ( diff == 21 ) mRet("three weeks from now")
	if ( diff < 21 ) { buf = (-diff); buf += " days from now"; return; }
    }

    if ( today.month_ == month_ && today.day_ == day_ )
    {
	buf = "exactly ";
	int difference = year_ - today.year_;
	buf += abs(difference);
	buf += " years ";
	buf += difference > 0 ? "from now" : "ago";
	return;
    }

    if ( today.year_ == year_ )
    {
	int difference = month_ - today.month_;
	if ( difference > -2 && difference < 2 )
	{
	    if ( today.day_ == day_ )
		mRet(difference<0?"one month ago":"one month from now")
	    buf = "the ";
	    addDay();
	    buf += " of ";
	    buf += difference ? (difference > 0 ? "next" : "last") : "this";
	    buf += " month";
	    return;
	}
    }

    buf = day(); buf += " ";
    buf += monthName();
    if ( today.year_ != year_ ) { buf += " "; buf += year(); }
}


void DateInfo::addDay() const
{
    buf += day();
    int swdy = day_ > 12 ? day_%10 : day_;
    switch ( swdy )
    {
    case 0: buf += "st"; break; case 1: buf += "nd"; break;
    case 2: buf += "rd"; break; default: buf += "th"; break;
    }
}


void DateInfo::toString( BufferString& str ) const
{
    if ( isUdf() )
	{ str = sKey::Undef; return; }

    str.setEmpty(); str += day();

    str += "-";
    buf = eString(DateInfo::Month,month()+1); buf[3] = '\0';
    buf[0] = tolower( buf[0] );
    str += buf;

    str += "-"; str += year();
}


bool DateInfo::fromString( const char* str )
{
    setUdf(); if ( !str || !*str ) return false;
    SeparString ss( str, '-' );
    const char* dayptr = ss[0];
    if ( !isdigit(*dayptr) ) return false;

    day_ = atoi( dayptr );
    month_ = eEnum(DateInfo::Month,ss[1]);   
    year_ = atoi( ss[2] );
    calcDays96();
    return true;
}


void DateInfo::getFullDisp( BufferString& disp, bool withtime ) const
{
    if ( isUdf() ) { disp = sKey::Undef; return; }

    disp = eString(DateInfo::Month,month());
    *(disp.buf() + 3) = ' '; *(disp.buf() + 4) = '\0';
    disp += day(); disp += " ";
    disp += year(); disp += " ";

    if ( withtime )
    {
	const int cursecs = Time_getMilliSeconds() / 1000;
	const int hrs = cursecs / 3600;
	const int mins = (cursecs - hrs * 3600) / 60;
	const int secs = cursecs - hrs * 3600 - mins * 60;

	if ( hrs < 10 ) disp += "0";  disp += hrs; disp += ":";
	if ( mins < 10 ) disp += "0"; disp += mins; disp += ":";
	if ( secs < 10 ) disp += "0"; disp += secs;
    }
}
