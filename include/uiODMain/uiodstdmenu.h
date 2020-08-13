#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:		Bert
 Date:			Dec 2003
________________________________________________________________________

-*/


/*
   These are the menu IDs of standard menu items in OpendTect.
   Note that all 'nodes' (i.e. items that have sub-items) are only available
   through the interface of the uiODMenuMgr directly, e.g.:
   uiMenu* viewmnu = ODMainWin()->menuMgr().viewMnu();

 */


#define mFileMnu			1000
#define mProcMnu			3000
#define mWinMnu				4000
#define mViewMnu			5000
#define mUtilMnu			6000
#define mAppMnu				7000
#define mHelpMnu			1000000000

/* 'File' menu */

#define mFileSessMnu			(mFileMnu + 100)
#define mFileImpMnu			(mFileMnu + 200)
#define mFileExpMnu			(mFileMnu + 500)
#define mFileManMnu			(mFileMnu + 800)
#define mFilePreLoadMnu			(mFileMnu + 1100)

#define mRestartMnuItm			(mFileMnu + 20)
#define mExitMnuItm			(mFileMnu + 21)

#define mSessSaveMnuItm			(mFileSessMnu + 10)
#define mSessRestMnuItm			(mFileSessMnu + 20)
#define mSessAutoMnuItm			(mFileSessMnu + 30)

#define mImpSeisSimpleMnu		(mFileImpMnu + 10)
#define mImpSeisSimple2DMnuItm		(mImpSeisSimpleMnu + 0)
#define mImpSeisSimplePS2DMnuItm	(mImpSeisSimpleMnu + 1)
#define mImpSeisSimple3DMnuItm		(mImpSeisSimpleMnu + 2)
#define mImpSeisSimplePS3DMnuItm	(mImpSeisSimpleMnu + 3)
#define mImpSeisODCubeMnuItm		(mFileImpMnu + 15)
#define mImpSeisODCubeOtherSurvMnuItm	(mFileImpMnu + 16)
#define mImpHorAsciiMnu			(mFileImpMnu + 20)
#define mImpHor2DAsciiMnuItm		(mImpHorAsciiMnu + 0)
#define mImpBulkHor2DAsciiMnuItm	(mFileImpMnu + 1)
#define mImpHor3DAsciiAttribMnuItm	(mFileImpMnu + 2)
#define mImpBulkHor3DAsciiMnuItm	(mFileImpMnu + 3)
#define mImpHor3DAsciiMnuItm		(mFileImpMnu + 4)
#define mImpWellAsciiTrackMnuItm	(mFileImpMnu + 30)
#define mImpWellAsciiLogsMnuItm		(mFileImpMnu + 31)
#define mImpWellAsciiMarkersMnuItm	(mFileImpMnu + 32)
#define mImpWellSimpleMultiMnuItm	(mFileImpMnu + 33)
#define mImpBulkWellTrackItm		(mFileImpMnu + 34)
#define mImpBulkWellLogsItm		(mFileImpMnu + 35)
#define mImpBulkWellMarkersItm		(mFileImpMnu + 36)
#define mImpBulkWellD2TItm		(mFileImpMnu + 37)
#define mImpFltAsciiMnuItm		(mFileImpMnu + 40)
#define mImpBulkFltAsciiMnuItm		(mFileImpMnu + 41)
#define mImpFltSSAsciiMnu		(mFileImpMnu + 50)
#define mImpFltSSAscii2DMnuItm		(mImpFltSSAsciiMnu + 0)
#define mImpFltSSAscii3DBulkMnuItm	(mImpFltSSAsciiMnu + 1)
#define mImpFltSSAscii3DMnuItm		(mImpFltSSAsciiMnu + 2)
#define mImpFltSSAscii2DBulkMnuItm	(mImpFltSSAsciiMnu + 3)
#define mImpFltSetAsciiMnuItm		(mFileImpMnu + 54)
#define mImpPickAsciiMnuItm		(mFileImpMnu + 60)
#define mImpWvltAsciiMnuItm		(mFileImpMnu + 70)
#define mImpMuteDefAsciiMnuItm		(mFileImpMnu + 80)
#define mImpPDFAsciiMnuItm		(mFileImpMnu + 90)
#define mImpVelocityAsciiMnuItm		(mFileImpMnu + 100)
#define mImpXPlotAsciiMnuItm		(mFileImpMnu + 110)
#define mImpAttrAsciiMnuItm		(mFileImpMnu + 130)
#define mImpAttrOthSurvMnuItm		(mFileImpMnu + 131)
#define mImpColTabMnuItm		(mFileImpMnu + 200)
#define mImpGeom2DAsciiMnuItm		(mFileImpMnu + 210)

