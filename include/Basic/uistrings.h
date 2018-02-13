#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          25/08/1999
________________________________________________________________________

-*/

#include "basicmod.h"
#include "fixedstring.h"
#include "uistring.h"

#define mPlural 2

/*!Common strings that are localized. Using these keeps the translation
   at a minimum.
*/
mExpClass(Basic) uiStrings
{ mODTextTranslationClass(uiStrings);
public:

    static uiPhrase phrAdd(const uiWord&);
    static uiPhrase phrASCII(const uiWord&);
    static uiPhrase phrCalculate(const uiWord&);
    static uiPhrase phrCalculateFrom(const uiWord&);
    static uiPhrase phrCannotAdd(const uiWord&);
    static uiPhrase phrCannotCompute(const uiWord&);
    static uiPhrase phrCannotCopy(const uiWord&);
    static uiPhrase phrCannotCreate(const uiWord&);
    static uiPhrase phrCannotCreateDBEntryFor(const uiWord&);
    static uiPhrase phrCannotCreateDirectory(const uiWord&);
    static uiPhrase phrCannotEdit(const uiWord&);
    static uiPhrase phrCannotExtract(const uiWord&);
    static uiPhrase phrCannotFind(const uiWord&);
    static uiPhrase phrCannotFindDBEntry(const uiWord&);
    static uiPhrase phrCannotImport(const uiWord&);
    static uiPhrase phrCannotLoad(const uiWord&);
    static uiPhrase phrCannotOpen(const uiWord&);
    static uiPhrase phrCannotParse(const uiWord&);
    static uiPhrase phrCannotRead(const uiWord&);
    static uiPhrase phrCannotSave(const uiWord&);
    static uiPhrase phrCannotRemove(const uiWord&);
    static uiPhrase phrCannotUnZip(const uiWord&);
    static uiPhrase phrCannotZip(const uiWord&);
    static uiPhrase phrCannotWrite(const uiWord&);
    static uiPhrase phrCannotWriteDBEntry(const uiWord&);
    static uiPhrase phrCannotStart(const uiWord&);
    static uiPhrase phrColonString(const uiWord&);
    static uiPhrase phrCheck(const uiWord&);
    static uiPhrase phrClose(const uiWord&);
    static uiPhrase phrCreateNew(const uiWord&);
    static uiPhrase phrCrossline(const uiWord&);
    static uiPhrase phrCrossPlot(const uiWord&);
    static uiPhrase phrCopy(const uiWord&);
    static uiPhrase phrCreate(const uiWord&);
    static uiPhrase phrData(const uiWord&);
    static uiPhrase phrDelete(const uiWord&);
    static uiPhrase phrDoesntExist(const uiWord&,int n=1);
    static uiPhrase phrEdit(const uiWord&);
    static uiPhrase phrEnter(const uiWord&);
    static uiPhrase phrExistsContinue(const uiWord&,bool overwrite);
    static uiPhrase phrExitOD();
    static uiPhrase phrExport(const uiWord& string);
    static uiPhrase phrExtract(const uiWord&);
    static uiPhrase phrGenerating(const uiWord&);
    static uiPhrase phrHandling(const uiWord&);
    static uiPhrase phrHandled(const uiWord&);
    static uiPhrase phrImport(const uiWord& string);
    static uiPhrase phrInline(const uiWord&);
    static uiPhrase phrInput(const uiWord&);
    static uiPhrase phrInsert(const uiWord&);
    static uiPhrase phrInvalid(const uiWord& string);
    static uiPhrase phrInternalError(const uiWord& string);
    static uiPhrase phrInternalError(const char* string);
    static uiPhrase phrJoinStrings(const uiPhrase& a,const uiPhrase& b);
    static uiPhrase phrJoinStrings(const uiPhrase& a,const uiPhrase& b,
				   const uiPhrase& c);
    static uiPhrase phrLoad(const uiWord&);
    static uiPhrase phrLoading(const uiWord&);
    static uiPhrase phrManage(const uiWord&);
    static uiPhrase phrMerge(const uiWord&);
    static uiPhrase phrModify(const uiWord&);
    static uiPhrase phrOpen(const uiWord&);
    static uiPhrase phrOutput(const uiWord&);
    static uiPhrase phrPlsContactSupport(bool firstconsultdoc);
    static uiPhrase phrReading(const uiWord&);
    static uiPhrase phrRead(const uiWord&);
    static uiPhrase phrRemove(const uiWord&);
    static uiPhrase phrRemoveSelected(const uiWord&);
    static uiPhrase phrRename(const uiWord&);
    static uiPhrase phrSave(const uiWord&);
    static uiPhrase phrSaveAs(const uiWord&);
    static uiPhrase phrPlsSelectAtLeastOne(const uiWord& string);
    static uiPhrase phrPlsSpecifyAtLeastOne(const uiWord& string);
    static uiPhrase phrSelect(const uiWord& string);
    static uiPhrase phrSelectObjectWrongType(const uiWord& string);
    static uiPhrase phrSelectPos(const uiWord& string);
    static uiPhrase phrSetAs(const uiWord&);
    static uiPhrase phrShowIn(const uiWord&);
    static uiPhrase phrSpecify(const uiWord&);
    static uiPhrase phrStart(const uiWord&);
    static uiPhrase phrStorageDir(const uiWord&);
    static uiPhrase phrSuccessfullyExported(const uiWord&);
    static uiPhrase phrTODONotImpl(const char* clssname);
    static uiPhrase phrNotImplInThisVersion(const char* fromver);
    static uiPhrase phrThreeDots(const uiWord& string,bool immediate=false);
    static uiPhrase phrWriting(const uiWord&);
    static uiPhrase phrWritten(const uiWord&);
    static uiPhrase phrXcoordinate(const uiWord&);
    static uiPhrase phrYcoordinate(const uiWord&);
    static uiPhrase phrZIn(const uiWord&);
    static uiPhrase phrZRange(const uiWord&);


//Words
    static uiWord s2D()			{ return tr("2D"); }
    static uiWord s2DLine()		{ return tr("2D line"); }
    static uiWord s2DPlane(int n=1)	{ return tr("2D Plane",0,n); }
    static uiWord s3D()			{ return tr("3D"); }
    static uiWord sAbort()		{ return tr("Abort"); }
    static uiWord sAbove()		{ return tr("Above"); }
    static uiWord sAction()		{ return tr("Action"); }
    static uiWord sAdd()		{ return tr("Add"); }
    static uiWord sAdvanced()		{ return tr("Advanced"); }
    static uiWord sAlignment()		{ return tr("Alignment"); }
    static uiWord sAll()		{ return tr("All"); }
    static uiWord sAlpha()		{ return tr("Alpha"); }
    static uiWord sAmplitude(int n=1)	{ return tr("Amplitude",0,n); }
    static uiWord sAnalysis()		{ return tr("Analysis"); }
    static uiWord sAnd()		{ return tr("and"); }
    static uiWord sApply()		{ return tr("Apply"); }
    static uiWord sASCII()		{ return tr("ASCII"); }
    static uiWord sAttribName()		{ return tr("Attribute Name"); }
    static uiWord sAxis()		{ return tr("Axis"); }
    static uiWord sAzimuth()		{ return tr("Azimuth"); }
    static uiWord sAttribute(int n=1)	{ return tr("Attribute",0,n); }
    static uiWord sAttributeSet(int n=1){ return tr("Attribute Set",0,n);}
    static uiWord sAverage()		{ return tr("Average"); }
    static uiWord sBasic()		{ return tr("Basic"); }
    static uiWord sBatch()		{ return tr("Batch"); }
    static uiWord sBatchProgram()	{ return tr("Batch Program"); }
    static uiWord sBelow()		{ return tr("Below"); }
    static uiWord sBlue()		{ return tr("Blue"); }
    static uiWord sBody(int n=1)	{ return tr("Body", 0, n); }
    static uiWord sBottom()		{ return tr("Bottom"); }
    static uiWord sBottomHor()		{ return tr("Bottom Horizon"); }
    static uiWord sCalculate()		{ return tr("Calculate"); }
    static uiWord sCalculateFrom()	{ return tr("Calculate From"); }
    static uiWord sCancel()		{ return tr("Cancel"); }
    static uiWord sCancelled()		{ return tr("Cancelled"); }
    static uiWord sCantCreateHor();
    static uiWord sCannotAllocate()	{ return tr("Cannot allocate memory"); }
    static uiWord sCannotExtract();
    static uiWord sCantFindAttrName();
    static uiWord sCantFindODB();
    static uiWord sCantFindSurf();
    static uiWord sCannot()		{ return tr("Cannot"); }
    static uiWord sCannotImport();
    static uiWord sCannotOpen();
    static uiWord sCantReadHor();
    static uiWord sCantReadInp();
    static uiWord sCantWriteSettings();
    static uiWord sCantOpenInpFile(int n=1);
    static uiWord sCantOpenOutpFile(int n=1);
    static uiWord sCannotAdd();
    static uiWord sCannotCompute();
    static uiWord sCannotCopy();
    static uiWord sCannotEdit();
    static uiWord sCannotFind();
    static uiWord sCannotLoad();
    static uiWord sCannotParse();
    static uiWord sCannotRemove();
    static uiWord sCannotSave();
    static uiWord sCannotStart();
    static uiWord sCannotWrite();
    static uiWord sCannotUnZip();
    static uiWord sCheck();
    static uiWord sCheckPermissions();
    static uiWord sCannotZip();
    static uiWord sClear()		{ return tr("Clear"); }
    static uiWord sClose()		{ return tr("Close"); }
    static uiWord sCode(int n=1)	{ return tr("Code",0,n); }
    static uiWord sCollectingData()	{ return tr("Collecting Data"); }
    static uiWord sColorTable(int n=1){ return tr("Color Table",0,n); }
    static uiWord sColor(int n=1)	{ return tr("Color",0,n); }
    static uiWord sColumn(int n=1)	{ return tr("Column",0,n); }
    static uiWord sComponent()		{ return tr("Component"); }
    static uiWord sConstant()		{ return tr("Constant"); }
    static uiWord sCoordSys()		{ return tr("Coordinate System"); }
    static uiWord sContinue()		{ return tr("Continue"); }
    static uiWord sContour(int n=1)	{ return tr("Contour",0,n); }
    static uiWord sCoordinate(int n=1)	{ return tr("Coordinate",0,n); }
    static uiWord sCopy();
    static uiWord sCoefficient()	{ return tr("Coefficient"); }
    static uiWord sCorrelation(int n=1)	{ return tr("Correlation",0,n); }
    static uiWord sCorrelCoeff()	{ return tr("Correlation Coefficient");}
    static uiWord sCreate()		{ return tr("Create"); }
    static uiWord sCreateGroup()	{ return tr("Create Group"); }
    static uiWord sCreateNew();
    static uiWord sCreateOutput();
    static uiWord sCreateProbDesFunc();
    static uiWord sCrossline(int n=1)	{ return tr("Cross-line", 0, n ); }
    static uiWord sCrosslineDip()	{ return sLineDip(false); }
    static uiWord sCrossPlot();
    static uiWord sCube(int n=1)	{ return tr("Cube",0,n); }
    static uiWord sDimension()		{ return tr("Dimension"); }
    static uiWord sDirectory()		{ return tr("Directory"); }
    static uiWord sData();
    static uiWord sDecimal()		{ return tr("Decimal"); }
    static uiWord sDefault()		{ return tr("Default"); }
    static uiWord sDefine()		{ return tr("Define"); }
    static uiWord sDelete();
    static uiWord sDepth()	        { return tr("Depth"); }
    static uiWord sDescription()	{ return tr("Description"); }
    static uiWord sDip()		{ return tr("Dip"); }
    static uiWord sDisabled()		{ return tr("Disabled"); }
    static uiWord sDisplay()		{ return tr("Display"); }
    static uiWord sDistance()		{ return tr("Distance"); }
    static uiWord sDone()		{ return tr("Done"); }
    static uiWord sDown()		{ return tr("Down"); }
    static uiWord sDraw()		{ return tr("Draw"); }
    static uiWord sEast(bool abbr)	{ return abbr ? tr("E"):tr("East"); }
    static uiWord sEdit();
    static uiWord sEmptyString()	{ return uiWord::emptyString(); }
    static uiWord sEnabled()		{ return tr("Enabled"); }
    static uiWord sEnter();
    static uiWord sEnterValidName();
    static uiWord sErrors(int n=1)	{ return tr("Error", 0, n); }
    static uiWord sEvent(int n=1)	{ return tr("Event", 0, n); }
    static uiWord sExamine()		{ return tr("Examine"); }
    static uiWord sExit()		{ return tr("Exit"); }
    static uiWord sExport();
    static uiWord sExtract();
    static uiWord sFactor(int n=1)	{ return tr("Factor",0,n); }
    static uiWord sFault(int n=1);
    static uiWord sFaultData()		{ return tr("Fault Data"); }
    static uiWord sFaultStickSet(int n=1);
    static uiWord sFeet()		{ return tr("Feet"); }
    static uiWord sFile()	        { return tr("File"); }
    static uiWord sFileDoesntExist()	{ return phrDoesntExist(sFile(),1); }
    static uiWord sFileName(int n=1)	{ return tr("File Name",0,n); }
    static uiWord sFinish()		{ return tr("Finish"); }
    static uiWord sFinished()		{ return tr("Finished"); }
    static uiWord sFilter(int n=1)	{ return tr("Filter",0,n); }
    static uiWord sFilters()		{ return sFilter(mPlural); }
    static uiWord sFlip()		{ return tr("Flip"); }
    static uiWord sFlipLeftRight()	{ return tr("Flip left/right"); }
    static uiWord sFrequency(int n=1);
    static uiWord sGeneral()		{ return tr("General"); }
    static uiWord sGenerating()	{ return tr("Generating"); }
    static uiWord sGeometry(int n=1)	{ return tr("Geometry",0,n); }
    static uiWord sGo()			{ return tr("Go"); }
    static uiWord sGreen()		{ return tr("Green"); }
    static uiWord sHelp();
    static uiWord sHeight()		{ return tr("Height"); }
    static uiWord sHide()		{ return tr("Hide"); }
    static uiWord sHistogram();
    static uiWord sHorizon(int n=1);
    static uiWord sHorizonData()	{ return tr("Horizon Data"); }
    static uiWord sHorizontal()		{ return tr("Horizontal"); }
    static uiWord sID()			{ return tr("ID"); }
    static uiWord sImport();
    static uiWord sImpSuccess()		{ return tr("Import successful"); }
    static uiWord sInfo()		{ return tr("info"); }
    static uiWord sInformation()	{ return tr("Information"); }
    static uiWord sInline(int n=1)	{ return tr("In-line",0,n); }
    static uiWord sInlineDip()		{ return tr("Inline Dip"); }
    static uiWord sInputParamsMissing();
    static uiWord sInput();
    static uiWord sInputFile();
    static uiWord sInputSelection();
    static uiWord sInputASCIIFile();
    static uiWord sInputData()		{ return tr("Input Data"); }
    static uiWord sInsert();
    static uiWord sInvalid();
    static uiWord sInvInpFile()		{ return tr("Invalid input file"); }
    static uiWord sInterpolating()	{ return tr("Interpolating"); }
    static uiWord sInterpolation()	{ return tr("Interpolation"); }
    static uiWord sLanguage()		{ return tr("Language"); }
    static uiWord sLatitude(bool abbr);
    static uiWord sLayer(int n=1)	{ return tr("Layer",0,n); }
    static uiWord sLeft()		{ return tr("Left"); }
    static uiWord sLine(int n=1)	{ return tr("Line",0,n); }
    static uiWord sLineDip(bool for2d)	{ return for2d ? tr("Line Dip")
							: tr("Crossline Dip"); }
    static uiWord sLineName(int n=1)	{ return tr("Line Name",0,n); }
    static uiWord sLineGeometry()	{ return tr("Line Geometry"); }
    static uiWord sLineStyle(int n=1)	{ return tr("Line Style",0,n); }
    static uiWord sLithology(int n=1)	{ return tr("Lithology",0,n); }
    static uiWord sLoad();
    static uiWord sLock()		{ return tr("Lock"); }
    static uiWord sLog(int n=1)		{ return tr("Log",0,n); }
    static uiWord sLogs();
    static uiWord sLogFile()		{ return tr("Log File"); }
    static uiWord sLooknFeel()		{ return tr("Look and Feel"); }
    static uiWord sLongitude(bool abbr);
    static uiWord sManage();
    static uiWord sManual()		{ return tr("Manual"); }
    static uiWord sManWav()		{ return tr("Manage Wavelets");}
    static uiWord sMarker(int n=1);
    static uiWord sMedian()		{ return tr("Median"); }
    static uiWord sMenu()		{ return tr("Menu"); }
    static uiWord sMeter()		{ return tr("Meter"); }
    static uiWord sMerge();
    static uiWord sModify();
    static uiWord sMouse()		{ return tr("Mouse"); }
    static uiWord sMove()		{ return tr("Move"); }
    static uiWord sMoveToBottom()	{ return tr("Move To Bottom"); }
    static uiWord sMoveToTop()		{ return tr("Move To Top"); }
    static uiWord sMoveDown()		{ return tr("Move Down"); }
    static uiWord sMoveUp()		{ return tr("Move Up"); }
    static uiWord sMsec(int n=1)	{ return tr("Millisecond",0,n); }
    static uiWord sMultiple()		{ return tr("Multiple"); }
    static uiWord sMute(int n=1)	{ return tr("Mute",0,n); }
    static uiWord sName(int n=1)	{ return tr("Name",0,n); }
    static uiWord sNew();
    static uiWord sNext()		{ return tr("Next"); }
    static uiWord sNo()			{ return tr("No"); }
    static uiWord sNode(int n=1)	{ return tr("Node",0,n); }
    static uiWord sNorth(bool abbr)	{ return abbr ? tr("N") : tr("North"); }
    static uiWord sNoLogSel()		{ return tr("No log selected"); }
    static uiWord sNone()		{ return tr("None"); }
    static uiWord sNormal()		{ return tr("Normal"); }
    static uiWord sNoValidData()	{ return tr("No valid data found"); }
    static uiWord sObject()		{ return tr("Object"); }
    static uiWord sOffset()		{ return tr("Offset"); }
    static uiWord sOk()			{ return tr("OK"); }
    static uiWord sOnlyAtSections()	{ return tr("Only at Sections"); }
    static uiWord sOpen();
    static uiWord sOpendTect()		{ return tr("OpendTect"); }
    static uiWord sOperator()		{ return tr("Operator"); }
    static uiWord sOptions();
    static uiWord sOr()			{ return tr("or"); }
    static uiWord sOther()		{ return tr("Other"); }
    static uiWord sOutpDataStore()	{ return tr("Output data store"); }
    static uiWord sOutputFile()		{ return tr("Output file"); }
    static uiWord sOutputStatistic()	{ return phrOutput( tr("statistic") ); }
    static uiWord sOutputFileExistsOverwrite();
    static uiWord sOutput();
    static uiWord sOutputSelection();
    static uiWord sOutputASCIIFile();
    static uiWord sOverwrite()		{ return tr("Overwrite"); }
    static uiWord sParFile()		{ return tr("Par File"); }
    static uiWord sPause()		{ return tr("Pause"); }
    static uiWord sParameter(int n=1)	{ return tr("Parameter",0,n); }
    static uiWord sPercentageDone()	{ return tr("Percentage done"); }
    static uiWord sPickSet(int n=1)	{ return tr("PickSet",0,n); }
    static uiWord sPointSet(int n=1)	{ return tr("PointSet",0,n); }
    static uiWord sPolygon(int n=1)	{ return tr("Polygon",0,n); }
    static uiWord sPosition(int n=1)	{ return tr("Position",0,n); }
    static uiWord sPreStack()		{ return tr("Prestack"); }
    static uiWord sPreStackEvents()	{ return tr("Prestack Events"); }
    static uiWord sPrevious()		{ return tr("Previous"); }
    static uiWord sProbe(int n=1)	{ return tr("Probe",0,n); }
    static uiWord sProcessing()		{ return tr("Processing"); }
    static uiWord sProcessingPars()	{ return tr("Processing parameters"); }
    static uiWord sProbDensFunc(bool abbr,int n=1);
    static uiWord sProgram()		{ return tr("Program"); }
    static uiWord sProblem(int n=1)	{ return tr("Problem",0,n); }
    static uiWord sProperty(int n=1)	{ return tr("Property",0,n); }
    static uiWord sProperties()		{ return sProperty(mPlural); }
    static uiWord sRange(int n=1)	{ return tr("Range",0,n); }
    static uiWord sRandomLine(int n=1)	{ return tr("Random Line",0,n); }
    static uiWord sReadingData()	{ return tr("Reading data"); }
    static uiWord sRectangle()		{ return tr("Rectangle"); }
    static uiWord sRed()		{ return tr("Red"); }
    static uiWord sRedo()		{ return tr("Redo"); }
    static uiWord sReload()		{ return tr("Reload"); }
    static uiWord sRemove();
    static uiWord sRemoveSelected();
    static uiWord sRename();
    static uiWord sReservoir()		{ return tr("Reservoir"); }
    static uiWord sReset()		{ return tr("Reset"); }
    static uiWord sResume()		{ return tr("Resume"); }
    static uiWord sRight()		{ return tr("Right"); }
    static uiWord sRightClick()		{ return tr("<right-click>"); }
    static uiWord sReversed()		{ return tr("Reversed"); }
    static uiWord sRockPhy()		{ return tr("Rock Physics"); }
    static uiWord sRow(int n=1)		{ return tr("Row",0,n); }
    static uiWord sRMS()		{ return tr("RMS"); }
    static uiWord sSave();
    static uiWord sSaveAs();
    static uiWord sSaveAsDefault()	{ return tr("Save as Default"); }
    static uiWord sSaveBodyFail()	{ return tr("Save body failed"); }
    static uiWord sSavingChanges()	{ return tr("Saving changes"); }
    static uiWord sSavingData()		{ return tr("Saving data"); }
    static uiWord sScanning()		{ return tr("Scanning"); }
    static uiWord sScene(int n=1)	{ return tr("Scene",0,n); }
    static uiWord sScenes()		{ return sScene(mPlural); }
    static uiWord sSearching()		{ return tr("Searching"); }
    static uiWord sSec(int n=1)		{ return tr("Second",0,1); }
    static uiWord sSEGY()		{ return tr("SEG-Y"); }
    static uiWord sSeismic(int n=1)	{ return tr("Seismic",0,n); }
    static uiWord sSeismicData()	{ return tr("Seismic Data"); }
    static uiWord sSelAttrib()		{ return tr("Select Attribute"); }
    static uiWord sSelection(int n=1);
    static uiWord sSelect();
    static uiWord sSelectedLog(int n=1)	{ return tr("Selected Log",0,n); }
    static uiWord sSelectPos();
    static uiWord sSelOutpFile();
    static uiWord sSession(int n=1)	{ return tr("Session",0,n); }
    static uiWord sSet(int n=1)		{ return tr("Set",0,n); }
    static uiWord sSetAs();
    static uiWord sSettings();
    static uiWord sSize()		{ return tr("Size"); }
    static uiWord sUserSettings();
    static uiWord sSetup()		{ return tr("Setup"); }
    static uiWord sShift();
    static uiWord sSemblance()		{ return tr("Semblance"); }
    static uiWord sSimilarity()		{ return tr("Similarity"); }
    static uiWord sSingle()		{ return tr("Single"); }
    static uiWord sSPNumber()		{ return tr("Shot-Point number"); }
    static uiWord sShow()		{ return tr("Show"); }
    static uiWord sShowIn();
    static uiWord sSlice()		{ return tr("Slice"); }
    static uiWord sSource(int n=1)	{ return tr("Source",0,n); }
    static uiWord sSouth(bool abbr)	{ return abbr ? tr("S"):tr("South"); }
    static uiWord sSpecify();
    static uiWord sSpecifyOut();
    static uiWord sStatistics()		{ return tr("Statistics"); }
    static uiWord sSteering()		{ return tr("Steering"); }
    static uiWord sStep(int n=1)	{ return tr("Step",0,n); }
    static uiWord sSteps()		{ return sStep(mPlural); }
    static uiWord sStepout()		{ return tr("Stepout"); }
    static uiWord sStop()		{ return tr("Stop"); }
    static uiWord sStorage()		{ return tr("Storage"); }
    static uiWord sStorageDir();
    static uiWord sStored();
    static uiWord sStratigraphy();
    static uiWord sSurface(int n=1)	{ return tr("Surface",0,n); }
    static uiWord sSurvey(int n=1)	{ return tr("Survey",0,n); }
    static uiWord sSurveys()		{ return sSurvey(mPlural); }
    static uiWord sTakeSnapshot()	{ return tr("Take Snapshot"); }
    static uiWord sTexture()		{ return tr("Texture"); }
    static uiWord sStart()		{ return tr("Start"); }
    static uiWord sTable(int n=1)	{ return tr("Table",0,n); }
    static uiWord sTheme()		{ return tr("Theme"); }
    static uiWord sTile()		{ return tr("Tile"); }
    static uiWord sTime()		{ return tr("Time"); }
    static uiWord sTitle()		{ return tr("Title"); }
    static uiWord sTmpStor()		{ return tr("Temporary storage "
							       "location :"); }
    static uiWord sToolbar()		{ return tr("Toolbar"); }
    static uiWord sTools()		{ return tr("Tools"); }
    static uiWord sTop()		{ return tr("Top"); }
    static uiWord sTopHor()		{ return tr("Top Horizon"); }
    static uiWord sTrace(int n=1)	{ return tr("Trace",0,n); }
    static uiWord sTraceNumber()	{ return tr("Trace number"); }
    static uiWord sTrack();
    static uiWord sTracking()		{ return tr("Tracking"); }
    static uiWord sTransform()		{ return tr("Transform"); }
    static uiWord sTransparency()	{ return tr("Transparency"); }
    static uiWord sType()		{ return tr("Type"); }
    static uiWord sUndef()		{ return tr("undef"); }
    static uiWord sUndefVal()		{ return tr("Undefined Value"); }
    static uiWord sUndo()		{ return tr("Undo"); }
    static uiWord sUnit(int n=1)	{ return tr("Unit",0,n); }
    static uiWord sUnlock()		{ return tr("Unlock"); }
    static uiWord sUnload()		{ return tr("Unload"); }
    static uiWord sUp()			{ return tr("Up"); }
    static uiWord sUse()		{ return tr("Use"); }
    static uiWord sUtilities()		{ return tr("Utilities"); }
    static uiWord sValue(int n=1)	{ return tr("Value",0,n); }
    static uiWord sVelocity(int n=1)	{ return tr("Velocity",0,n); }
    static uiWord sVertical()		{ return tr("Vertical"); }
    static uiWord sVolume(int n=1);
    static uiWord sView()		{ return tr("View"); }
    static uiWord sUpdatingDB()		{ return tr("Updating database"); }
    static uiWord sUpdatingDisplay()	{ return tr("Updating display"); }
    static uiWord sUseSingleColor()	{ return tr("Use Single Color"); }
    static uiWord sWarning()		{ return tr("Warning"); }
    static uiWord sWavelet(int n=1);
    static uiWord sWaveNumber(int n=1);
    static uiWord sWell(int n=1);
    static uiWord sWells()		{ return sWell(mPlural); }
    static uiWord sWellLog(int n=1);
    static uiWord sWest(bool abbr)	{ return abbr ? tr("W"):tr("West"); }
    static uiWord sWidth()		{ return tr("Width"); }
    static uiWord sWiggle()		{ return tr("Wiggle"); }

    static uiWord sWrite()		{ return tr("Write"); }

    static uiWord sWriting()		{ return tr("Writing"); }
    static uiWord sX()			{ return tr("X"); }
    static uiWord sY()			{ return tr("Y"); }
    static uiWord sY2()			{ return tr("Y2"); }
    static uiWord sY1()			{ return tr("Y1"); }
    static uiWord sZ()			{ return tr("Z"); }
    static uiWord sYes()		{ return tr("Yes"); }
    static uiWord sXcoordinate();
    static uiWord sYcoordinate();
    static uiWord sZip()		{ return tr("Zip"); }
    static uiWord sZUnit()		{ return tr("Z-unit"); }
    static uiWord sZSlice(int n=1)	{ return tr("Z-slice",0,n); }
    static uiWord sZValue(int n=1)	{ return tr("Z value",0,n); }
    static uiWord sZRange();

    static uiWord sDistUnitString(bool isfeet,bool abbr,bool withparentheses);
    /*!< returns "m", "ft", "meter", or "feet" */
    static uiWord sTimeUnitString(bool abbr=true);
    /*!< returns "s" or "seconds" */
    static uiWord sVolDataName(bool is2d,bool is3d,bool isprestack,
				     bool both_2d_3d_in_context=false,
				     bool both_pre_post_in_context=false);
    /*!<Returns names for data volumes such as "2D Data", "Cube",
	"Prestack Data", and similar */
};


#define m3Dots( txt ) uiStrings::phrThreeDots( txt, false )
#define mJoinUiStrs( txt1, txt2 )\
   uiStrings::phrJoinStrings( uiStrings::txt1, uiStrings::txt2 )

#define mTODONotImplPhrase() uiStrings::phrTODONotImpl( ::className(*this) )
