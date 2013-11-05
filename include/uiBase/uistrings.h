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

inline FixedString sCancel()			{ return "&Cancel"; }
inline FixedString sOk()			{ return "&Ok"; }
inline FixedString sOpen(bool immediate=false);
inline FixedString sSave(bool immediate=true);
inline FixedString sSaveAs()			{ return "Save &as ..."; }
inline FixedString sSaveAsDefault();


//Implementations
inline FixedString sOpen(bool immediate)
{ return immediate ? "&Open" : "&Open ..."; }


inline FixedString sSave(bool immediate)
{ return immediate ? "&Save" : "&Save ..."; }


inline FixedString sSaveAsDefault()
{ return "Save as &default"; }



#endif

