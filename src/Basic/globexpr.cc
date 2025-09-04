/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "globexpr.h"
#include <ctype.h>

GlobExpr::GlobExpr( const char* s, OD::CaseSensitivity cs )
    : expr_("")
    , errmsg_(nullptr)
    , cs_(cs)
{
    set(s);
}


GlobExpr::GlobExpr( const GlobExpr& ge )
    : expr_(ge.expr_)
    , errmsg_(nullptr)
    , cs_(ge.cs_)
{}


GlobExpr& GlobExpr::operator=( const GlobExpr& ge )
{
    expr_ = ge.expr_;
    errmsg_ = nullptr;
    cs_ = ge.cs_;
    return *this;
}


bool GlobExpr::operator==( const GlobExpr& ge ) const
{
    return expr_ == ge.expr_ && cs_ == ge.cs_;
}


void GlobExpr::set( const char* str )
{
    expr_ = str;
    errmsg_ = 0;

    // remove possible trailing backslash
    char* ptr = expr_.getCStr();
    while ( *ptr )
    {
	if ( !*(ptr+1) && *ptr == '\\' )
	    *ptr = '\0';
	ptr++;
    }
}


bool GlobExpr::matches( const char* p, const char* t, const char*& errmsg,
			OD::CaseSensitivity cs )
{
    errmsg = 0;
    if ( !t || !*t ) return !*p;

    for ( ; *p; p++, t++ )
    {
	/* if this is the end of the text then this is the end of the match */
	if ( !*t )
	    return *p == '*' && *++p == '\0';

	switch ( *p )
	{
	case '?':	break;
	case '*':	return starMatches( p, t, errmsg, cs );

	case '[':
	{
	    p++;

	    bool inverted = false;
	    if ( *p == '!' || *p == '^')
		{ inverted = true; p++; }

	    if ( *p == ']' )
		{ errmsg = "Missing expression inside '[]'"; return false; }

	    bool member_matched = false;
	    bool go_on = true;
	    char range_start, range_end;

	    while ( go_on )
	    {
		if ( *p == ']' )
		    { go_on = false; continue; }

		/* matching a '!', '^', '-', '\' or a ']' */
		if ( *p == '\\' )
		    range_start = range_end = *++p;
		else
		    range_start = range_end = *p;
		if ( !range_start )
		    { errmsg = "Missing ']'"; return false; }

		// check for range bar
		if ( *++p == '-' )
		{
		    range_end = *++p;
		    if ( range_end == '\0' || range_end == ']' )
			{ errmsg = "Missing 'Y' in '[X-Y]'"; return false; }

		    // special character range end
		    if ( range_end == '\\' ) range_end = *++p;

		    // move just beyond this range
		    p++;
		}

		/* if the text character is in range then match found.
		   make sure the range letters have the proper
		   relationship to one another before comparison */
		if ( (*t >= range_start && *t <= range_end)
		  || (*t >= range_end && *t <= range_start) )
		    { member_matched = true; go_on = false; }
	    }

	    /* if there was a match in an exclusion set then no match */
	    /* if there was no match in a member set then no match */
	    if (  (inverted && member_matched)
	      || !(inverted || member_matched) )
		return false;

	    /* if this is not an exclusion then skip the rest of the [...]
		construct that already matched. */
	    if ( member_matched )
	    {
		while ( *p != ']' )
		{
		    if ( !*p )
			{ errmsg = "Missing ']'"; return false; }

		    /* skip exact match */
		    if (*p == '\\') {
			p++;
		    }

		    /* move to next pattern char */
		    p++;
		}
	    }

	    break;
	}

	    /* next character is quoted and must match exactly */
	    case '\\':

		/* move pattern pointer to quoted char and fall through */
		p++;

	    /* must match this character exactly */
	    default:
		if ( cs == OD::CaseSensitive )
		{
		    if ( *p != *t )
			return false;
		}
		else
		{
		    if ( toupper(*p) != toupper(*t) )
			return false;
		}
	}
    }

    /* if end of text not reached then the pattern fails */
    return !*t;
}


bool GlobExpr::starMatches( const char* p, const char* t, const char*& errmsg,
			    OD::CaseSensitivity cs )
{
    /* pass over existing ? and * in pattern */
    while ( *p == '?' || *p == '*' )
    {

	/* take one char for each ? */
	if ( *p == '?' )
	{

	    /* if end of text then no match */
	    if ( !*t++ )
		return false;
	}

	/* move to next char in pattern */
	p++;
    }

    /* if end of pattern we have matched regardless of text left */
    if ( !*p )
	return true;

    /* get the next character to match which must be a literal or '[' */
    char nextp = *p;
    if ( nextp == '\\' )
	nextp = p[1];

    /* Continue until we run out of text or definite result seen */
    bool matched = false, needmatched;
    while ( !matched )
    {
	needmatched = nextp == '[';
	if ( !needmatched )
	    needmatched = cs==OD::CaseInsensitive ? toupper(nextp)==toupper(*t)
						  : nextp == *t;
	if ( needmatched )
	    matched = matches( p, t, errmsg, cs );

	/* if the end of text is reached then no match */
	if ( !*t++ )
	    break;
    }

    return matched;
}


void GlobExpr::validateFilterString( BufferString& filt )
{
    if ( filt.isEmpty() )
	filt = "*";
    else if ( !filt.contains("*") )
    {
	filt.insertAt( 0, "*" );
	filt.add( "*" );
    }
}