#define mExpSeisSimpleMnu		(mFileExpMnu + 10)
#define mExpSeisSimple2DMnuItm		(mExpSeisSimpleMnu + 0)
#define mExpSeisSimplePS2DMnuItm	(mExpSeisSimpleMnu + 1)
#define mExpSeisSimple3DMnuItm		(mExpSeisSimpleMnu + 2)
#define mExpSeisSimplePS3DMnuItm	(mExpSeisSimpleMnu + 3)
#define mExpSeisCubePositionsMnuItm	(mExpSeisSimpleMnu + 9)
#define mExpHorAsciiMnu			(mFileExpMnu + 20)
#define mExpHor2DAsciiMnuItm		(mExpHorAsciiMnu + 0)
#define mExpBulkHor2DAsciiMnuItm	(mExpHorAsciiMnu + 1)
#define mExpHor3DAsciiMnuItm		(mExpHorAsciiMnu + 2)
#define mExpBulkHor3DAsciiMnuItm	(mExpHorAsciiMnu + 3)
#define mExpFltAsciiMnuItm		(mFileExpMnu + 40)
#define mExpBulkFltAsciiMnuItm		(mFileExpMnu + 41)
#define mExpFltSSAsciiMnuItm		(mFileExpMnu + 50)
#define mExpBulkFltSSAsciiMnuItm	(mFileExpMnu + 51)
#define mExpFltSetAsciiMnuItm		(mFileExpMnu + 55)
#define mExpPickAsciiMnuItm		(mFileExpMnu + 60)
#define mExpPolyAsciiMnuItm		(mFileExpMnu + 61)
#define mExpWvltAsciiMnuItm		(mFileExpMnu + 70)
#define mExpMuteDefAsciiMnuItm		(mFileExpMnu + 80)
#define mExpPDFAsciiMnuItm		(mFileExpMnu + 90)
#define mExpGeom2DAsciiMnuItm		(mFileExpMnu + 120)

#define mManSurveyMnuItm		(mFileManMnu + 0)
#define mManSeisMnu			(mFileManMnu + 10)
#define mManSeis2DMnuItm		(mManSeisMnu + 0)
#define mManSeisPS2DMnuItm		(mManSeisMnu + 1)
#define mManSeis3DMnuItm		(mManSeisMnu + 2)
#define mManSeisPS3DMnuItm		(mManSeisMnu + 3)
#define mManHor2DMnuItm			(mFileManMnu + 20)
#define mManHor3DMnuItm			(mFileManMnu + 21)
#define mManWellMnuItm			(mFileManMnu + 30)
#define mManFltMnuItm			(mFileManMnu + 40)
#define mManFltSSMnuItm			(mFileManMnu + 50)
#define mManFaultSetMnuItm		(mFileManMnu + 55)
#define mManPickMnuItm			(mFileManMnu + 60)
#define mManWvltMnuItm			(mFileManMnu + 70)
#define mManPDFMnuItm			(mFileManMnu + 90)
#define mManXPlotMnuItm			(mFileManMnu + 110)
#define mManGeom2DMnuItm		(mFileManMnu + 120)
#define mManAttr2DMnuItm		(mFileManMnu + 130)
#define mManAttr3DMnuItm		(mFileManMnu + 131)
#define mManBodyMnuItm			(mFileManMnu + 140)
#define mManSessMnuItm			(mFileManMnu + 150)
#define mManNLAMnuItm			(mFileManMnu + 160)
#define mManStratMnuItm			(mFileManMnu + 170)
#define mManPropsMnuItm			(mFileManMnu + 180)
#define mManRanLMnuItm			(mFileManMnu + 190)
#define mManColTabMnuItm		(mFileManMnu + 200)

#define mPreLoadSeisMnuItm		(mFilePreLoadMnu + 10)
#define mPreLoadHorMnuItm		(mFilePreLoadMnu + 11)


/* 'Analysis' menu */

#define mXPlotMnuItm			(mAppMnu + 110)
#define mAttrXPlotMnuItm		(mAppMnu + 111)
#define mOpenXPlotMnuItm		(mAppMnu + 112)
#define mEditAttrMnuItm			(mAppMnu + 130)
#define mEdit2DAttrMnuItm		(mAppMnu + 131)
#define mEdit3DAttrMnuItm		(mAppMnu + 132)


/* 'Processing' menu */

