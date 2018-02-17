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

//! Use if you do not know exactly how many, but still it's more than 1
#define mPlural 2
//! Adds '...' to string, usable for menu items
#define m3Dots( txt ) \
    uiStrings::phrThreeDots( txt, false )
//! Incorrect, need replace. use uiStrng::appendXXX() and tool functions
#define mJoinUiStrs( txt1, txt2 )\
    uiStrings::phrJoinStrings( uiStrings::txt1, uiStrings::txt2 )
//! Shortcut handy macro for during development
#define mTODONotImplPhrase() \
    uiStrings::phrTODONotImpl( ::className(*this) )


/*!\brief Phrases and words that can (and must!) be re-used when possible.

  OpendTect has a tremendous amount of translatable strings; translating these
  into a non-English language can be a huge amount of work. To keep the work
  at least to a minimum we should try to re-use as many translations as
  possible.

*/

mExpClass(Basic) uiStrings
{ mODTextTranslationClass(uiStrings);
public:


// Phrases

    static uiPhrase phrAdd(const uiWord&);
    static uiPhrase phrASCII(const uiWord&);
    static uiPhrase phrCalculate(const uiWord&);
    static uiPhrase phrCalculateFrom(const uiWord&);
    static uiPhrase phrCannotAdd(const uiWord&);
    static uiPhrase phrCannotCalculate(const uiWord&);
    static uiPhrase phrCannotCopy(const uiWord&);
    static uiPhrase phrCannotCreate(const uiWord&);
    static uiPhrase phrCannotCreateDBEntryFor(const uiWord&);
    static uiPhrase phrCannotCreateDirectory(const uiWord&);
    static uiPhrase phrCannotCreateHor();
    static uiPhrase phrCannotEdit(const uiWord&);
    static uiPhrase phrCannotExtract(const uiWord&);
    static uiPhrase phrCannotFind(const uiWord&);
    static uiPhrase phrCannotFindDBEntry(const uiWord&);
    static uiPhrase phrCannotImport(const uiWord&);
    static uiPhrase phrCannotLoad(const uiWord&);
    static uiPhrase phrCannotOpen(const uiWord&);
    static uiPhrase phrCannotParse(const uiWord&);
    static uiPhrase phrCannotRead(const uiWord&);
    static uiPhrase phrCannotRemove(const uiWord&);
    static uiPhrase phrCannotSave(const uiWord&);
    static uiPhrase phrCannotStart(const uiWord&);
    static uiPhrase phrCannotUnZip(const uiWord&);
    static uiPhrase phrCannotWrite(const uiWord&);
    static uiPhrase phrCannotWriteDBEntry(const uiWord&);
    static uiPhrase phrCannotZip(const uiWord&);
    static uiPhrase phrCheck(const uiWord&);
    static uiPhrase phrClose(const uiWord&);
    static uiPhrase phrColonString(const uiWord&);
    static uiPhrase phrCopy(const uiWord&);
    static uiPhrase phrCreate(const uiWord&);
    static uiPhrase phrCreateNew(const uiWord&);
    static uiPhrase phrCrossPlot(const uiWord&);
    static uiPhrase phrCrossline(const uiWord&);
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
    static uiPhrase phrHandled(const uiWord&);
    static uiPhrase phrHandling(const uiWord&);
    static uiPhrase phrImport(const uiWord& string);
    static uiPhrase phrInline(const uiWord&);
    static uiPhrase phrInput(const uiWord&);
    static uiPhrase phrInsert(const uiWord&);
    static uiPhrase phrInternalError(const char* string);
    static uiPhrase phrInternalError(const uiWord& string);
    static uiPhrase phrInvalid(const uiWord& string);
    static uiPhrase phrJoinStrings(const uiPhrase&,const uiPhrase&);
    static uiPhrase phrJoinStrings(const uiPhrase&,const uiPhrase&,
				   const uiPhrase&);
    static uiPhrase phrLoad(const uiWord&);
    static uiPhrase phrLoading(const uiWord&);
    static uiPhrase phrManage(const uiWord&);
    static uiPhrase phrMerge(const uiWord&);
    static uiPhrase phrModify(const uiWord&);
    static uiPhrase phrNotImplInThisVersion(const char* fromver);
    static uiPhrase phrOpen(const uiWord&);
    static uiPhrase phrOutput(const uiWord&);
    static uiPhrase phrPlsContactSupport(bool firstconsultdoc);
    static uiPhrase phrPlsSelectAtLeastOne(const uiWord& string);
    static uiPhrase phrPlsSpecifyAtLeastOne(const uiWord& string);
    static uiPhrase phrRead(const uiWord&);
    static uiPhrase phrReading(const uiWord&);
    static uiPhrase phrRemove(const uiWord&);
    static uiPhrase phrRemoveSelected(const uiWord&);
    static uiPhrase phrRename(const uiWord&);
    static uiPhrase phrSave(const uiWord&);
    static uiPhrase phrSaveAs(const uiWord&);
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
    static uiPhrase phrThreeDots(const uiWord& string,bool immediate=false);
    static uiPhrase phrWriting(const uiWord&);
    static uiPhrase phrWritten(const uiWord&);
    static uiPhrase phrXcoordinate(const uiWord&);
    static uiPhrase phrYcoordinate(const uiWord&);
    static uiPhrase phrZIn(const uiWord&);
    static uiPhrase phrZRange(const uiWord&);


//Words

    static uiWord sDistUnitString(bool isfeet,bool abbr,bool withparentheses);
	/*!< returns "m", "ft", "meter", or "feet" */
    static uiWord sTimeUnitString(bool abbr=true);
	/*!< returns "s" or "seconds" */
    static uiWord sVolDataName(bool is2d,bool is3d,bool isprestack,
				     bool both_2d_3d_in_context=false,
				     bool both_pre_post_in_context=false);
	/*!< returns names for data volumes such as "2D Data", "Cube",
	    "Prestack Data", and similar */


    // Words with no inline definition are incorrect;
    // do not use or turn into phrase

    static uiWord s2D()			{ return tr("2D"); }
    static uiWord s2DHorizon(int n=1)	{ return tr("2D Horizon",0,n); }
    static uiWord s2DLine()		{ return tr("2D line"); }
    static uiWord s2DPlane(int n=1)	{ return tr("2D Plane",0,n); }
    static uiWord s3D()			{ return tr("3D"); }
    static uiWord s3DHorizon(int n=1)	{ return tr("3D Horizon",0,n); }
    static uiWord sAbort()		{ return tr("Abort"); }
    static uiWord sAbout()		{ return tr("About"); }
    static uiWord sAbove()		{ return tr("Above"); }
    static uiWord sAbsolute()		{ return tr("Absolute"); }
    static uiWord sAccept()		{ return tr("Accept"); }
    static uiWord sAction()		{ return tr("Action"); }
    static uiWord sAdd()		{ return tr("Add"); }
    static uiWord sAdvanced()		{ return tr("Advanced"); }
    static uiWord sAlgorithm()		{ return tr("Algorithm"); }
    static uiWord sAlignment()		{ return tr("Alignment"); }
    static uiWord sAll()		{ return tr("All"); }
    static uiWord sAlpha()		{ return tr("Alpha"); }
    static uiWord sAmplitude(int n=1)	{ return tr("Amplitude",0,n); }
    static uiWord sAnalyse()		{ return tr("Analyse"); }
    static uiWord sAnalysis()		{ return tr("Analysis"); }
    static uiWord sAnd()		{ return tr("and"); }
    static uiWord sAngle()		{ return tr("Angle"); }
    static uiWord sAnnotation(int n=1)	{ return tr("Annotation",0,n); }
    static uiWord sApply()		{ return tr("Apply"); }
    static uiWord sArea()		{ return tr("Area"); }
    static uiWord sASCII()		{ return tr("ASCII"); }
    static uiWord sAttribName()		{ return tr("Attribute Name"); }
    static uiWord sAttribute(int n=1)	{ return tr("Attribute",0,n); }
    static uiWord sAttributeSet(int n=1){ return tr("Attribute Set",0,n);}
    static uiWord sAuto()		{ return tr("Auto"); }
    static uiWord sAutomatic()		{ return tr("Automatic"); }
    static uiWord sAvailable()		{ return tr("Available"); }
    static uiWord sAverage()		{ return tr("Average"); }
    static uiWord sAxis()		{ return tr("Axis"); }
    static uiWord sAzimuth()		{ return tr("Azimuth"); }
    static uiWord sBasic()		{ return tr("Basic"); }
    static uiWord sBase(bool math)	{ return math ? tr("Base","Math")
						      : tr("Base","Layer"); }
    static uiWord sBinary()		{ return tr("Binary"); }
    static uiWord sBatch()		{ return tr("Batch"); }
    static uiWord sBatchProgram()	{ return tr("Batch Program"); }
    static uiWord sBelow()		{ return tr("Below"); }
    static uiWord sBlue()		{ return tr("Blue"); }
    static uiWord sBody(int n=1)	{ return tr("Body", 0, n); }
    static uiWord sBottom()		{ return tr("Bottom"); }
    static uiWord sBottomHor()		{ return tr("Bottom Horizon"); }
    static uiWord sBrowse()		{ return tr("Browse"); }
    static uiWord sCalculate()		{ return tr("Calculate"); }
    static uiWord sCalculating()	{ return tr("Calculating"); }
    static uiWord sCalculateFrom()	{ return tr("Calculate From"); }
    static uiWord sCancel()		{ return tr("Cancel"); }
    static uiWord sCancelled()		{ return tr("Cancelled"); }
    static uiWord sCannot();
    static uiWord sCannotAdd();
    static uiWord sCannotAllocate();
    static uiWord sCannotCopy();
    static uiWord sCannotEdit();
    static uiWord sCannotExtract();
    static uiWord sCannotFind();
    static uiWord sCannotImport();
    static uiWord sCannotLoad();
    static uiWord sCannotOpen();
    static uiWord sCannotParse();
    static uiWord sCannotRemove();
    static uiWord sCannotSave();
    static uiWord sCannotStart();
    static uiWord sCannotUnZip();
    static uiWord sCannotWrite();
    static uiWord sCannotZip();
    static uiWord sCantFindAttrName();
    static uiWord sCantFindODB();
    static uiWord sCantFindSurf();
    static uiWord sCantOpenInpFile(int n=1);
    static uiWord sCantOpenOutpFile(int n=1);
    static uiWord sCantReadHor();
    static uiWord sCantReadInp();
    static uiWord sCantWriteSettings();
    static uiWord sChange()		{ return tr("Change"); }
    static uiWord sCheckPermissions();
    static uiWord sClear()		{ return tr("Clear"); }
    static uiWord sClose()		{ return tr("Close"); }
    static uiWord sCode(int n=1)	{ return tr("Code",0,n); }
    static uiWord sCoefficient()	{ return tr("Coefficient"); }
    static uiWord sCollectingData()	{ return tr("Collecting Data"); }
    static uiWord sColor(int n=1)	{ return tr("Color",0,n); }
    static uiWord sColorTable(int n=1)	{ return tr("Color Table",0,n); }
    static uiWord sColumn(int n=1)	{ return tr("Column",0,n); }
    static uiWord sComponent()		{ return tr("Component"); }
    static uiWord sConstant(bool form)
    { return form ? tr("Constant","in formula"):tr("Constant","not changing"); }
    static uiWord sContinue()		{ return tr("Continue"); }
    static uiWord sContour(int n=1)	{ return tr("Contour",0,n); }
    static uiWord sConversion()		{ return tr("Conversion"); }
    static uiWord sCoordSys()		{ return tr("Coordinate System"); }
    static uiWord sCoordinate(int n=1)	{ return tr("Coordinate",0,n); }
    static uiWord sCopy()		{ return tr("Copy"); }
    static uiWord sCorrelCoeff()	{ return tr("Correlation Coefficient");}
    static uiWord sCorrelation(int n=1)	{ return tr("Correlation",0,n); }
    static uiWord sCreate()		{ return tr("Create"); }
    static uiWord sCreateGroup()	{ return tr("Create Group"); }
    static uiWord sCreateNew();
    static uiWord sCreateOutput();
    static uiWord sCreateProbDesFunc();
    static uiWord sCrl()		{ return tr("Crl","abbrev Cross-line");}
    static uiWord sCrossPlot()		{ return tr("Cross Plot"); }
    static uiWord sCrossline(int n=1)	{ return tr("Cross-line", 0, n ); }
    static uiWord sCrosslineDip()	{ return sLineDip(false); }
    static uiWord sCube(int n=1)	{ return tr("Cube",0,n); }
    static uiWord sData()		{ return tr("Data"); }
    static uiWord sDataSet()		{ return tr("Data Set"); }
    static uiWord sDecimal()		{ return tr("Decimal"); }
    static uiWord sDefault()		{ return tr("Default"); }
    static uiWord sDefine()		{ return tr("Define"); }
    static uiWord sDelete()		{ return tr("Delete"); }
    static uiWord sDepth()	        { return tr("Depth"); }
    static uiWord sDescription()	{ return tr("Description"); }
    static uiWord sDimension()		{ return tr("Dimension"); }
    static uiWord sDip()		{ return tr("Dip"); }
    static uiWord sDirectory()		{ return tr("Directory"); }
    static uiWord sDisabled()		{ return tr("Disabled"); }
    static uiWord sDisplay()		{ return tr("Display"); }
    static uiWord sDistance()		{ return tr("Distance"); }
    static uiWord sDone()		{ return tr("Done"); }
    static uiWord sDown()		{ return tr("Down"); }
    static uiWord sDraw()		{ return tr("Draw"); }
    static uiWord sEast(bool abbr)	{ return abbr ? tr("E") : tr("East"); }
    static uiWord sEdit()		{ return tr("Edit"); }
    static uiWord sEmptyString()	{ return uiWord::emptyString(); }
    static uiWord sEnabled()		{ return tr("Enabled"); }
    static uiWord sEnter();
    static uiWord sEnterValidName();
    static uiWord sErrors(int n=1)	{ return tr("Error",0,n); }
    static uiWord sEvent(int n=1)	{ return tr("Event",0,n); }
    static uiWord sExamine()		{ return tr("Examine"); }
    static uiWord sExit()		{ return tr("Exit"); }
    static uiWord sExport()		{ return tr("Export"); }
    static uiWord sExtract()		{ return tr("Extract"); }
    static uiWord sFactor(int n=1)	{ return tr("Factor",0,n); }
    static uiWord sFault(int n=1)	{ return tr("Fault",0,n); }
    static uiWord sFaultData()		{ return tr("Fault Data"); }
    static uiWord sFaultStickSet(int n=1) { return tr("FaultStickSet",0,n); }
    static uiWord sFeet()		{ return tr("Feet"); }
    static uiWord sFile()	        { return tr("File"); }
    static uiWord sFileDoesntExist()	{ return phrDoesntExist(sFile(),1); }
    static uiWord sFileName(int n=1)	{ return tr("File Name",0,n); }
    static uiWord sFilter(int n=1)	{ return tr("Filter",0,n); }
    static uiWord sFilters()		{ return sFilter(mPlural); }
    static uiWord sFinish()		{ return tr("Finish"); }
    static uiWord sFinished()		{ return tr("Finished"); }
    static uiWord sFlip()		{ return tr("Flip"); }
    static uiWord sFlipLeftRight()	{ return tr("Flip left/right"); }
    static uiWord sFrequency(int n=1)	{ return tr("Frequency",0,n); }
    static uiWord sFull()		{ return tr("Full"); }
    static uiWord sGeneral()		{ return tr("General"); }
    static uiWord sGenerating()		{ return tr("Generating"); }
    static uiWord sGeometry(int n=1)	{ return tr("Geometry",0,n); }
    static uiWord sGo()			{ return tr("Go"); }
    static uiWord sGreen()		{ return tr("Green"); }
    static uiWord sHalf()		{ return tr("Half"); }
    static uiWord sHeight()		{ return tr("Height"); }
    static uiWord sHelp()		{ return tr("Help"); }
    static uiWord sHide()		{ return tr("Hide"); }
    static uiWord sHigh()		{ return tr("High"); }
    static uiWord sHigher()		{ return tr("Higher"); }
    static uiWord sHighest()		{ return tr("Highest"); }
    static uiWord sHistogram()		{ return tr("Histogram"); }
    static uiWord sHorizon(int n=1)	{ return tr("Horizon",0,n); }
    static uiWord sHorizonData()	{ return tr("Horizon Data"); }
    static uiWord sHorizontal()		{ return tr("Horizontal"); }
    static uiWord sID()			{ return tr("ID"); }
    static uiWord sImpSuccess()		{ return tr("Import successful"); }
    static uiWord sImport()		{ return tr("Import"); }
    static uiWord sInfo()		{ return tr("info"); }
    static uiWord sInformation()	{ return tr("Information"); }
    static uiWord sInl()		{ return tr("Inl","abbrev In-line"); }
    static uiWord sInline(int n=1)	{ return tr("In-line",0,n); }
    static uiWord sInlineDip()		{ return tr("Inline Dip"); }
    static uiWord sInput()		{ return tr("Input"); }
    static uiWord sInputASCIIFile();
    static uiWord sInputData()		{ return tr("Input Data"); }
    static uiWord sInputFile();
    static uiWord sInputParamsMissing();
    static uiWord sInputSelection();
    static uiWord sInsert()		{ return tr("Insert"); }
    static uiWord sInterpolating()	{ return tr("Interpolating"); }
    static uiWord sInterpolation()	{ return tr("Interpolation"); }
    static uiWord sInvInpFile()		{ return tr("Invalid input file"); }
    static uiWord sInvalid()		{ return tr("Invalid"); }
    static uiWord sLanguage()		{ return tr("Language"); }
    static uiWord sLatitude(bool abbr)	{ return abbr?tr("Lat"):tr("Latitude");}
    static uiWord sLayer(int n=1)	{ return tr("Layer",0,n); }
    static uiWord sLeft()		{ return tr("Left"); }
    static uiWord sLine(int n=1)	{ return tr("Line",0,n); }
    static uiWord sLineDip(bool for2d)	{ return for2d ? tr("Line Dip")
						       : tr("Crossline Dip"); }
    static uiWord sLineGeometry()	{ return tr("Line Geometry"); }
    static uiWord sLineName(int n=1)	{ return tr("Line Name",0,n); }
    static uiWord sLineStyle(int n=1)	{ return tr("Line Style",0,n); }
    static uiWord sLithology(int n=1)	{ return tr("Lithology",0,n); }
    static uiWord sLoad()		{ return tr("Load"); }
    static uiWord sLock()		{ return tr("Lock"); }
    static uiWord sLog(int n=1)		{ return tr("Log",0,n); }
    static uiWord sLogFile()		{ return tr("Log File"); }
    static uiWord sLogs()		{ return sLog(mPlural); }
    static uiWord sLongitude(bool abbr)	{ return abbr ? tr("Long")
						      : tr("Longitude"); }
    static uiWord sLooknFeel()		{ return tr("Look and Feel"); }
    static uiWord sLow()		{ return tr("Low"); }
    static uiWord sManWav()		{ return tr("Manage Wavelets");}
    static uiWord sManage()		{ return tr("Manage"); }
    static uiWord sManual()		{ return tr("Manual"); }
    static uiWord sMarker(int n=1)	{ return tr("Marker",0,n); }
    static uiWord sMedian()		{ return tr("Median"); }
    static uiWord sMenu()		{ return tr("Menu"); }
    static uiWord sMerge()		{ return tr("Merge"); }
    static uiWord sMeter()		{ return tr("Meter"); }
    static uiWord sModify()		{ return tr("Modify"); }
    static uiWord sMouse()		{ return tr("Mouse"); }
    static uiWord sMove()		{ return tr("Move"); }
    static uiWord sMoveDown()		{ return tr("Move Down"); }
    static uiWord sMoveToBottom()	{ return tr("Move To Bottom"); }
    static uiWord sMoveToTop()		{ return tr("Move To Top"); }
    static uiWord sMoveUp()		{ return tr("Move Up"); }
    static uiWord sMsec(int n=1)	{ return tr("Millisecond",0,n); }
    static uiWord sMultiple()		{ return tr("Multiple"); }
    static uiWord sMute(int n=1)	{ return tr("Mute",0,n); }
    static uiWord sName(int n=1)	{ return tr("Name",0,n); }
    static uiWord sNew()		{ return tr("New"); }
    static uiWord sNext()		{ return tr("Next"); }
    static uiWord sNo()			{ return tr("No"); }
    static uiWord sNoLogSel()		{ return tr("No log selected"); }
    static uiWord sNoValidData()	{ return tr("No valid data found"); }
    static uiWord sNode(int n=1)	{ return tr("Node",0,n); }
    static uiWord sNone()		{ return tr("None"); }
    static uiWord sNormal()		{ return tr("Normal"); }
    static uiWord sNorth(bool abbr)	{ return abbr ? tr("N") : tr("North"); }
    static uiWord sObject()		{ return tr("Object"); }
    static uiWord sOffset()		{ return tr("Offset"); }
    static uiWord sOk()			{ return tr("OK"); }
    static uiWord sOnlyAtSections()	{ return tr("Only at Sections"); }
    static uiWord sOpen()		{ return tr("Open"); }
    static uiWord sOpendTect()		{ return tr("OpendTect"); }
    static uiWord sOperator()		{ return tr("Operator"); }
    static uiWord sOption(int n=1)	{ return tr("Option",0,n); }
    static uiWord sOptions()		{ return sOption(mPlural); }
    static uiWord sOr()			{ return tr("or"); }
    static uiWord sOther()		{ return tr("Other"); }
    static uiWord sOutpDataStore()	{ return tr("Output data store"); }
    static uiWord sOutput()		{ return tr("Output"); }
    static uiWord sOutputASCIIFile();
    static uiWord sOutputFile()		{ return tr("Output file"); }
    static uiWord sOutputFileExistsOverwrite();
    static uiWord sOutputSelection();
    static uiWord sOutputStatistic()	{ return phrOutput( tr("statistic") ); }
    static uiWord sOverwrite()		{ return tr("Overwrite"); }
    static uiWord sParFile()		{ return tr("Par File"); }
    static uiWord sParameter(int n=1)	{ return tr("Parameter",0,n); }
    static uiWord sPause()		{ return tr("Pause"); }
    static uiWord sPercentageDone()	{ return tr("Percentage done"); }
    static uiWord sPickSet(int n=1)	{ return tr("PickSet",0,n); }
    static uiWord sPointSet(int n=1)	{ return tr("PointSet",0,n); }
    static uiWord sPolygon(int n=1)	{ return tr("Polygon",0,n); }
    static uiWord sPosition(int n=1)	{ return tr("Position",0,n); }
    static uiWord sPreStack()		{ return tr("Prestack"); }
    static uiWord sPreStackEvents()	{ return tr("Prestack Events"); }
    static uiWord sPrevious()		{ return tr("Previous"); }
    static uiWord sProbDensFunc(bool abbr,int n=1)
    { return abbr ? tr("PDF",0,n) : tr("Probability Density Function", 0,n); }
    static uiWord sProbe(int n=1)	{ return tr("Probe",0,n); }
    static uiWord sProblem(int n=1)	{ return tr("Problem",0,n); }
    static uiWord sProcessing()		{ return tr("Processing"); }
    static uiWord sProcessingPars()	{ return tr("Processing parameters"); }
    static uiWord sProgram()		{ return tr("Program"); }
    static uiWord sProgress()		{ return tr("Progress"); }
    static uiWord sProperties()		{ return sProperty(mPlural); }
    static uiWord sProperty(int n=1)	{ return tr("Property",0,n); }
    static uiWord sRandomLine(int n=1)	{ return tr("Random Line",0,n); }
    static uiWord sRange(int n=1)	{ return tr("Range",0,n); }
    static uiWord sReadingData()	{ return tr("Reading data"); }
    static uiWord sRectangle()		{ return tr("Rectangle"); }
    static uiWord sRed()		{ return tr("Red"); }
    static uiWord sRedo()		{ return tr("Redo"); }
    static uiWord sRelative()		{ return tr("Relative"); }
    static uiWord sReload()		{ return tr("Reload"); }
    static uiWord sRemove()		{ return tr("Remove"); }
    static uiWord sRemoveSelected()	{ return tr("Remove Selected"); }
    static uiWord sRename()		{ return tr("Rename"); }
    static uiWord sReservoir()		{ return tr("Reservoir"); }
    static uiWord sReset()		{ return tr("Reset"); }
    static uiWord sResume()		{ return tr("Resume"); }
    static uiWord sReversed()		{ return tr("Reversed"); }
    static uiWord sRight()		{ return tr("Right"); }
    static uiWord sRightClick()		{ return tr("<right-click>"); }
    static uiWord sRMS()		{ return tr("RMS"); }
    static uiWord sRockPhy()		{ return tr("Rock Physics"); }
    static uiWord sRow(int n=1)		{ return tr("Row",0,n); }
    static uiWord sSave()		{ return tr("Save"); }
    static uiWord sSaveAs()		{ return tr("Save As"); }
    static uiWord sSaveAsDefault()	{ return tr("Save as Default"); }
    static uiWord sSaveBodyFail();
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
    static uiWord sSelOutpFile();
    static uiWord sSelect()		{ return tr("Select"); }
    static uiWord sSelectPos()		{ return tr("Select Position"); }
    static uiWord sSelectedLog(int n=1)	{ return tr("Selected Log",0,n); }
    static uiWord sSelection(int n=1)	{ return tr("Selection",0,n); }
    static uiWord sSemblance()		{ return tr("Semblance"); }
    static uiWord sSession(int n=1)	{ return tr("Session",0,n); }
    static uiWord sSet(int n=1)		{ return tr("Set",0,n); }
    static uiWord sSetAs()		{ return tr("Set As"); }
    static uiWord sSettings()		{ return tr("Settings"); }
    static uiWord sSetup()		{ return tr("Setup"); }
    static uiWord sShift()		{ return tr("Shift"); }
    static uiWord sShow()		{ return tr("Show"); }
    static uiWord sShowIn()		{ return tr("Show In"); }
    static uiWord sSimilarity()		{ return tr("Similarity"); }
    static uiWord sSingle()		{ return tr("Single"); }
    static uiWord sSize()		{ return tr("Size"); }
    static uiWord sSlice()		{ return tr("Slice"); }
    static uiWord sSource(int n=1)	{ return tr("Source",0,n); }
    static uiWord sSouth(bool abbr)	{ return abbr ? tr("S"):tr("South"); }
    static uiWord sSpecify()		{ return tr("Specify"); }
    static uiWord sSpecifyOut();
    static uiWord sSPNumber()		{ return tr("Shot-Point number"); }
    static uiWord sStandard()		{ return tr("Standard"); }
    static uiWord sStart()		{ return tr("Start"); }
    static uiWord sStatistics()		{ return tr("Statistics"); }
    static uiWord sSteering()		{ return tr("Steering"); }
    static uiWord sStep(int n=1)	{ return tr("Step",0,n); }
    static uiWord sStepout()		{ return tr("Stepout"); }
    static uiWord sSteps()		{ return sStep(mPlural); }
    static uiWord sStop()		{ return tr("Stop"); }
    static uiWord sStorage()		{ return tr("Storage"); }
    static uiWord sStorageDir()		{ return tr("Storage Directory"); }
    static uiWord sStored()		{ return tr("Stored"); }
    static uiWord sStratigraphy()	{ return tr("Stratigraphy"); }
    static uiWord sSurface(int n=1)	{ return tr("Surface",0,n); }
    static uiWord sSurvey(int n=1)	{ return tr("Survey",0,n); }
    static uiWord sSurveys()		{ return sSurvey(mPlural); }
    static uiWord sTable(int n=1)	{ return tr("Table",0,n); }
    static uiWord sTakeSnapshot()	{ return tr("Take Snapshot"); }
    static uiWord sTexture()		{ return tr("Texture"); }
    static uiWord sTheme()		{ return tr("Theme"); }
    static uiWord sTile()		{ return tr("Tile"); }
    static uiWord sTime()		{ return tr("Time"); }
    static uiWord sTitle()		{ return tr("Title"); }
    static uiWord sTmpStor()	    { return tr("Temporary storage location"); }
    static uiWord sToolbar()		{ return tr("Toolbar"); }
    static uiWord sTools()		{ return tr("Tools"); }
    static uiWord sTop()		{ return tr("Top"); }
    static uiWord sTopHor()		{ return tr("Top Horizon"); }
    static uiWord sTrace(int n=1)	{ return tr("Trace",0,n); }
    static uiWord sTraceNumber()	{ return tr("Trace number"); }
    static uiWord sTrack()		{ return tr("Track" ); }
    static uiWord sTracking()		{ return tr("Tracking"); }
    static uiWord sTransform()		{ return tr("Transform"); }
    static uiWord sTransparency()	{ return tr("Transparency"); }
    static uiWord sType()		{ return tr("Type"); }
    static uiWord sUndef()		{ return tr("undef"); }
    static uiWord sUndefVal()		{ return tr("Undefined Value"); }
    static uiWord sUndo()		{ return tr("Undo"); }
    static uiWord sUnit(int n=1)	{ return tr("Unit",0,n); }
    static uiWord sUnload()		{ return tr("Unload"); }
    static uiWord sUnlock()		{ return tr("Unlock"); }
    static uiWord sUp()			{ return tr("Up"); }
    static uiWord sUpdatingDB()		{ return tr("Updating database"); }
    static uiWord sUpdatingDisplay()	{ return tr("Updating display"); }
    static uiWord sUse()		{ return tr("Use"); }
    static uiWord sUseSingleColor()	{ return tr("Use Single Color"); }
    static uiWord sUserSettings()	{ return tr("User Settings"); }
    static uiWord sUtilities()		{ return tr("Utilities"); }
    static uiWord sVariable(bool form)
    { return form ? tr("Variable","in formula") : tr("Variable","changing"); }
    static uiWord sValue(int n=1)	{ return tr("Value",0,n); }
    static uiWord sVelocity(int n=1)	{ return tr("Velocity",0,n); }
    static uiWord sVertical()		{ return tr("Vertical"); }
    static uiWord sView()		{ return tr("View"); }
    static uiWord sVolume(int n=1)	{ return tr("Volume",0,n); }
    static uiWord sWarning()		{ return tr("Warning"); }
    static uiWord sWaveNumber()		{ return tr("Wavenumber"); }
    static uiWord sWavelet(int n=1)	{ return tr("Wavelet",0,n); }
    static uiWord sWell(int n=1)	{ return tr("Well",0,n); }
    static uiWord sWellLog(int n=1)	{ return tr("Well Log",0,n); }
    static uiWord sWells()		{ return sWell(mPlural); }
    static uiWord sWest(bool abbr)	{ return abbr ? tr("W") : tr("West"); }
    static uiWord sWidth()		{ return tr("Width"); }
    static uiWord sWiggle()		{ return tr("Wiggle"); }
    static uiWord sWrite()		{ return tr("Write"); }
    static uiWord sWriting()		{ return tr("Writing"); }
    static uiWord sX()			{ return tr("X-coordinate"); }
    static uiWord sXcoordinate()	{ return tr("X"); }
    static uiWord sY()			{ return tr("Y"); }
    static uiWord sY1()			{ return tr("Y1"); }
    static uiWord sY2()			{ return tr("Y2"); }
    static uiWord sYcoordinate()	{ return tr("Y-coordinate"); }
    static uiWord sYes()		{ return tr("Yes"); }
    static uiWord sZ()			{ return tr("Z"); }
    static uiWord sZip()		{ return tr("Zip"); }
    static uiWord sZRange()		{ return tr("Z Range"); }
    static uiWord sZSlice(int n=1)	{ return tr("Z-slice",0,n); }
    static uiWord sZUnit()		{ return tr("Z-unit"); }
    static uiWord sZValue(int n=1)	{ return tr("Z value",0,n); }

};
