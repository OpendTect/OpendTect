#ifndef msgh_h
#define msgh_h

/*
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H.Bril
 Date:		19-10-1995
 Contents:	Error handler
 RCS:		$Id: msgh.h,v 1.2 2001-02-13 17:15:46 bert Exp $
________________________________________________________________________

*/

#include <callback.h>
#include <enums.h>


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

    static CallBack	theCB;

};


inline void UsrMsg( const char* msg, MsgClass::Type t=MsgClass::Info )
//!< Will pass the message to the appropriate destination.
{
    if ( !MsgClass::theCB.willCall() )
	cerr << msg << endl;
    else
    {
	MsgClass obj( msg, t );
	MsgClass::theCB.doCall( &obj );
    }
}


#ifdef __prog__

CallBack MsgClass::theCB;
DefineEnumNames(MsgClass,Type,1,"Message type")
	{ "Information", "Message", "Warning", "Error", "PE", 0 };

#endif


#endif
