#ifndef uiodstdmenu_h
#define uiodstdmenu_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bert Bril
 Date:		Dec 2003
 RCS:		$Id: uiodstdmenu.h,v 1.1 2003-12-20 13:24:05 bert Exp $
________________________________________________________________________


-*/


/* These are the menu IDs of standard menu items in OpendTect.
   Note that all 'nodes' (i.e. items that have sub-items) are also available
   through the interface of ui3DApplication directly, e.g.:
   uiPopupMenu* viewmnu = dTectMainWin()->viewMenu();
 */


#define mFileMnu		1000
#define mProcMnu		2000
#define mWinMnu			3000
#define mViewMnu		4000
#define mUtilMnu		5000
#define mHelpMnu		6000

/* 'File' menu */

#define mFileSessMnu		(mFileMnu + 100)
#define mFileImpMnu		(mFileMnu + 200)
#define mFileExpMnu		(mFileMnu + 300)
#define mFileManMnu		(mFileMnu + 400)

#define mManSurveyMnuItm	(mFileMnu + 10)
#define mExitMnuItm		(mFileMnu + 20)
#define mSessSaveMnuItm		(mFileSessMnu + 10)
#define mSessRestMnuItm		(mFileSessMnu + 20)
#define mImpSeisSEGYMnuItm	(mFileImpMnu + 10)
#define mImpSeisCBVSMnuItm	(mFileImpMnu + 20)
#define mImpHorAsciiMnuItm	(mFileImpMnu + 30)
#define mImpWellAsciiMnuItm	(mFileImpMnu + 40)
#define mImpPickMnuItm		(mFileImpMnu + 50)
#define mImpLmkFaultMnuItm	(mFileImpMnu + 60)
#define mExpSeisSEGYMnuItm	(mFileExpMnu + 10)
#define mExpHorAsciiMnuItm	(mFileExpMnu + 20)
#define mManSeisMnuItm		(mFileManMnu + 10)
#define mManHorMnuItm		(mFileManMnu + 20)
#define mManWellMnuItm		(mFileManMnu + 30)


/* 'Processing' menu */

#define mManAttribsMnuItm	(mProcMnu + 10)
#define mCreateVolMnuItm	(mProcMnu + 20)
#define mReStartMnuItm		(mProcMnu + 30)


/* 'Windows' menu */

#define mAddSceneMnuItm		(mWinMnu + 10)
#define mCascadeMnuItm		(mWinMnu + 20)
#define mTileMnuItm		(mWinMnu + 30)


/* "View' menu */

#define mViewStereoMnu		(mViewMnu + 100)

#define mWorkAreaMnuItm		(mViewMnu + 10)
#define mZScaleMnuItm		(mViewMnu + 20)
#define mStereoOffMnuItm	(mViewStereoMnu + 10)
#define mStereoRCMnuItm		(mViewStereoMnu + 20)
#define mStereoQuadMnuItm	(mViewStereoMnu + 30)
#define mStereoOffsetMnuItm	(mViewStereoMnu + 40)


/* 'Utilities' menu */

#define mUtilsSettingMnu	(mUtilsMnu + 100)

#define mBatchProgMnuItm	(mUtilsMnu + 10)
#define mPluginsMnuItm		(mUtilsMnu + 20)
#define mCrDevEnvMnuItm		(mUtilsMnu + 30)
#define mSettFontsMnuItm	(mUtilsSettingMnu + 10)
#define mSettMouseMnuItm	(mUtilsSettingMnu + 20)


/* 'Help' menu */

#define mStdHelpMnuBase		(mHelpMnu + 90)
#define mAdminMnuItm		(mStdHelpMnuBase + 1)
#define mProgrammerMnuItm	(mStdHelpMnuBase + 2)
#define mAboutMnuItm		(mStdHelpMnuBase + 3)



#endif
