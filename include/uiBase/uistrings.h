#ifndef uistrings_h
#define uistrings_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          25/08/1999
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uibasemod.h"
#include "fixedstring.h"

//Common strings used in OD. Should be extended

inline FixedString sApply()			{ return "&Apply"; }
inline FixedString sCancel()			{ return "&Cancel"; }
inline FixedString sClose()			{ return "&Close"; }
inline FixedString sHelp()			{ return "&Help"; }
inline FixedString sLoad()			{ return "&Load ..."; }
inline FixedString sNew()			{ return "&New"; }
inline FixedString sNo()			{ return "&No"; }
inline FixedString sOk()			{ return "&OK"; }
inline FixedString sOpen(bool immediate=false);
inline FixedString sRun()			{ return "&Run"; }
inline FixedString sSave(bool immediate=true);
inline FixedString sSaveAs()			{ return "Save &as ..."; }
inline FixedString sSaveAsDefault();
inline FixedString sYes()			{ return "&Yes"; }


//Implementations
inline FixedString sOpen(bool immediate)
{ return immediate ? "&Open" : "&Open ..."; }


inline FixedString sSave(bool immediate)
{ return immediate ? "&Save" : "&Save ..."; }


inline FixedString sSaveAsDefault()
{ return "Save as &default"; }



#endif

