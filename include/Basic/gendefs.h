#ifndef gendefs_H
#define gendefs_H

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H.Bril
 Date:		1-9-1995
 Contents:	General definitions for every module
 RCS:		$Id: gendefs.h,v 1.34 2005-02-23 14:45:12 cvsarend Exp $
________________________________________________________________________

 This file contains general defines that are so basic they can be used in
 any source file.

-*/


#define mODMajorVersion		2
#define mODMinorVersion		1

#include "plfdefs.h"

#define mMaxUserIDLength	127

#define mSWAP(x,y,tmp)		{ tmp = x; x = y; y = tmp; }
#define mNINT(x)		( (int)((x)>0 ? (x)+.5 : (x)-.5) )
#define mMAX(x,y)		( (x)>(y) ? (x) : (y) )
#define mMIN(x,y)		( (x)<(y) ? (x) : (y) )

#define mIsZero(x,eps)		( (x) < (eps) && (x) > (-eps) )
#define mIsEqual(x,y,eps)	( (x-y) < (eps) && (x-y) > (-eps) )
#define mIsEqualRel(x,y,e)	( (y) ? ((x)/(y))-1<(e) && ((x)/(y)-1)>(-e) \
				      : mIsZero(x,e) )
#define mDefEps			(1e-10)

#ifdef FORCE_NEW_UNDEF
#define mUndefValue		_obsolete_see__undefval_h__
#define mIsUndefined(x)		_obsolete_see__undefval_h__
#define sUndefValue		_obsolete_use__sKey__FloatUdf__see_keystrs_h
#define mUndefIntVal		_obsolete_see__undefval_h__
#define mIsUndefInt(x)		_obsolete_see__undefval_h__
#else
/* TODO: remove once all modules are converted to new undef handling */
#define mUndefValue		1e30
#define mIsUndefined(x)		(((x)>9.99999e29)&&((x)<1.00001e30))
#define sUndefValue		"1e30"
#define mUndefIntVal		2109876543
#define mIsUndefInt(x)		((x) == mUndefIntVal)
#endif

#undef	YES
#define	YES	1
#undef	NO
#define	NO	0


#define mMALLOC(sz,tp)		(tp*)malloc((sz)*sizeof(tp))
#define mREALLOC(var,sz,tp)	(tp*)realloc(var,(sz)*sizeof(tp))
#define mFREE(ptr)		{ if (ptr) free(ptr); ptr = 0; }

#ifdef PATH_LENGTH
# undef PATH_LENGTH
#endif

#ifdef __win__
# include <stdio.h>
#endif

#ifdef __msvc__

# include <stdlib.h>
# include <windefs.h>
# define PATH_LENGTH			_MAX_PATH

# define mPolyRet(base,clss)		base
# define mTFriend(T,clss)
# define mTTFriend(T,C,clss)
# define mProtected			public
# define mPolyRetDownCast(clss,var)	dynamic_cast<clss>(var)
# define mPolyRetDownCastRef(clss,var)	*(dynamic_cast<clss*>(&var))
# define mDynamicCastGet(typ,out,in) \
	 typ out = 0; try { out = dynamic_cast< typ >( in ); } catch (...) {}

#else

#define PATH_LENGTH			255

# define mPolyRet(base,clss)		clss
# define mTFriend(T,clss)		template <class T> friend class clss
# define mTTFriend(T,C,clss)	template <class T, class C> friend class clss
# define mProtected			protected
# define mPolyRetDownCast(clss,var)	var
# define mPolyRetDownCastRef(clss,var)	var
# define mDynamicCastGet(typ,out,in)	typ out = dynamic_cast< typ >( in );

#endif


/*!\mainpage Basic utilities
  \section Introduction Introduction

  This module handles all things that are so basic to all other modules that
  they can be seen as a layer of common services to the entire system. One of
  the tasks is to provide platform-independence of file, date&time, threads,
  and more.

  The difference with the 'General' module was, traditionally, that Basic
  utilities could in principle be insteresting outside OpendTect. This
  distinction is not enforced (see e.g. the survey info class). We place
  things in Basic that feel more basic than General. Note that General depends
  on Basic, not vice versa.

  You'll find that many of these tools can be found in other packages in some
  form or another. But, the problems of management of dependencies are usually
  much bigger than having to maintain a bunch of lean-and-mean specially made
  objects that do selected things in a way that fits our system.


  \section Content Content

  We'll just name a few groups of services. There are many more isolated
  useful objects, defines, functions and so forth.

<ul>
 <li>Sets/Lists
  <ul>
   <li>sets.h : 'The' classes for sets of objects and pointers to objects
   <li>sortedlist.h and sortedtable.h : sets that are sorted during build
   <li>toplist.h : holds a "top N" list
   <li>iopar.h : IOPar is a keyword-value lookup list that is used as 'generic'
       parameter list throughout OpendTect.
  </ul>
 <li>Strings
  <ul>
   <li>string2.h : things not in the standard <string.h>
   <li>bufstring.h and bufstringset.h : Variable length strings commonly used in
       OpendTect with a guaranteed minimum buffer size. That makes them ideal
       as bridge with C strings.
   <li>compoundkey.h and multiid.h : dot-separated keys.
   <li>fixstring.h: fixed length strings but with tools like '+=' .
  </ul>
 <li>System-wide service objects
  <ul>
   <li>msgh.h, errh.h and debug.h : Message pushing without user interface
   <li>settings.h : access to persistent user specific settings
   <li>survinfo.h : access to the survey setup (names, positions, ranges)
  </ul>
 <li>Positions (coordinates, inlines/crosslines=BinIDs)
  <ul>
   <li>position.h and posgeom.h : basic map position tools
   <li>binidexcl.h, binidprov.h, binidsel.h, binidselimpl.h: 
       BinID selection and iteration
  </ul>
 <li>File and stream handling
  <ul>
   <li>filegen.h : basic file tools like existence, path operations, copy, etc.
   <li>ascstream.h : read and write of the typical OpendTect standard Ascii data
   <li>dirlist.h : list contents of a directory
   <li>strmprov.h and strmdata.h : access files, pipes, or devices for read
       or write and make sure they are closed correctly.
  </ul>
 <li>CallBacks
  <ul>
   <li>callback.h : our (simple but powerful) event system
  </ul>
 <li>Template floating-point algorithms
  <ul>
   <li>simpnumer.h, sorting.h, periodicvalue.h, genericnumer.h, extremefinder.h
  </ul>
</ul>

*/

#endif
