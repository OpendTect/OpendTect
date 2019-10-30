#ifndef od_helpids_h
#define od_helpids_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. TIngdahl
 Date:          March 2014
 RCS:           $Id: od_helpids.h 37068 2014-10-28 12:15:29Z nanne.hemstra@dgbes.com $
________________________________________________________________________

NOTE: This file belongs to the documentation package and is copied into
include/General at build from external/doc_csh/ at build. Any changes in the
include/General copy will be discarded at build. Make changes in the copy at
external/doc_csh instance of it.

-*/

//General 000
// General_support 01
#define	        mProgressViewerHelpID				0x00001003
#define		mBatchProgLaunchHelpID                          0x00001005
#define		mBatchHostsDlgHelpID				0x00001006
// General Setup 02
#define		mSettingsHelpID				        0x00002001
#define		mSetFontsHelpID					0x00002002
#define		mLooknFeelSettingsHelpID			0x00002003
#define		mShortcutsDlgHelpID				0x00002004
#define		mPluginManHelpID				0x00002005
#define		mPluginSelHelpID				0x00002006
#define		mODSceneMgrsetKeyBindingsHelpID			0x00002007
#define		mSurveySettingsHelpID				0x00002008
// General Survey 03
#define         mSurveyHelpID				        0x00003001
#define		mSurveyInfoEditorHelpID				0x00003002
#define		mWorkAreaDlgHelpID				0x00003004
#define		mAttribDescSetEdimportSetHelpID			0x00003005
#define		mCopySurveySIPHelpID			        0x00003006
#define		mConvertPosHelpID				0x00003007
#define		m2DDefSurvInfoDlgHelpID				0x00003008
#define		mLatLong2CoordDlgHelpID				0x00003009
#define		mGoogleExportSurveyHelpID			0x00003010
#define		mSeisWvltMangetFromOtherSurveyHelpID	        0x00003011
#define		mSurveyCompressButPushedHelpID			0x00003012
#define         mSelObjFromOtherSurveyHelpID                    0x00003013
#define         mNewSurveyByCopyHelpID                          0x00003014
#define         mStartNewSurveySetupHelpID                      0x00003015
#define         mSurveySelectDlgHelpID                          0x00003016
// General Other 04
#define		mSliceSelHelpID					0x00004001
#define		mSliceScrollHelpID				0x00004002
#define		mmcmddriverimpsHelpID			        0x00004003
#define		mODApplMgraddTimeDepthSceneHelpID		0x00004004
#define		mODApplMgrDispatchersetAutoUpdatePolHelpID	0x00004005
#define		mHandleDLSiteFailHelpID				0x00004006
#define		mProxyDlgHelpID					0x00004007
#define         mExp2DGeomHelpID                                0x00004008
#define         mLatLong2CoordFileTransDlgHelpID                0x00004009
#define         mEditSEGYFileDataDlgHelpID                      0x00004010
#define         mSEGYReadImpParsDlgHelpID                       0x00004011
#define         mSEGYStoreImpParsDlgHelpID                      0x00004012
#define         mSEGYReadFinisherHelpID                         0x00004013
#define         mhandleExistingGeometryHelpID                   0x00004014
#define         mSEGYReadStarterHelpID                          0x00004015
#define         mSingleBatchJobDispatcherParsHelpID             0x00004016
// General Installation 05
#define	        mODInstMgrHelpID				0x00005001
#define	        mODInstPkgMgrHelpID				0x00005002
#define	        mODInstMgrcheckInstDirHelpID		        0x00005003
#define	        mODInstMgrgetPackageChoiceHelpID		0x00005004
//Object-Management 008
// Object-Management Basic 00
#define		mSetDataDirHelpID				0x00800001
// Object-Management Selection 01
#define		mIOObjSelDlgHelpID				0x00801001
//Visualization 50
// Visualization 3D 00
#define		mSnapshotDlgHelpID				0x05000001
#define		mStereoDlgHelpID				0x05000002
#define		mGridLinesDlgHelpID				0x05000003
#define		mPropertiesDlgHelpID			        0x05000004
#define		mScenePropertyDlgHelpID				0x05000005
#define		mColorBarDialog					0x05000006
#define		mZScaleDlgHelpID				0x05000007
#define		mPSViewerSettingDlgHelpID			0x05000008
#define		mPrintSceneDlgHelpID			        0x05000009
#define		mWindowGrabDlgWindowHelpID			0x05000010
#define		mWindowGrabDlgDesktopHelpID			0x05000011
#define		mMultiMapperRangeEditWinHelpID		        0x05000012
#define		mSurvTopBotImageDlgHelpID			0x05000013
#define		mArrowDialogHelpID				0x05000014
#define		mMeasureDlgHelpID				0x05000015
#define		mVolrenTreeItemHelpID			        0x05000016
#define		mSeisPartServerselect2DLinesHelpID	        0x05000017
#define		mDirLightDlgHelpID				0x05000018
#define		mScenePropertyDlgLineSurfSepHelpID		0x05000019
#define		mViewer3DPositionsDlgHelpID			0x05000020
// Visualization Colors 01
#define		mColTabMarkerDlgHelpID				0x05001001
#define	        mColorTableManHelpID			        0x05001002
#define		mAutoRangeClipDlgHelpID				0x05001003
#define		mColTabImportHelpID				0x05001004
#define		mODEditAttribColorDlgHelpID			0x05001005
// Visualization Sessions 03
#define		mODMainAutoSessionDlgHelpID			0x05003001
#define		mSessionManHelpID				0x05003002
#define         mSlideLayoutDlgHelpID                           0x05003003
#define         mPresentationMakerDlgHelpID                     0x05003004
#define		mSessionSaveRestoreHelpID			0x05003005
//Flat_displays 51
// Flat_displays Display 00
#define		mODViewer2DHelpID				0x05100000
#define		mODViewer2DPropDlgHelpID			0x05100001
#define		mODViewer2DPosDlgHelpID				0x05100002
// Flat_displays 2DPSViewer 01
#define		mViewer2DPSMainWinHelpID			0x05101000
#define		mViewer2DPSPosDlgHelpID				0x05101001
#define		mViewer2DPSSelDataDlgHelpID			0x05101002
#define		mViewer2DPSMainWindisplayAngleHelpID		0x05101003
//Attributes 101
// Attributes Definition 00
#define	        mCoherencyAttrib			        0x10100000
#define		mConvolveAttribHelpID			        0x10100001
#define		mDipFilterAttribHelpID				0x10100002
#define		mEnergyAttribHelpID				0x10100003
#define		mEventAttribHelpID				0x10100004
#define		mFingerPrintAttribHelpID			0x10100005
#define		mFrequencyAttribHelpID			        0x10100006
#define		mFreqFilterAttribHelpID				0x10100007
#define		mInstantaneousAttribHelpID			0x10100008
#define		mMathAttribHelpID				0x10100009
#define		mPositionAttribHelpID			        0x10100010
#define		mReferenceAttribHelpID			        0x10100011
#define		mScalingAttribHelpID			        0x10100012
#define		mShiftAttribHelpID				0x10100013
#define		mSimilarityAttribHelpID				0x10100014
#define		mSpecDecompAttribHelpID				0x10100015
#define		mVolumeStatisticsAttribHelpID		        0x10100016
#define		mPreStackAttribHelpID			        0x10100017
#define		mWellLogAttribHelpID				0x10100018
#define		mSemblanceAttribHelpID				0x10100019
#define		mHorizonAttribHelpID			        0x10100100
#define		mGapDeconHelpID					0x10100101
#define		mMatchDeltaHelpID				0x10100102
#define		mDeltaResampleHelpID			        0x10100103
#define		mTextureAttribHelpID			        0x10100104
#define		mSampleValueAttribHelpID			0x10100105
#define         mWhereIsDotDlgHelpID                            0x10100106
#define         mGrubbsFilterAttribHelpID                       0x10100107
#define         mVolProcAttribHelpID                            0x10100108
#define         mEventFreqAttribHelpID                          0x10100109
#define		mReliefAttribHelpID				0x10100110
#define		mCEEMDAttribHelpID				0x10100111
#define		mTextureDirectionalHelpID			0x10100112
// Attributes Selection 01
#define		mAttribDescSetEdHelpID			        0x10101000
#define		mAttrSelDlgNo_NNHelpID			        0x10101001
#define		mAttrInpDlgHelpID				0x10101002
#define		mGetFileForAttrSetHelpID			0x10101003
#define		mAttrSrchProcFilesHelpID			0x10101004
#define		mAutoAttrSelDlgHelpID			        0x10101005
#define		mAttribDescSetEddefaultSetHelpID		0x10101006
#define		mTrcPositionDlgHelpID			        0x10101007
#define         mImpAttrSetHelpID                               0x10101008
// Attributes Output 02
#define		mAttrVolOutHelpID				0x10102000
#define		mRestartBatchDialogHelpID			0x10102001
#define		mClusterJobProvHelpID			        0x10102002
#define		mMultOutSelHelpID				0x10102003
#define		mAttrVolOut2DHelpID				0x10102004
// Attributes Utilities 03
#define		mAttrSetManHelpID				0x10103000
#define		mEvaluateDlgHelpID				0x10103001
#define		mFPAdvancedDlgHelpID			        0x10103002
#define		mFreqTaperDlgHelpID				0x10103003
//Seismics 103
// Seismics I/O 00
#define		mSeisImpCBVSHelpID			        0x10300001
#define		mSEGYExpMoreHelpID				0x10300002
#define		mSEGYExpTxtHeaderDlgHelpID			0x10300003
#define		mSEGYImpSimilarDlgHelpID			0x10300004
#define		mSEGYExamineHelpID				0x10300005
#define		mSEGYDefDlgHelpID				0x10300006
#define		mSEGYExpHelpID				        0x10300007
#define		mSEGYReadRev1QuestionHelpID			0x10300008
#define		mSEGYReadDlgHelpID				0x10300009
#define		mSEGYReadPreScannerHelpID			0x10300010
#define		mSeisIOSimpleImpHelpID			        0x10300011
#define		mSeisIOSimpleExpHelpID			        0x10300012
#define		mSeisPreLoadMgrHelpID			        0x10300013
#define		mSeisPreLoad2D3DDataHelpID			0x10300014
#define		mDZTImporterHelpID				0x10300016
#define		mSeisFmtScaleDlgHelpID			        0x10300017
#define		mSEGYFileManipHelpID			        0x10300019
#define		mSEGYBinHdrEdDlgHelpID			        0x10300020
#define		mSEGYFileManipHdrCalcEdHelpID		        0x10300021
#define		mSeisImpCBVSFromOtherSurveyDlgHelpID	        0x10300022
#define		mSeisExpCubePosDlgHelpID		        0x10300023
// Seismics Manage 01
#define		mSeisFileMan3DHelpID			        0x10301000
#define		mSeisCopyHelpID					0x10301001
#define		mMergeSeisHelpID				0x10301002
#define		mSeis2DManHelpID				0x10301003
#define		mSeisDump2DGeomHelpID			        0x10301004
#define		mSeisBrowserHelpID				0x10301005
#define		mPosProvSelHelpID				0x10301006
#define		mSeisMultiCubePSHelpID			        0x10301007
#define		mSeisCopyLineSetHelpID			        0x10301008
#define		mSeis2DFileManMergeDlgHelpID		        0x10301009
#define		mSeis2DExtractFrom3DHelpID			0x10301010
#define		mSeisFileMan2DHelpID			        0x10301011
#define		mGoogleExport2DSeisHelpID			0x10301012
#define		mSeis2DMultiLineSelDlgHelpID		        0x10301013
#define		m2DGeomManageDlgHelpID			        0x10301014
#define		mManageLineGeomDlgHelpID			0x10301015
#define		mGeom2DImpDlgHelpID				0x10301016
#define		mDataPointSetManHelpID			        0x10301017
#define		mDataPointSetMergerHelpID			0x10301018
#define		mBodyOperatorDlgHelpID			        0x10301019
#define		mBodyRegionDlgHelpID			        0x10301020
#define		mImplicitBodyValueSwitchDlgHelpID		0x10301021
#define		mImplBodyCalDlgHelpID			        0x10301022
// Seismics Processing 02
#define		mSeisMMProcHelpID				0x10302000
#define		mPreStackAGCHelpID			        0x10302001
#define		mPreStackMuteHelpID			        0x10302002
#define		mPreStackVerticalStackHelpID		        0x10302003
#define		mPreStackImportMuteHelpID			0x10302005
#define		mPreStackExportMuteHelpID			0x10302006
#define		mPreStackImportMuteParsHelpID		        0x10302007
#define		mImportVelFuncHelpID			        0x10302008
#define		mImportVelFuncParsHelpID			0x10302009
#define		mPreStackBatchProcSetupHelpID		        0x10302010
#define		mVolProcBatchSetupHelpID			0x10302011
#define		mBatchTime2DepthSetupHelpID			0x10302012
#define		mPreStackProcSelHelpID			        0x10302013
#define		mVelBatchVolumeConversionHelpID			0x10302014
#define		mSeisBayesPDFInpHelpID			        0x10302015
#define		mSeisBayesNormHelpID			        0x10302016
#define		mSeisBayesSeisInpHelpID				0x10302017
#define		mSeisBayesOutHelpID				0x10302018
#define		mAngleMuteComputerHelpID			0x10302019
#define		mAngleMuteHelpID				0x10302020
#define		mResortSEGYDlg			                0x10302021
#define	        mMultiWellCreateLogCubeDlg		        0x10302022
#define		mCreate2DGridHelpID				0x10302023
#define		mSeis2DTo3DHelpID				0x10302024
#define		mCreateLogCubeDlgHelpID				0x10302025
#define		mAngleCompAdvParsDlgHelpID			0x10302026
#define		mProcSettingsHelpID				0x10302027
#define		mSurfaceLimitedFillerHelpID			0x10302028
// Seismics Wavelets 03
#define		mSeisWvltManHelpID				0x10303000
#define		mSeisWvltImpHelpID				0x10303001
#define		mSeisWvltManCrWvltHelpID			0x10303002
#define		mSeisWvltImpParsHelpID			        0x10303003
#define		mWaveletExtractionHelpID			0x10303004
#define		mSeisWvltSliderDlgHelpID			0x10303005
#define		mSeisWvltTaperDlgHelpID				0x10303006
#define		mSeisWvltMergeHelpID			        0x10303007
#define		mWaveletDispPropDlgHelpID			0x10303008
// Seismics Prestack 04
#define		mSeisPrestackMan3DHelpID			0x10304000
#define		mSeisPrestackMan2DHelpID			0x10304001
#define		mPreStackCopyDlgHelpID			        0x10304002
#define		mPreStackMergeDlgHelpID				0x10304003
#define		mRayTrcParamsDlgHelpID			        0x10304004
#define         mPreStackMMProcHelpID                           0x10304005
// Seismics LINKS 05
#define		mMadagascarMainHelpID			        0x10305000
#define		mMadIOSelDlgHelpID				0x10305001
#define		mGMTMainWinHelpID				0x10305002
// Seismics Analysis 06
#define		mChainHelpID				        0x10306000
#define		mBodyFillerHelpID				0x10306002
#define		mLateralSmootherHelpID				0x10306003
#define		mVolumeSmootherHelpID				0x10306004
#define		mVelocityGridderHelpID				0x10306005
#define		mAddFunctionHelpID				0x10306006
#define		mVelocityDescDlg			        0x10306007
#define		mVolumeReaderHelpID				0x10306008
#define		mEditFunctionHelpID				0x10306009
#define		mVoxelConnectivityFilterHelpID		        0x10306010
#define		mWellLogInterpolHelpID				0x10306011
#define		mHorInterFillerHelpID				0x10306012
//Surfaces 104
// Surfaces Horizons 00
#define		mImportHorizonHelpID			        0x10400000
#define		mExportHorizonHelpID			        0x10400001
#define		mImportHorAttribHelpID			        0x10400002
#define		mChangeSurfaceDlgHelpID				0x10400003
#define		mHor3DFrom2DDlgHelpID			        0x10400005
#define		mArr2DFilterParsDlgHelpID			0x10400007
#define		mTableImpDataSel3DSurfacesHelpID		0x10400008
#define		mTableImpDataSel2DSurfacesHelpID		0x10400009
#define		mFlattenedCubeHelpID			        0x10400010
#define		mSnapToEventHelpID				0x10400011
#define		mHor2DFrom3DDlgHelpID			        0x10400012
#define		mInverseDistanceArray2DInterpolHelpID	        0x10400013
#define		mImportHorizon2DHelpID			        0x10400014
#define		mHorizonShiftDialogHelpID			0x10400015
#define		mBulkHorizonImportHelpID			0x10400016
#define		mHorizonSettingsHelpID				0x10400017
#define		mHorizonPreLoadDlgHelpID		        0x10400018
#define         mCreateHorizonHelpID                            0x10400019
#define         mgetHorizonZAxisTransformHelpID                 0x10400020
#define         mDataPointSetPickDlgHelpID                      0x10400021
#define         mIsochronMakerBatchHelpID                       0x10400022
// Surfaces Faults 01
#define		mImportFaultHelpID				0x10401000
#define		mExportFaultHelpID				0x10401001
#define		mTableImpDataSelFaultsHelpID		        0x10401002
#define		mImportFaultStick3DHelpID			0x10401003
#define		mImportFaultStick2DHelpID			0x10401004
#define		mExportFaultStickHelpID				0x10401005
#define		mTableImpDataSelFaultStickSet3DHelpID	        0x10401006
#define		mTableImpDataSelFaultStickSet2DHelpID	        0x10401007
#define		mFaultStickTransferDlgHelpID		        0x10401008
#define         mFaultOptSelHelpID                              0x10401009
#define         mBulkFaultImportHelpID				0x10401010
// Surfaces Manage 02
#define		mSurfaceManHelpID				0x10402000
#define		mSurface2DManHelpID				0x10402001
#define		mHorizonRelationsDlgHelpID			0x10402002
#define		HorizonModifyDlgHelpID			        0x10402003
#define		mFaultStickSetsManageHelpID			0x10402004
#define		mFaultsManageHelpID				0x10402005
#define		mCopySurface3DHelpID				0x10402006
#define		mCopySurface2DHelpID			        0x10402007
#define		mCopySurfaceStickSetsHelpID		        0x10402008
#define		mCopySurfaceFaultsHelpID			0x10402009
#define		mHorizonMergeDlgHelpID			        0x10402010
#define		mBodyManHelpID					0x10402011
// Surfaces Display 03
#define		mMultiSurfaceReadDlgHelpID			0x10403000
#define		mWriteSurfaceDlgHelpID			        0x10403001
#define		mContourParsDlgHelpID			        0x10403002
// Surfaces Attributes 04
#define		mAttrSurfaceOutHelpID			        0x10404000
#define		mAttrTrcSelOutBetweenHelpID			0x10404001
#define		mAttrTrcSelOutSliceHelpID			0x10404002
#define	        mStratAmpCalcHelpID			        0x10404003
#define		mIsopachMakerHelpID				0x10404004
#define		mCalcPoly2HorVolHelpID			        0x10404005
#define		mStoreAuxDataHelpID				0x10404006
#define		mHorAttr2GeomHelpID				0x10404007
#define		mHorGeom2AttrHelpID				0x10404008
#define		muiEMDataPointSetPickDlgHelpID			0x10404009
// Prestack events 05
#define		mPreStackEventExportHelpID			0x10405000
#define		mPreStackEventImportHelpID			0x10405001

