#ifndef legal_h
#define legal_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H.Bril
 Date:		Mar 2006
 RCS:		$Id$
________________________________________________________________________


-*/

#include "basicmod.h"
#include "factory.h"

/*!Code that is reworked from other open source projects is
   sometimes required to display copyright information as
   well as discalmers.

   Such messages are handled through a factory, where messages
   can be added. To do so, a static function that returns
   the message has to be implemented, and given to the
   factory at startup:

\code
   static uiString* legalInfo()
   {
       uiString* res = new uiString;
       *res = toUiString(
	       "Copyright (C) Myself\n"
	       "Full legal info");
       return res;
   }

   MyClass::initClass()
   {
       legalInformation().addCreator( legalInfo, "My code" );
   }
\endcode

   The text is then visible to users at the Help->Legal menu
   of OpendTect
*/


mGlobal(Basic) ::Factory<uiString>& legalInformation();

#endif
