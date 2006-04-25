#ifndef msgh_h
#define msgh_h

/*
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H.Bril
 Date:		19-10-1995
 Contents:	Error handler
 RCS:		$Id: msgh.h,v 1.8 2006-04-25 16:53:11 cvsbert Exp $
________________________________________________________________________

*/

#include "callback.h"
#include "enums.h"


/*!\brief class to encapsulate a message to the user.

Along with the message there's also a type. In any case, there's a handler
for when UsrMsg is called: theCB. If it is not set, messages go to cerr.

*/

class MsgClass : public CallBacker
{
public:

    enum Type		{ Info, Message, Warning, Error, ProgrammerError };
			DeclareEnumUtilsWithVar(Type,type)

			MsgClass( const char* s, Type t=Info )
			: msg(s), type_(t)		{}

    const char*		msg;

    static CallBack&	theCB( const CallBack* cb=0 );
    			//!< pass non-null to set the CB

};


void UsrMsg(const char*,MsgClass::Type t=MsgClass::Info);
//!< Will pass the message to the appropriate destination.



#endif