//Picking 105
// Picking I/O 00
#define		mFetchPicksHelpID				0x10500000
#define		mImpPickSetHelpID			        0x10500001
#define		mExpPickSetHelpID			        0x10500002
#define		mMergePickSetsHelpID			        0x10500004
#define		mTableImpDataSelpicksHelpID		        0x10500005
#define		mPickSetManHelpID				0x10500006
// Picking Other 01
#define		mGoogleExportPolygonHelpID			0x10501000
#define		mSetPickDirsHelpID				0x10501001
//Wells 107
// Wells I/O 00
#define		mWellImportAscHelpID			        0x10700000
#define		mWellImpSegyVspHelpID			        0x10700001
#define		mWellImportAscDataSelHelpID			0x10700002
#define		mD2TModelGroupHelpID			        0x10700003
#define		mWellImpPptDlgHelpID			        0x10700004
#define         mNewWellTrackDlgHelpID				0x10700005
#define		mSimpleMultiWellCreateHelpID		        0x10700007
#define		mSimpleMultiWellCreateReadDataHelpID	        0x10700008
#define		mTableImpDataSelwellsHelpID		        0x10700009
#define		mBulkTrackImportHelpID			        0x10700010
#define		mBulkLogImportHelpID			        0x10700011
#define		mBulkMarkerImportHelpID				0x10700012
#define         mBulkD2TModelImportHelpID                       0x10700013
// Wells Manage 01
#define		mWellManHelpID				        0x10701000
#define		mMarkerDlgHelpID				0x10701001
#define		mImportLogsHelpID				0x10701002
#define		mExportLogsHelpID				0x10701003
#define		mReadMarkerFileHelpID			        0x10701004
#define		mD2TModelDlgHelpID				0x10701005
#define		mD2TModelReadDlgHelpID			        0x10701006
#define		mWellTrackDlgHelpID				0x10701007
#define		mWellTrackReadDlgHelpID				0x10701008
#define		mTableImpDataSelmarkersHelpID		        0x10701009
#define		mWellLogCalcHelpID				0x10701010
#define		mGoogleExportWellsHelpID			0x10701011
#define		mWellLogCalcRockPhysHelpID			0x10701012
#define         mWellLogEditorHelpID                            0x10701013
#define         mWellRockPhysLauncherHelpID                     0x10701014
// Wells Display 02
#define		mWellDispPropDlgHelpID			        0x10702000
#define		mDispEditMarkerDlgHelpID			0x10702001
#define         mMarkerViewDlgHelpID                            0x10702002
#define         mWellMarkersDlgHelpID                           0x10702003
#define         mSynthCorrAdvancedDlgHelpID                     0x10702004
// Wells Attributes 03
#define		mCreateAttribLogDlgHelpID			0x10703000
#define		mWellLogToolWinMgrHelpID			0x10703001
// Wells Well_Ties 04
#define		mWellTiMgrDlemgHelpID			        0x10704000
#define		mWellTieTieWinHelpID			        0x10704001
#define		mWellTieInfoDlgHelpID			        0x10704002
#define		mWellTieSaveDataDlgHelpID			0x10704003
#define		mCheckShotEditHelpID			        0x10704004
//Tracking 108
// Trakcing I/O 00
#define	        mTrackingWizardHelpID			        0x10800000
#define		mTrackingSetupGroupHelpID			0x10800001

