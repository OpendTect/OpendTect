#ifndef prog_h
#define prog_h

/*@+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H. Bril
 Date:		5-12-1995
 RCS:		$Id: prog.h,v 1.2 2001-07-06 11:41:43 bert Exp $
________________________________________________________________________

@$*/

#include <gendefs.h>

#ifdef __cpp__

#include <errh.h>
#include <connfact.h>
#include <sighndl.h>

SignalHandling SignalHandling::theinst_;


extern "C" {

#endif

extern const char*	_g_pvn_();
const char*		GetProjectVersionName()		{ return _g_pvn_(); }

#ifdef __cpp__
}
#endif

#endif
