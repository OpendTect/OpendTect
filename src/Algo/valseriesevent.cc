/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : May 2005
-*/


#include "valseriesevent.h"
#include "uistrings.h"

mDefineEnumUtils(VSEvent,Type,"Event type")
{
	"None",
	"Peak or trough",
	"Peak (Max)",
	"Trough (Min)",
	"Zero crossing",
	"Zero crossing - to +",
	"Zero crossing + to -",
	"Largest peak",
	"Largest trough",
	0
};

template<>
void EnumDefImpl<VSEvent::Type>::init()
{
    uistrings_ += uiStrings::sNone();
    uistrings_ += mEnumTr("Peak or trough",0);
    uistrings_ += mEnumTr("Peak (Maximum)",0);
    uistrings_ += mEnumTr("Trough (Minimum)",0);
    uistrings_ += mEnumTr("Zero crossing",0);
    uistrings_ += mEnumTr("Zero crossing - to +",0);
    uistrings_ += mEnumTr("Zero crossing + to -",0);
    uistrings_ += mEnumTr("Largest peak",0);
    uistrings_ += mEnumTr("Largest trough",0);
}