//Random_lines 109
// Random_lines General 00
#define		mWell2RandomLineDlgHelpID			0x10900000
#define		mGenRanLinesByContourHelpID			0x10900001
#define		mGenRanLinesByShiftHelpID			0x10900002
#define		mGenRanLinesFromPolygonHelpID		        0x10900003
#define		mODRandomLineTreeItemHelpID			0x10900004
#define		mSeisRandTo2DLineDlgHelpID			0x10900005
#define		mRandomLinePolyLineDlgHelpID		        0x10900006
#define		mGoogleExportRandomLineHelpID		        0x10900007
#define		mWellto2DLineDlgHelpID				0x10900008
#define		mRandomLineManHelpID				0x10900009
//Stratigraphy 110
// Stratigraphy General 00
#define		mStratTreeWinHelpID				0x11000000
#define		mStratUnitDlgHelpID				0x11000001
#define		mStratLevelDlgHelpID			        0x11000002
#define		mStratLinkLvlUnitDlgHelpID			0x11000003
#define		mStratLithoDlgHelpID			        0x11000004
#define		mStratContentsDlgHelpID				0x11000005
#define		mStratSynthExportHelpID				0x11000008
#define		mStartSynthOutSelHelpID				0x11000009
#define		mStratSynthLayerModFRPPropSelectorHelpID	0x11000010
#define		mStratEditLayerHelpID			        0x11000011
#define		mStratSimpleLayerModDispHelpID		        0x11000012
#define         mStratUnitDivideDlgHelpID                       0x11000013
// Stratigraphy Properties 01
#define		mManPROPSHelpID					0x11001000
#define		mEditPropRefHelpID				0x11001001
#define		mSelectPropRefsHelpID			        0x11001002
#define		mElasticPropSelDlgHelpID			0x11001003
// Stratigraphy BasicModelling 02
#define		mSingleLayerGeneratorEdHelpID		        0x11002000
#define		mSynthToRealScaleHelpID				0x11002001
#define		mStratLayerModelcheckUnscaledWaveletHelpID	0x11002002
#define         mStratLayerModelDispIOHelpID                    0x11002003
// Stratigraphy Crossplotting 03
#define		mStratSynthCrossplotHelpID			0x11003000
#define		mSingleAttribEdHelpID			        0x11003001
#define		mLaySeqAttribEdHelpID			        0x11003002
//Crossplots 111
// Crossplots General 00
#define		mDataPointSetHelpID				0x11100000
#define		mdataPointSetSaveHelpID				0x11100001
#define		mDataPointSetCrossPlotterPropDlgHelpID	        0x11100002
#define		mCreateDPSPDFHelpID				0x11100003
#define		mSelectionSettDlgHelpID				0x11100004
#define		mDPSOverlayPropDlgHelpID			0x11100005
#define		mSelColorDlgHelpID				0x11100006
#define		mImpPVDSHelpID				        0x11100007
#define		mTableImpDataSelpvdsHelpID		        0x11100008
#define		mExpSelectionAreaHelpID				0x11100009
#define		mReadSelGrpHelpID				0x11100010
#define		mDPSSelectednessDlgHelpID			0x11100011
#define		mVariogramDlgHelpID				0x11100012
#define		mVariogramDisplayHelpID				0x11100013
#define		mOpenCossplotHelpID				0x11100014
// Crossplots Extraction 01
#define		mAttribCrossPlotHelpID			        0x11101000
#define		mWellAttribCrossPlotHelpID			0x11101001
#define		mPosFilterSetSelHelpID			        0x11101002
//Probability_density_functions 112
// Probability_density_functions_I/O 0
#define		mImpRokDocPDFHelpID				0x11200000
#define		mExpRokDocPDFHelpID				0x11200001
// Probability_density_functions Manage 1
#define		mProbDenFuncManHelpID			        0x11201000
#define		mEditProbDenFuncHelpID			        0x11201001
#define		mProbGenFuncGenHelpID			        0x11201002
#define         mProbDenFuncGenSampledHelpID                    0x11201003
#define         mProbDenFuncGenGaussianHelpID                   0x11201004
//SynthRock 113
#define		mRockPhysFormHelpID				0x11300000
#define		mMathPropEdDlgHelpID				0x11300001
#define         mrockPhysReqHelpID                              0x11300002
//NLA 114
#define		mPrepNLADataHelpID				0x11400000
#define		mLithCodeManHelpID				0x11400001

#endif