#define mSeisOutMnuItm			(mProcMnu + 10)
#define mSeisOut2DMnuItm		(mProcMnu + 11)
#define mSeisOut3DMnuItm		(mProcMnu + 12)
#define mCreate2DFrom3DMnuItm		(mProcMnu + 13)
#define mExtract2DFrom3DMnuItm		(mProcMnu + 14)
#define mCreate3DFrom2DMnuItm		(mProcMnu + 15)
#define mUseHorMnu			(mProcMnu + 20)
#define mCreateSurf2DMnuItm		(mUseHorMnu + 1)
#define mCompBetweenHor2DMnuItm		(mUseHorMnu + 2)
#define mCompAlongHor2DMnuItm		(mUseHorMnu + 3)
#define mCreateSurf3DMnuItm		(mUseHorMnu + 4)
#define mCompBetweenHor3DMnuItm		(mUseHorMnu + 5)
#define mCompAlongHor3DMnuItm		(mUseHorMnu + 6)
#define mPSProc2DMnuItm			(mProcMnu + 300)
#define mPSProc3DMnuItm			(mProcMnu + 301)
#define mVolProc2DMnuItm		(mProcMnu + 400)
#define mVolProc3DMnuItm		(mProcMnu + 401)
#define mT2DConv2DMnuItm		(mProcMnu + 410)
#define mT2DConv3DMnuItm		(mProcMnu + 411)
#define mStartBatchJobMnuItm		(mProcMnu + 500)


/* 'Scenes' menu */

#define mAddSceneMnuItm			(mWinMnu + 10)
#define mAddMapSceneMnuItm		(mWinMnu + 11)
#define mAddTimeDepth2DMnuItm		(mWinMnu + 12)
#define mAddTimeDepth3DMnuItm		(mWinMnu + 13)
#define mAddHorFlat2DMnuItm		(mWinMnu + 14)
#define mAddHorFlat3DMnuItm		(mWinMnu + 15)
#define mCascadeMnuItm			(mWinMnu + 20)
#define mTileAutoMnuItm			(mWinMnu + 30)
#define mTileHorMnuItm			(mWinMnu + 31)
#define mTileVerMnuItm			(mWinMnu + 32)
#define mScenePropMnuItm		(mWinMnu + 40)
#define mSceneSelMnuItm			(mWinMnu + 50)


/* "View' menu */

#define mWorkAreaMnuItm			(mViewMnu + 10)
#define mZScaleMnuItm			(mViewMnu + 20)
#define m2DViewMnuItm			(mViewMnu + 30)

#define mViewStereoMnu			(mViewMnu + 100)

#define mStereoOffMnuItm		(mViewStereoMnu + 1)
#define mStereoRCMnuItm			(mViewStereoMnu + 2)
#define mStereoQuadMnuItm		(mViewStereoMnu + 3)
#define mStereoOffsetMnuItm		(mViewStereoMnu + 4)


/* 'Utilities' menu */

#define mUtilSettingMnu			(mUtilMnu + 100)

#define mBatchProgMnuItm		(mUtilMnu + 10)
#define mSetupBatchItm			(mUtilMnu + 15)
#define mGraphicsInfoItm		(mUtilMnu + 16)
#define mFirewallProcItm		(mUtilMnu + 17)
#define mHostIDInfoItm			(mUtilMnu + 18)
#define mPluginsMnuItm			(mUtilMnu + 20)
#define mPosconvMnuItm			(mUtilMnu + 25)
#define mCRSPosconvMnuItm		(mUtilMnu + 26)
#define mCrDevEnvMnuItm			(mUtilMnu + 30)
#define mShwLogFileMnuItm		(mUtilMnu + 40)
#define mInstMgrMnuItem			(mUtilMnu + 50)
#define mInstAutoUpdPolMnuItm		(mUtilMnu + 51)
#define mInstConnSettsMnuItm		(mUtilMnu + 52)
#define mDisplayMemoryMnuItm		(mUtilMnu + 98)
#define mDumpDataPacksMnuItm		(mUtilMnu + 99)
#define mSettingsMnuItm			(mUtilSettingMnu + 10)
#define mSettAdvPersonal		(mUtilSettingMnu + 20)
#define mSettAdvPython			(mUtilSettingMnu + 21)
#define mSettAdvSurvey			(mUtilSettingMnu + 22)
#define mSettLanguageMnu		(mUtilSettingMnu + 100)


/* 'Help' menu */

#define mHelpMnuBase			(mHelpMnu + 100)
#define mUserDocMnuItm			(mHelpMnuBase + 1)
#define mAdminDocMnuItm			(mHelpMnuBase + 2)
#define mProgrammerDocMnuItm		(mHelpMnuBase + 3)
#define mAboutMnuItm			(mHelpMnuBase + 4)
#define mSupportMnuItm			(mHelpMnuBase + 5)
#define mWorkflowsMnuItm		(mHelpMnuBase + 6)
#define mTrainingManualMnuItm		(mHelpMnuBase + 7)
#define mAttribMatrixMnuItm		(mHelpMnuBase + 8)
#define mShortcutsMnuItm		(mHelpMnuBase + 9)
#define mLegalMnuItm			(mHelpMnuBase + 10)
