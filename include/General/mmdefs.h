#ifndef mmdefs_h
#define mmdefs_h
 
/*
________________________________________________________________________
 
 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H. Lammertink
 Date:		Dec 2002
 RCS:		$Id: mmdefs.h,v 1.1 2002-12-17 15:04:58 arend Exp $
________________________________________________________________________

Defines for multimachine status tags and codes
 
*/


#define mPID_TAG	'P'
#define mPROC_STATUS	'S'
#define mCTRL_STATUS	'C'


// Control status values
#define mSTAT_UNDEF	0
#define mSTAT_INITING	1
#define mSTAT_WORKING	2
#define mSTAT_FINISHED	3
#define mSTAT_ERROR	-1
#define mSTAT_KILLED	-2
#define mSTAT_TIMEOUT	-3


// Master Control tag
#define mRSP_ACK	'A'
#define mRSP_REQ_STOP	'S'
/*
#define mRSP_REQ_PAUSE	'P'
#define mRSP_REQ_CONT	'R'
*/

#endif
