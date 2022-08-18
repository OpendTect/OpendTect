#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "commondefs.h"

class uiString;

mGlobal(Basic) void ErrMsg(const char*,bool progr=false);
mGlobal(Basic) void ErrMsg(const uiString&);

#include <typeinfo>
template <class T>
inline const char* className( const T& t )
{	//!< Also works for gcc that returns the size first e.g. 4Clss
    const char* nm = typeid(t).name();
    while ( *nm >= '0' && *nm <= '9' ) nm++;
    return nm;
}


namespace OD {
      mGlobal(Basic) void programmerErrMsg(const char* msg,const char* cname,
					   const char* fnm,int linenr);
      mGlobal(Basic) void SetGlobalLogFile(const char*);
}

# define pErrMsg(msg) \
    OD::programmerErrMsg(msg,::className(*this),__FILE__,__LINE__)
    //!< Usual access point for programmer error messages

#define pErrMsgOnce(msg) \
{ \
    mDefineStaticLocalObject( bool, __message_shown__, = false ); \
    if ( !__message_shown__ ) \
    { \
	__message_shown__ = true; \
	pErrMsg(msg); \
    } \
}


# define pFreeFnErrMsg(msg) \
    OD::programmerErrMsg( msg, __func__, __FILE__, __LINE__ )
    //!< Usual access point for programmer error messages in free functions
