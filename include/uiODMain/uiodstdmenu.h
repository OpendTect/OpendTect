#ifndef uiodstdmenu_h
#define uiodstdmenu_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bert Bril
 Date:		Dec 2003
 RCS:		$Id: uiodstdmenu.h,v 1.43 2008-06-26 11:06:35 cvsumesh Exp $
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
#define mSessAutoMnuItm		(mFileSessMnu + 30)
#define mImpSeisSEGY3DMnuItm	(mFileImpMnu + 10)
#define mImpSeisSEGY2DMnuItm	(mFileImpMnu + 11)
#define mImpSeisSEGYPS3DMnuItm	(mFileImpMnu + 12)
#define mImpSeisSEGYPS2DMnuItm	(mFileImpMnu + 13)
#define mImpSeisSimple3DMnuItm	(mFileImpMnu + 14)
#define mImpSeisSimple2DMnuItm	(mFileImpMnu + 15)
#define mImpSeisSimplePS3DMnuItm (mFileImpMnu + 16)
#define mImpSeisSimplePS2DMnuItm (mFileImpMnu + 17)
#define mImpSeisCBVSMnuItm	(mFileImpMnu + 20)
#define mImpHorAsciiMnuItm	(mFileImpMnu + 30)
#define mImpHorAsciiAttribMnuItm (mFileImpMnu + 31)
#define mImpHor2DAsciiMnuItm	(mFileImpMnu + 32)
#define mImpWellAsciiTrackMnuItm (mFileImpMnu + 40)
#define mImpWellAsciiLogsMnuItm	(mFileImpMnu + 41)
#define mImpWellAsciiMarkersMnuItm (mFileImpMnu + 42)
#define mImpFaultMnuItm		(mFileImpMnu + 50)
#define mImpPickMnuItm		(mFileImpMnu + 60)
#define mImpWvltMnuItm		(mFileImpMnu + 70)
#define mExpSeisSEGY3DMnuItm	(mFileExpMnu + 10)
#define mExpSeisSEGY2DMnuItm	(mFileExpMnu + 11)
#define mExpSeisSimple3DMnuItm	(mFileExpMnu + 14)
#define mExpSeisSimple2DMnuItm	(mFileExpMnu + 15)
#define mExpHorAscii3DMnuItm	(mFileExpMnu + 20)
#define mExpHorAscii2DMnuItm	(mFileExpMnu + 21)
#define mExpFltAsciiMnuItm	(mFileExpMnu + 30)
#define mExpPickMnuItm		(mFileExpMnu + 50)
#define mManSeisMnuItm		(mFileManMnu + 10)
#define mManHor3DMnuItm		(mFileManMnu + 20)
#define mManHor2DMnuItm		(mFileManMnu + 21)
#define mManFaultMnuItm		(mFileManMnu + 30)
#define mManWellMnuItm		(mFileManMnu + 40)
#define mManPickMnuItm		(mFileManMnu + 50)
#define mManWvltMnuItm		(mFileManMnu + 60)
#define mManAttrMnuItm		(mFileManMnu + 70)
#define mManNLAMnuItm		(mFileManMnu + 80)
#define mManSessMnuItm		(mFileManMnu + 90)
#define mManStratMnuItm		(mFileManMnu + 95)


/* 'Processing' menu */

#define mUseHorMnu       	(mProcMnu + 30)

#define mEditAttrMnuItm		(mProcMnu + 10)
#define mEdit2DAttrMnuItm	(mProcMnu + 11)
#define mEdit3DAttrMnuItm	(mProcMnu + 12)
#define mSeisOutMnuItm		(mProcMnu + 20)
#define mSeisOut2DMnuItm	(mProcMnu + 21)
#define mSeisOut3DMnuItm	(mProcMnu + 22)
#define mCreateSurf2DMnuItm	(mUseHorMnu + 1)
#define mCompBetweenHor2DMnuItm	(mUseHorMnu + 2)
#define mCompAlongHor2DMnuItm	(mUseHorMnu + 3)
#define mCreateSurf3DMnuItm	(mUseHorMnu + 4)
#define mCompBetweenHor3DMnuItm	(mUseHorMnu + 5)
#define mCompAlongHor3DMnuItm	(mUseHorMnu + 6)
#define mXplotMnuItm		(mProcMnu + 40)
#define mReStartMnuItm		(mProcMnu + 50)


/* 'Windows' menu */

#define mAddSceneMnuItm		(mWinMnu + 10)
#define mCascadeMnuItm		(mWinMnu + 20)
#define mTileAutoMnuItm		(mWinMnu + 30)
#define mTileHorMnuItm		(mWinMnu + 31)
#define mTileVerMnuItm		(mWinMnu + 32)
#define mSceneSelMnuItm		(mWinMnu + 40)


/* "View' menu */

#define mWorkAreaMnuItm		(mViewMnu + 10)
#define mZScaleMnuItm		(mViewMnu + 20)

#define mViewStereoMnu		(mViewMnu + 100)

#define mStereoOffMnuItm	(mViewStereoMnu + 10)
#define mStereoRCMnuItm		(mViewStereoMnu + 20)
#define mStereoQuadMnuItm	(mViewStereoMnu + 30)
#define mStereoOffsetMnuItm	(mViewStereoMnu + 40)

#define mViewIconsMnuItm	(mViewMnu + 200)


/* 'Utilities' menu */

#define mUtilSettingMnu		(mUtilMnu + 100)

#define mBatchProgMnuItm	(mUtilMnu + 10)
#define mPluginsMnuItm		(mUtilMnu + 20)
#define mPosconvMnuItm		(mUtilMnu + 25)
#define mCrDevEnvMnuItm		(mUtilMnu + 30)
#define mShwLogFileMnuItm	(mUtilMnu + 40)
#define mDumpDataPacksMnuItm	(mUtilMnu + 99)
#define mSettFontsMnuItm	(mUtilSettingMnu + 10)
#define mSettMouseMnuItm	(mUtilSettingMnu + 20)
#define mSettLkNFlMnuItm	(mUtilSettingMnu + 30)
#define mSettGeneral		(mUtilSettingMnu + 40)
#define mSettShortcutsMnuItm	(mUtilSettingMnu + 50)


/* 'Help' menu */

#define mHelpMnuBase		(mHelpMnu + 100)
#define mAdminMnuItm		(mHelpMnuBase + 1)
#define mProgrammerMnuItm	(mHelpMnuBase + 2)
#define mHelpAboutMnuBase	(mHelpMnuBase + 10)
#define mHelpVarMnuBase		(mHelpMnuBase + 100)



#endif
