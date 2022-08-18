#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "basicmod.h"
#include "callback.h"


/*!
\brief Class to encapsulate a message to the user.

  Along with the message there's also a type. In any case, there's a handler
  for when UsrMsg is called: theCB. If it is not set, messages go to cerr.
*/

mExpClass(Basic) MsgClass : public CallBacker
{
public:

    enum Type		{ Info, Message, Warning, Error, ProgrammerError };

			MsgClass( const char* s, Type t=Info )
			: msg(s), type_(t)		{}

    const char*		msg;
    Type		type_;

    static CallBack&	theCB( const CallBack* cb=0 );
			//!< pass non-null to set the CB
    static const char*	nameOf(Type);

};


mGlobal(Basic) void UsrMsg(const char*,MsgClass::Type t=MsgClass::Info);
//!< Will pass the message to the appropriate destination.
