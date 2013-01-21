#ifndef msgh_h
#define msgh_h

/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H.Bril
 Date:		19-10-1995
 Contents:	Error handler
 RCS:		$Id$
________________________________________________________________________

*/

#include "basicmod.h"
#include "callback.h"


/*!
\ingroup Basic
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


mGlobal(Basic)  void UsrMsg(const char*,MsgClass::Type t=MsgClass::Info);
//!< Will pass the message to the appropriate destination.



#endif

