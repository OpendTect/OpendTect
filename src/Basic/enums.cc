/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Jan 2011
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "enums.h"

#include "string2.h"

EnumDef::EnumDef( const char* nm, const char* s[], short nrs )
    : NamedObject(nm)
    , names_(s)
    , nrsign_(nrs)	{}

bool EnumDef::isValidName( const char* s ) const
{ return getIndexInStringArrCI(s,names_,0,nrsign_,-1) >= 0; }


int EnumDef::convert( const char* s ) const
{ return getIndexInStringArrCI(s,names_,0,nrsign_,0); }


const char* EnumDef::convert( int i ) const
{ return names_[i]; }

int EnumDef::size() const
{ int i=0; while ( names_[i] ) i++; return i; }
