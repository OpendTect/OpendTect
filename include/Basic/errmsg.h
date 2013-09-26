#ifndef errmsg_h
#define errmsg_h

/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Sep 2013
 RCS:		$Id$
________________________________________________________________________

*/

#ifndef commondefs_h
# include "commondefs.h"
#endif
#ifndef __cpp__
# define ErrMsg(x,y) fprintf(stderr,"%s",x)
# define pErrMsg(msg)
# define pFreeFnErrMsg(msg,fn)
#else


mGlobal(Basic) void ErrMsg(const char*,bool progr=false);


#ifdef __debug__


  namespace OD {
      mGlobal(Basic) void programmerErrMsg(const char* msg,const char* cname,
					   const char* fnm,int linenr);
}

#include <typeinfo>
template <class T>
inline const char* className( const T& t )
{	//!< Also works for gcc that returns the size first e.g. 4Clss
    const char* nm = typeid(t).name();
    while ( *nm >= '0' && *nm <= '9' ) nm++;
    return nm;
}

# define pErrMsg(msg) \
    OD::programmerErrMsg(msg,::className(*this),__FILE__,__LINE__)
    //!< Usual access point for programmer error messages

# define pFreeFnErrMsg(msg,fn) \
    OD::programmerErrMsg( msg, fn, __FILE__, __LINE__ )
    //!< Usual access point for programmer error messages in free functions

#else

# define pErrMsg(msg)
# define pFreeFnErrMsg(msg,fn)

#endif

#endif // __cpp__

#endif
