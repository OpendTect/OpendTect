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
    uistrings_ += mEnumTr("Peak or trough");
    uistrings_ += mEnumTr("Peak (Maximum)");
    uistrings_ += mEnumTr("Trough (Minimum)");
    uistrings_ += mEnumTr("Zero crossing");
    uistrings_ += mEnumTr("Zero crossing - to +");
    uistrings_ += mEnumTr("Zero crossing + to -");
    uistrings_ += mEnumTr("Largest peak");
    uistrings_ += mEnumTr("Largest trough");
}
