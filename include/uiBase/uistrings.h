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
#include "uistring.h"

//Common strings. Use these and extend when needed

mExpClass(uiBase) uiStrings
{ mODTextTranslationClass(uiStrings);
public:
    static inline uiString sAbort()		{ return tr("&Abort"); }
    static inline uiString sAdd(bool immediate);
    static inline uiString sApply()		{ return tr("&Apply"); }
    static inline uiString sBack()		{ return tr("&< Back"); }
    static inline uiString sCancel()		{ return tr("&Cancel"); }
    static inline uiString sClose()		{ return tr("&Close"); }
    static inline uiString sContinue()		{ return tr("&Continue"); }
    static inline uiString sCopy()		{ return tr("&Copy"); }
    static inline uiString sCreate(bool immediate);
    static inline uiString sDoesNotExist();
    static inline uiString sEdit(bool immediate);
    static inline uiString sEmptyString()	{ return uiString(""); }
    static inline uiString sError()		{ return tr("Error"); }
    static inline uiString sExport()		{ return tr("&Export"); }
    static inline uiString sHelp()		{ return tr("&Help"); }
    static inline uiString sImport()		{ return tr("&Import"); }
    static inline uiString sLoad()		{ return tr("&Load ..."); }
    static inline uiString sNew(bool immediate);
    static inline uiString sNext()		{ return tr("Next &>"); }
    static inline uiString sNo()		{ return tr("&No"); }
    static inline uiString sOk()		{ return tr("&OK"); }
    static inline uiString sOpen(bool immediate);
    static inline uiString sProperties(bool immediate);
    static inline uiString sRemove(bool immediate);
    static inline uiString sRun()		{ return tr("&Run"); }
    static inline uiString sSave(bool immediate);
    static inline uiString sSaveAs()		{ return tr("Save &as ..."); }
    static inline uiString sSaveAsDefault();
    static inline uiString sSelect(bool arg=false,bool plural=false);
    static inline uiString sYes()		{ return tr("&Yes"); }
};

/*Old strings, move to uiStrings class and replace globally.
  DONT USE IN NEW CODE!
*/

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


inline uiString uiStrings::sAdd( bool immediate )
{
    return immediate ? tr("&Add") : tr("&Add ...");
}


inline uiString uiStrings::sCreate( bool immediate )
{
    return immediate ? tr("&Create") : tr("&Create ...");
}

inline uiString uiStrings::sDoesNotExist()
{ return tr( "%1 does not exist."); }


inline uiString uiStrings::sEdit( bool immediate )
{
    return immediate ? tr("&Edit") : tr("&Edit ...");
}


inline uiString uiStrings::sNew( bool immediate )
{
    return immediate ? tr("&New") : tr("&New ...");
}


inline uiString uiStrings::sOpen(bool immediate)
{ return immediate ? "&Open" : "&Open ..."; }


inline uiString uiStrings::sProperties(bool immediate)
{ return immediate ? tr("&Properties") : tr("&Properties ..."); }


inline uiString uiStrings::sRemove(bool immediate)
{ return immediate ? tr("&Remove") : tr("&Remove ..."); }


inline uiString uiStrings::sSave(bool immediate)
{ return immediate ? "&Save" : "&Save ..."; }


inline uiString uiStrings::sSelect(bool arg,bool plural)
{
    if ( !arg )
	return tr("&Select");

    return plural ? tr( "Select %1(s)" ) : tr( "Select %1" );
}



inline uiString uiStrings::sSaveAsDefault()
{ return "Save as &default"; }


#endif

