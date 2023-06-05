#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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

mGlobal(Basic) uiString* legalText(const char* libnm_or_legaltxtpath);
//!<If libnm, reads legal text from data/Legal/libnm/LICENSE.txt

mGlobal(Basic) uiString gplV3Text();
//!<Gnu Public License V3 text

mGlobal(Basic) uiString lgplV3Text();
//!<Gnu Lesser Public License Text. Includes GPL text
