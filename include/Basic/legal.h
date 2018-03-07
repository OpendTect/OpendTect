#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Mar 2006
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


mGlobal(Basic) ::Factory0Param<uiString>& legalInformation();

mGlobal(Basic) uiString gplV3Text();
//!<Gnu Public License V3 text

mGlobal(Basic) uiString lgplV3Text();
//!<Gnu Lesser Public License Text. Includes GPL text
