#ifndef mmdefs_h
#define mmdefs_h
 
/*
________________________________________________________________________
 
 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H. Lammertink
 Date:		Dec 2002
 RCS:		$Id: mmdefs.h,v 1.9 2005-03-30 11:19:22 cvsarend Exp $
________________________________________________________________________

Defines for multimachine status tags and codes
 
*/


#define mPID_TAG	'P'
#define mPROC_STATUS	'S'
#define mCTRL_STATUS	'C'
#define mEXIT_STATUS	'E'
#define mERROR_MSG	'e'


// Control status values
#define mSTAT_UNDEF	0
#define mSTAT_WORKING	1
#define mSTAT_FINISHED	2
#define mSTAT_DONE	3
#define mSTAT_PAUSED	4
#define mSTAT_JOBERROR	-1
#define mSTAT_HSTERROR	-2
#define mSTAT_KILLED	-3
#define mSTAT_TIMEOUT	-4

#define mIsOk( stat )		( stat >= 0 && stat <= mSTAT_PAUSED  )
#define mIsError( stat )	( stat < 0 || stat > mSTAT_PAUSED  )


// Master Control tag
#define mRSP_WORK	'W'
#define mRSP_STOP	'S'
#define mRSP_PAUSE	'P'

#endif
