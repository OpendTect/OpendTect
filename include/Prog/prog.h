#ifndef prog_h
#define prog_h

/*@+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H. Bril
 Date:		5-12-1995
 RCS:		$Id: prog.h,v 1.1.1.1 1999-09-03 10:11:41 dgb Exp $
________________________________________________________________________

@$*/

#include <gendefs.h>

#ifdef __cpp__

#include <errh.h>
#include <connfact.h>
extern "C" {

#endif

extern const char*	_g_pvn_();
const char*		GetProjectVersionName()		{ return _g_pvn_(); }

#ifdef __cpp__
}
#endif

#endif
