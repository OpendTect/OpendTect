#ifndef errh_H
#define errh_H

/*@+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H.Bril
 Date:		19-10-1995
 Contents:	Error handler
 RCS:		$Id: errh.h,v 1.1.1.2 1999-09-16 09:18:56 arend Exp $
________________________________________________________________________

@$*/


#include <Vector.h>
#include <fixstring.h>


#define mMaxErrorMsgLength 160
typedef FixedString<mMaxErrorMsgLength+1> ErrMsgString;

class ErrH
{
public:
			ErrH()	{ curmsg = -1; }
    const char*		msg() const
			{ return curmsg >= 0 ? (const char*)errs[curmsg]
						     : (const char*)empty; }
    const char*		prevMsg()
			{
			    ((ErrH*)this)->curmsg--;
			    return curmsg > -1  ? (const char*)errs[curmsg]
						: (const char*)empty;
			}

    void		addMsg( const char* s )
			{
			    if ( s ) errs.push_back( ErrMsgString(s) );
			    curmsg = errs.size() - 1;
			}
    void		clear()
			{ errs.erase(); }

    static ErrH*	  curErrH;
    static FixedString<2> empty;

private:
    Vector<ErrMsgString>  errs;
    int			  curmsg;
};

inline void ErrMsg( const char* msg )
{
    if ( ErrH::curErrH ) ErrH::curErrH->addMsg( msg );
    else		 cerr << msg << endl;
}

inline const char* getErrMsg()
{
    return ErrH::curErrH ? (const char*)ErrH::curErrH->msg()
			 : (const char*)ErrH::empty;
}

#ifdef __prog__
    ErrH*		ErrH::curErrH = 0;
    FixedString<2>	ErrH::empty;
#endif


/*$-*/
#endif
