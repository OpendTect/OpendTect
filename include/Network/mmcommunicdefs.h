#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

// Defines for MM job communication status tags and codes

#define mPID_TAG	'P'
#define mPROC_STATUS	'S'
#define mCTRL_STATUS	'C'
#define mEXIT_STATUS	'E'
#define mERROR_MSG	'e'


// Control status values
#define mSTAT_UNDEF	0
#define mSTAT_WORKING	1
#define mSTAT_WRAPUP	2
#define mSTAT_FINISHED	3
#define mSTAT_ALLDONE	4
#define mSTAT_PAUSED	5
#define mSTAT_JOBERROR	-1
#define mSTAT_HSTERROR	-2
#define mSTAT_KILLED	-3
#define mSTAT_TIMEOUT	-4

#define mIsOk( stat )		( stat >= 0 && stat <= mSTAT_PAUSED  )
#define mIsError( stat )	( stat < 0 || stat > mSTAT_PAUSED  )


// Primary Host Control tag
#define mRSP_WORK	'W'
#define mRSP_STOP	'S'
#define mRSP_PAUSE	'P'
#define mRSP_UNDEF	'U'
