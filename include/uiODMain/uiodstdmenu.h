#ifndef uiodstdmenu_h
#define uiodstdmenu_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bert Bril
 Date:		Dec 2003
 RCS:		$Id: uiodstdmenu.h,v 1.16 2006-04-26 09:43:22 cvsbert Exp $
________________________________________________________________________


-*/


/*
   These are the menu IDs of standard menu items in OpendTect.
   Note that all 'nodes' (i.e. items that have sub-items) are also available
   through the interface of the uiODMenuMgr directly, e.g.:
   uiPopupMenu* viewmnu = ODMainWin()->menuMgr().viewMnu();

 */


#define mFileMnu		1000
#define mProcMnu		2000
#define mWinMnu			3000
#define mViewMnu		4000
#define mUtilMnu		5000
#define mHelpMnu		1000000000

/* 'File' menu */

#define mFileSessMnu		(mFileMnu + 100)
#define mFileImpMnu		(mFileMnu + 200)
#define mFileExpMnu		(mFileMnu + 300)
#define mFileManMnu		(mFileMnu + 400)

#define mManSurveyMnuItm	(mFileMnu + 10)
#define mExitMnuItm		(mFileMnu + 20)
#define mSessSaveMnuItm		(mFileSessMnu + 10)
#define mSessRestMnuItm		(mFileSessMnu + 20)
#define mImpSeisSEGY3DMnuItm	(mFileImpMnu + 10)
#define mImpSeisSEGY2DMnuItm	(mFileImpMnu + 11)
#define mImpSeisSEGYPSMnuItm	(mFileImpMnu + 12)
#define mImpSeisCBVSMnuItm	(mFileImpMnu + 20)
#define mImpHorAsciiMnuItm	(mFileImpMnu + 30)
#define mImpWellAsciiTrackMnuItm (mFileImpMnu + 40)
#define mImpWellAsciiLogsMnuItm	(mFileImpMnu + 41)
#define mImpWellAsciiMarkersMnuItm (mFileImpMnu + 42)
#define mImpPickMnuItm		(mFileImpMnu + 50)
#define mImpLmkFaultMnuItm	(mFileImpMnu + 60)
#define mExpSeisSEGY3DMnuItm	(mFileExpMnu + 10)
#define mExpSeisSEGY2DMnuItm	(mFileExpMnu + 11)
#define mExpSeisSEGYPSMnuItm	(mFileExpMnu + 12)
#define mExpHorAsciiMnuItm	(mFileExpMnu + 20)
#define mExpPickMnuItm		(mFileExpMnu + 50)
#define mManSeisMnuItm		(mFileManMnu + 10)
#define mManHorMnuItm		(mFileManMnu + 20)
#define mManFaultMnuItm		(mFileManMnu + 30)
#define mManWellMnuItm		(mFileManMnu + 40)


/* 'Processing' menu */

#define mUseHorMnu       	(mProcMnu + 30)

#define mManAttribsMnuItm	(mProcMnu + 10)
#define mCreateVolMnuItm	(mProcMnu + 20)
#define mCreateSurfMnuItm	(mUseHorMnu + 1)
#define mCompBetweenHorMnuItm	(mUseHorMnu + 2)
#define mCompAlongHorMnuItm	(mUseHorMnu + 3)
#define mReStartMnuItm		(mProcMnu + 40)


/* 'Windows' menu */

#define mAddSceneMnuItm		(mWinMnu + 10)
#define mCascadeMnuItm		(mWinMnu + 20)
#define mTileAutoMnuItm		(mWinMnu + 30)
#define mTileHorMnuItm		(mWinMnu + 31)
#define mTileVerMnuItm		(mWinMnu + 32)


/* "View' menu */

#define mViewStereoMnu		(mViewMnu + 100)

#define mWorkAreaMnuItm		(mViewMnu + 10)
#define mZScaleMnuItm		(mViewMnu + 20)
#define mStereoOffMnuItm	(mViewStereoMnu + 10)
#define mStereoRCMnuItm		(mViewStereoMnu + 20)
#define mStereoQuadMnuItm	(mViewStereoMnu + 30)
#define mStereoOffsetMnuItm	(mViewStereoMnu + 40)


/* 'Utilities' menu */

#define mUtilSettingMnu		(mUtilMnu + 100)

#define mBatchProgMnuItm	(mUtilMnu + 10)
#define mPluginsMnuItm		(mUtilMnu + 20)
#define mCrDevEnvMnuItm		(mUtilMnu + 30)
#define mShwLogFileMnuItm	(mUtilMnu + 40)
#define mSettFontsMnuItm	(mUtilSettingMnu + 10)
#define mSettMouseMnuItm	(mUtilSettingMnu + 20)
#define mSettLkNFlMnuItm	(mUtilSettingMnu + 30)
#define mSettGeneral		(mUtilSettingMnu + 40)
#define mSettShortcutsMnuItm	(mUtilSettingMnu + 50)


/* 'Help' menu */

#define mHelpMnuBase		(mHelpMnu + 100)
#define mAboutMnuItm		(mHelpMnuBase + 0)
#define mAdminMnuItm		(mHelpMnuBase + 1)
#define mProgrammerMnuItm	(mHelpMnuBase + 2)
#define mHelpVarMnuBase		(mHelpMnuBase + 10)



#endif
