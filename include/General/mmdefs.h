#ifndef mmdefs_h
#define mmdefs_h
 
/*
________________________________________________________________________
 
 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H. Lammertink
 Date:		Dec 2002
 RCS:		$Id: mmdefs.h,v 1.6 2004-03-17 14:48:25 arend Exp $
________________________________________________________________________

Defines for multimachine status tags and codes
 
*/


#define mPID_TAG	'P'
#define mPROC_STATUS	'S'
#define mCTRL_STATUS	'C'
#define mEXIT_STATUS	'E'


// Control status values
#define mSTAT_UNDEF	0
#define mSTAT_INITING	1
#define mSTAT_WORKING	2
#define mSTAT_FINISHED	3
#define mSTAT_DONE	4
#define mSTAT_PAUSED	5
#define mSTAT_ERROR	-1
#define mSTAT_KILLED	-2
#define mSTAT_TIMEOUT	-3

#define mIsOk( stat )		( stat >= 0 && stat <= mSTAT_PAUSED  )
#define mIsError( stat )	( stat < 0 && stat >= mSTAT_TIMEOUT  )


// Master Control tag
#define mRSP_WORK	'W'
#define mRSP_STOP	'S'
#define mRSP_PAUSE	'P'

#endif
