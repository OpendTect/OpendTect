#ifndef prog_h
#define prog_h

/*@+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H. Bril
 Date:		5-12-1995
 RCS:		$Id: prog.h,v 1.7 2003-09-11 15:56:00 bert Exp $
________________________________________________________________________

@$*/

#include "plugins.h"

#ifdef __cpp__

#include "transl.h"
mDefTranslatorGroups

#include "errh.h"
#include "connfact.h"

#ifndef __nosighndl__
#include <sighndl.h>
SignalHandling SignalHandling::theinst_;
#endif


extern "C" {

#endif

extern const char*	_g_pvn_();
const char*		GetProjectVersionName()		{ return _g_pvn_(); }
extern void		SetDgbApplicationCode(int);

#ifdef __cpp__
}
#endif

#endif
