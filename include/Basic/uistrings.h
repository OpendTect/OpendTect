#ifndef uistrings_h
#define uistrings_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          25/08/1999
 RCS:           $Id$
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
//Phrases
    static uiString phrAdd(const uiString&);
    //!<"Add <string>"
    static uiString phrASCII(const uiString& string);
    //!<"ASCII <string>"
    static uiString phrCalculate(const uiString&);
    //!<"Calculate <string>"
    static uiString phrCalculateFrom(const uiString& string);
    //!<"Calculate from <string>"
    static uiString phrCannotAdd(const uiString&);
    //!<"Cannot Add <string>"
    static uiString phrCannotCopy(const uiString&);
    //!<"Cannot Copy <string>"
    static uiString phrCannotCreate(const uiString& string);
    //!<"Cannot create <string>"
    static uiString phrCannotCreateDBEntryFor(const uiString& string);
    //!<"Cannot create database entry for <string>"
    static uiString phrCannotCreateDirectory(const uiString& string);
    //!<"Cannot create directory <string>"
    static uiString phrCannotEdit(const uiString&);
    //!<"Cannot edit <string>"
    static uiString phrCannotExtract(const uiString&);
    //!<"Cannot extract <string>"
    static uiString phrCannotFind(const uiString& string);
    //!<"Cannot find <string>"
    static uiString phrCannotFindDBEntry(const uiString&);
    //!<"Cannot find database entry for
    static uiString phrCannotImport(const uiString&);
    //!"Cannot Import <string>"
    static uiString phrCannotLoad(const uiString&);
    //!"Cannot Load <string>"
    static uiString phrCannotOpen(const uiString& string);
    //!<"Cannot open <string>"
    static uiString phrCannotRead(const uiString& string);
    //!<"Cannot read <string>"
    static uiString phrCannotSave(const uiString&);
    //!<"Cannot Save <string>"
    static uiString phrCannotRemove(const uiString& string);
    //!<"Cannot remove <string>"
    static uiString phrCannotUnZip(const uiString&);
    //!<"Cannot UnZip <string>"
    static uiString phrCannotZip(const uiString&);
    //!<"Cannot Zip <string>"
    static uiString phrCannotWrite(const uiString& string);
    //!<"Cannot write <string>"
    static uiString phrCannotWriteDBEntry(const uiString&);
    //!<"Cannot write database entry for <string>"
    static uiString phrCannotStart(const uiString&);
    //!<"Cannot Start <string>"
    static uiString phrColonString(const uiString&);
    //!<": <string>"
    static uiString phrCheck(const uiString&);
    //!<"Check <string>"
    static uiString phrCreateNew(const uiString&);
    //!<"Create New <string>"
    static uiString phrCrossline(const uiString&);
    //!<"Cross-line <string>
    static uiString phrCrossPlot(const uiString&);
    //!<"Cross Plot <string>"
    static uiString phrCopy(const uiString&);
    //!<"Copy <string>"
    static uiString phrCreate(const uiString& string);
    //!<"Create <string>"
    static uiString phrData(const uiString&);
    //!<"Data <string>"
    static uiString phrDelete(const uiString&);
    //!<"Delete <string>"
    static uiString phrDoesntExist(const uiString& string,int num=1);
    //!<"<string> does/do not exist"
    static uiString phrEdit(const uiString& string);
    //!<"Edit <string>"
    static uiString phrEnter(const uiString&);
    //!<"Enter <string>"
    static uiString phrExistsConinue(const uiString&,bool overwrite);
    //!<"<string> exists. Continue?" or "<string> exists. Overwrite?
    static uiString phrExport(const uiString& string);
    //!<"Export <string>"
    static uiString phrExtract(const uiString&);
    //!<"Extract <string>"
    static uiString phrGenerating(const uiString&);
    //!<"Generating <string>"
    static uiString phrImport(const uiString& string);
    //!<"Import <string>"
    static uiString phrInline(const uiString&);
    //!<"In-line <string>
    static uiString phrInput(const uiString&);
    //!<"Input <string>"
    static uiString phrInsert(const uiString&);
    //!<"Insert <string>"
    static uiString phrInvalid(const uiString& string);
    //!<"Invalid <string>"
    static uiString phrJoinStrings(const uiString& a,const uiString& b);
    //!<"<a> <b>
    static uiString phrJoinStrings(const uiString& a,const uiString& b,
				   const uiString& c);
    //!<"<a> <b> <c>
    static uiString phrLoad(const uiString&);
    //!<"Load <string>"
    static uiString phrManage(const uiString&);
    //!<"Manage <string>"
    static uiString phrMerge(const uiString&);
    //!<"Merge <string>"
    static uiString phrModify(const uiString&);
    //!<"Modify <string>"
    static uiString phrOpen(const uiString&);
    //!<"Open <string>"
    static uiString phrOutput(const uiString&);
    //!<"Output <string>"
    static uiString phrReading(const uiString&);
    //!<"Reading <string>"
    static uiString phrRemove(const uiString&);
    //!<"Remove <string>"
    static uiString phrRemoveSelected(const uiString&);
    //!<"Remove Selected <string>"
    static uiString phrRename(const uiString&);
    //!<"Rename <string>"
    static uiString phrSave(const uiString&);
    //!<"Save <string>"
    static uiString phrSelect(const uiString& string);
    //!<"Select <string>"
    static uiString phrSelectObjectWrongType(const uiString& string);
    //!<"Selected object is not a <string>"
    static uiString phrSelectPos(const uiString& string);
    //!<"Select Position <string>"
    static uiString phrSetAs(const uiString&);
    //<!"Set As <string>"
    static uiString phrShowIn(const uiString&);
    //<!"Show in <string>"
    static uiString phrSpecify(const uiString&);
    //<!"Specify <string>"
    static uiString phrStorageDir(const uiString& string);
    //!<"Storage Directory <string>"
    static uiString phrSuccessfullyExported(const uiString&);
    //!<"Successfully exported <string>"
    static uiString phrTODONotImpl(const char* clssname);
    //!<"[clssname] TO DO: Not Implemented"> ...
    static uiString phrThreeDots(const uiString& string,bool immediate=false);
    //!<string> ...
    static uiString phrWriting(const uiString&);
    //!<"Writing <string>"
    static uiString phrXcoordinate(const uiString&);
    //!<"X-coordinate <string>"
    static uiString phrYcoordinate(const uiString&);
    //!<"Y-coordinate <string>"
    static uiString phrZIn(const uiString&);
    //!<"Z in <string>"
    static uiString phrZRange(const uiString&);
    //!<"Z Range <string>"


//Words
    static uiString s2D();
    static uiString s2DLine()		{ return tr("2D line"); }
    static uiString s3D();
    static uiString sAbort()		{ return tr("Abort"); }
    static uiString sAbove()		{ return tr("Above"); }
    static uiString sAction()		{ return tr("Action"); }
    static uiString sAdd();
    static uiString sAddColBlend()	{ return tr("Add Color Blended"); }
    static uiString sAdvanced()		{ return tr("Advanced"); }
    static uiString sAlignment()	{ return tr("OD::Alignment"); }
    static uiString sAll()		{ return tr("All"); }
    static uiString sAlpha()		{ return tr("Alpha"); }
    static uiString sAmplitude(int num=1){ return tr("Amplitude",0,num); }
    static uiString sAnalysis()		{ return tr("Analysis"); }
    static uiString sApply()		{ return tr("Apply"); }
    static uiString sASCII();
    static uiString sAttribName()	{ return tr("Attribute Name"); }
    static uiString sAxis()		{ return tr("Axis"); }
    static uiString sAzimuth()		{ return tr("Azimuth"); }
    static uiString sAttribute(int num=1) { return tr("Attribute",0,num); }
    static uiString sAttributes()	{ return sAttribute(mPlural); }
    static uiString sAverage()		{ return tr("Average"); }
    static uiString sBatch()		{ return tr("Batch"); }
    static uiString sBatchProgram();
    static uiString sBatchProgramFailedStart();
    static uiString sBelow()		{ return tr("Below"); }
    static uiString sBlue()		{ return tr("Blue"); }
    static uiString sBody(int num=1)	{ return tr("Body", 0, num); }
    static uiString sBottom()		{ return tr("Bottom"); }
    static uiString sBottomHor()	{ return tr("Bottom Horizon"); }
    static uiString sCalculate();
    static uiString sCalculateFrom();
    static uiString sCancel()		{ return tr("Cancel"); }
    static uiString sCantCreateHor();
    static uiString sCannotExtract();
    static uiString sCantFindAttrName();
    static uiString sCantFindODB();
    static uiString sCantFindSurf();
    static uiString sCannot()		{ return tr("Cannot"); }
    static uiString sCannotImport();
    static uiString sCannotOpen();
    static uiString sCantReadHor();
    static uiString sCantReadInp();
    static uiString sCantWriteSettings();
    static uiString sCantOpenInpFile(int num=1);
    static uiString sCantOpenOutpFile(int num=1);
    static uiString sCannotAdd();
    static uiString sCannotCopy();
    static uiString sCannotEdit();
    static uiString sCannotLoad();
    static uiString sCannotRemove();
    static uiString sCannotSave();
    static uiString sCannotStart();
    static uiString sCannotWrite();
    static uiString sCannotUnZip();
    static uiString sCheck();
    static uiString sCheckPermissions();
    static uiString sCannotZip();
    static uiString sCreateNew();
    static uiString sCreateOutput();
    static uiString sClear()		{ return tr("Clear"); }
    static uiString sClose()		{ return tr("Close"); }
    static uiString sCode(int num=1)	{ return tr("Code",0,num); }
    static uiString sColorTable(int num=1);
    static uiString sColor(int num=1)	{ return tr("Color",0, num); }
    static uiString sComponent()	{ return tr("Component"); }
    static uiString sConstant()		{ return tr("Constant"); }
    static uiString sContinue()		{ return tr("Continue"); }
    static uiString sContour(int num=1)	{ return tr("Contour",0,num); }
    static uiString sCoordinate(int num=1) { return tr("Coordinate",0,num); }
    static uiString sCopy();
    static uiString sCreateGroup()	{ return tr("Create Group"); }
    static uiString sCoefficient()	{ return tr("Coefficient"); }
    static uiString sCorrelation(int num=1)  { return tr("Correlation",0,num); }
    static uiString sCorrelCoeff()	{ return tr("Correlation Coefficient");}
    static uiString sCreate();
    static uiString sCreateProbDesFunc();
    static uiString sCrossline(int num=1) { return tr("Cross-line", 0, num ); }
    static uiString sCrossPlot();
    static uiString sCube(int num=1)	{ return tr("Cube",0,num); }
    static uiString sDimension()	{ return tr("Dimension"); }
    static uiString sDirectory()	{ return tr("Directory"); }
    static uiString sData();
    static uiString sDecimal()		{ return tr("Decimal"); }
    static uiString sDefault()		{ return tr("Default"); }
    static uiString sDefine()		{ return tr("Define"); }
    static uiString sDelete();
    static uiString sDepth()	        { return tr("Depth"); }
    static uiString sDip()		{ return tr("Dip"); }
    static uiString sDisabled()		{ return tr("Disabled"); }
    static uiString sDisplay()		{ return tr("Display"); }
    static uiString sDistance()		{ return tr("Distance"); }
    static uiString sDone()		{ return tr("Done"); }

    static uiString sDown()		{ return tr("Down"); }
    static uiString sDraw()		{ return tr("Draw"); }
    static uiString sEast(bool abb)	{ return abb ? tr("E"):tr("East"); }
    static uiString sEdit();
    static uiString sEmptyString()	{ return uiString::emptyString(); }
    static uiString sEnabled()		{ return tr("Enabled"); }
    static uiString sEnter();
    static uiString sEnterValidName();
    static uiString sErrors(int num=1)	{ return tr("Error", 0, num); }
    static uiString sExamine()		{ return tr("Examine"); }
    static uiString sExitOD()		{ return tr("Exit OpendTect"); }
    static uiString sExit()		{ return tr("Exit"); }
    static uiString sExport();
    static uiString sExtract();
    static uiString sFaultStickSet(int num=1);
    static uiString sFactor(int num=1)	{ return tr("Factor",0,num); }
    static uiString sFault(int num=1);
    static uiString sFeet()		{ return tr("Feet"); }
    static uiString sFile()	        { return tr("File"); }
    static uiString sFileDoesntExist()	{ return phrDoesntExist(sFile(),1); }
    static uiString sFileName()	        { return tr("File name"); }
    static uiString sFinish()		{ return tr("Finish"); }
    static uiString sFilter(int num=1)  { return tr("Filter",0,num); }
    static uiString sFilters()		{ return sFilter(mPlural); }
    static uiString sFlip()		{ return tr("Flip"); }
    static uiString sFlipLeftRight()	{ return tr("Flip left/right"); }
    static uiString sFrequency(int num=1);
    static uiString sGenerating()	{ return tr("Generating"); }
    static uiString sGeometry()		{ return tr("Geometry"); }
    static uiString sGo()	        { return tr("Go"); }
    static uiString sGreen()		{ return tr("Green"); }
    static uiString sHelp();
    static uiString sHeight()		{ return tr("Height"); }
    static uiString sHide()		{ return tr("Hide"); }
    static uiString sHistogram();
    static uiString sHorizon(int num=1);
    static uiString sHorizontal()	{ return tr("Horizontal"); }
    static uiString sImport();
    static uiString sImpSuccess()	{ return tr("Import successful"); }
    static uiString sInfo()		{ return tr("info"); }
    static uiString sInformation()	{ return tr("Information"); }
    static uiString sInline(int num=1)	{ return tr("In-line",0,num); }
    static uiString sInputParamsMissing();
    static uiString sInput();
    static uiString sInputFile();
    static uiString sInputSelection();
    static uiString sInputASCIIFile();
    static uiString sInputData()	{ return tr("Input Data"); }
    static uiString sInsert();
    static uiString sInvalid();
    static uiString sInvInpFile()	{ return tr("Invalid input file"); }
    static uiString sLat()		{ return tr("Latitude"); }
    static uiString sLayer()		{ return tr("Layer"); }
    static uiString sLine(int num=1)	{ return tr("Line",0,num); }
    static uiString sLineName(int num=1) { return tr("Line Name",0,num); }
    static uiString sLeft()		{ return tr("Left"); }
    static uiString sLithology(int num=1){ return tr("Lithology",0,num); }
    static uiString sLoad();
    static uiString sLock()		{ return tr("Lock"); }
    static uiString sLog(int num=1)	{ return tr("Log",0,num); }
    static uiString sLogs();
    static uiString sLogFile()		{ return tr("Log File"); }
    static uiString sLongitude()	{ return tr("Longitude"); }
    static uiString sManage();
    static uiString sManual()		{ return tr("Manual"); }
    static uiString sManWav(){ return uiStrings::phrManage( sWavelet(mPlural));}
    static uiString sMarker(int num=1);
    static uiString sMedian()		{ return tr("Median"); }
    static uiString sMenu()		{ return tr("Menu"); }
    static uiString sMeter()		{ return tr("Meter"); }
    static uiString sMerge();
    static uiString sModify();
    static uiString sMouse()		{ return tr("Mouse"); }
    static uiString sMove()		{ return tr("Move"); }
    static uiString sMoveDown()		{ return tr("Move Down"); }
    static uiString sMoveUp()		{ return tr("Move Up"); }
    static uiString sMsec()		{ return tr("msec"); }
    static uiString sMute(int num=1)	{ return tr("Mute",0,num); }
    static uiString sName(int num=1)	{ return tr("Name",0,num); }
    static uiString sNew();
    static uiString sNext()		{ return tr("Next"); }
    static uiString sNo()		{ return tr("No"); }
    static uiString sNorth(bool abb)	{ return abb ? tr("N") : tr("North"); }
    static uiString sNoLogSel()		{ return tr("No log selected"); }
    static uiString sNone()		{ return tr("None"); }
    static uiString sNormal()		{ return tr("Normal"); }
    static uiString sNoValidData()	{ return tr("No valid data found"); }
    static uiString sOffset()		{ return tr("Offset"); }
    static uiString sOk()		{ return tr("OK"); }
    static uiString sOnlyAtSections()	{ return tr("Only at Sections"); }
    static uiString sOpen();
    static uiString sOperator()		{ return tr("Operator"); }
    static uiString sOptions();
    static uiString sOutpDataStore()	{ return tr("Output data store"); }
    static uiString sOutputFile()	{ return tr("Output file"); }
    static uiString sOutputStatistic()	{ return phrOutput( tr("statistic") ); }
    static uiString sOutputFileExistsOverwrite();
    static uiString sOutput();
    static uiString sOutputSelection();
    static uiString sOutputASCIIFile();
    static uiString sOverwrite()        { return tr("Overwrite"); }
    static uiString sParFile()		{ return tr("Par File"); }
    static uiString sPause()            { return tr("Pause"); }
    static uiString sParameter(int num=1) { return tr("Parameter",0,num); }
    static uiString sPercentageDone()	{ return tr("Percentage done"); }
    static uiString sPickSet(int num=1)	{ return tr("PickSet",0,num); }
    static uiString sPolygon(int num=1)	{ return tr("Polygon",0,num); }
    static uiString sPosition(int num=1){ return tr("Position",0,num); }
    static uiString sPreStack()		{ return tr("PreStack"); }
    static uiString sPreStackEvents()	{ return tr("Prestack Events"); }
    static uiString sPrevious()		{ return tr("Previous"); }
    static uiString sProcessing()	{ return tr("Processing"); }
    static uiString sProcessingPars()	{ return tr("Processing parameters"); }
    static uiString sProbDensFunc(bool abbrevation=false, int num=1);
    static uiString sProgram()		{ return tr("Program"); }
    static uiString sProperties();
    static uiString sRange(int num=1)	{ return tr("Range",0,1); }
    static uiString sRandomLine(int num=1) { return tr("Random Line",0,num); }
    static uiString sRectangle()	{ return tr("Rectangle"); }
    static uiString sRed()		{ return tr("Red"); }
    static uiString sRedo()		{ return tr("Redo"); }
    static uiString sReload()		{ return tr("Reload"); }
    static uiString sRemove();
    static uiString sRemoveSelected();
    static uiString sRename();
    static uiString sReservoir()	{ return tr("Reservoir"); }
    static uiString sReset()		{ return tr("Reset"); }
    static uiString sResume()		{ return tr("Resume"); }
    static uiString sRight()		{ return tr("Right"); }
    static uiString sRightClick()	{ return tr("<right-click>"); }
    static uiString sReversed()		{ return tr("Reversed"); }
    static uiString sRockPhy()		{ return tr("Rock Physics"); }
    static uiString sRMS()		{ return tr("RMS"); }
    static uiString sSave();
    static uiString sSaveAs();
    static uiString sSaveAsDefault()    { return tr("Save as Default"); }
    static uiString sSaveBodyFail()	{ return tr("Save body failed"); }
    static uiString sScanning()		{ return tr("Scanning"); }
    static uiString sScene(int num=1)	{ return tr("Scene",0,1); }
    static uiString sScenes()		{ return sScene(mPlural); }
    static uiString sSec()		{ return tr("sec"); }
    static uiString sSize()		{ return tr("Size"); }
    static uiString sSEGY()		{ return tr("SEG-Y"); }
    static uiString sSeismic(int num=1);
    static uiString sSeismics()		{ return sSeismic(mPlural); }
    static uiString sSeismics(bool is2d,bool isps,int num);
    static uiString sSelAttrib()	{ return tr("Select Attribute"); }
    static uiString sSelection(int num=1);
    static uiString sSelect();
    static uiString sSelectedLog(int num =1) {return tr("Selected Log",0,num);}
    static uiString sSelectPos();
    static uiString sSelOutpFile();
    static uiString sSession(int num=1)	{ return tr("Session",0,num); }
    static uiString sSet(int num=1)	{ return tr("Set",0,num); }
    static uiString sSetAs();
    static uiString sSetting(int num=1);
    static uiString sSettings()		{ return sSetting(mPlural); }
    static uiString sSetup()		{ return tr("Setup"); }
    static uiString sShift();
    static uiString sSPNumber()		{ return tr("Shot-Point number"); }
    static uiString sShow()             { return tr("Show"); }
    static uiString sShowIn();
    static uiString sSlice()		{ return tr("Slice"); }
    static uiString sSource(int num=1)	{ return tr("Source",0,num); }
    static uiString sSouth(bool abb)	{ return abb ? tr("S"):tr("South"); }
    static uiString sSpecify();
    static uiString sSpecifyOut();
    static uiString sStatistics()	{ return tr("Statistics"); }
    static uiString sSteering()		{ return tr("Steering"); }
    static uiString sStep(int num=1)	{ return tr("Step",0,num); }
    static uiString sSteps()		{ return sStep(mPlural); }
    static uiString sStepout()		{ return tr("Stepout"); }
    static uiString sStop()		{ return tr("Stop"); }
    static uiString sStorageDir();
    static uiString sStored();
    static uiString sStratigraphy();
    static uiString sSurface()		{ return tr("Surface"); }
    static uiString sSurvey(int num=1)  { return tr("Survey",0,num); }
    static uiString sSurveys()		{ return sSurvey(mPlural); }
    static uiString sTakeSnapshot()	{ return tr("Take Snapshot"); }
    static uiString sStart()		{ return tr("Start"); }
    static uiString sTable(int num=1)	{ return tr("Table",0,num); }
    static uiString sTile()		{ return tr("Tile"); }
    static uiString sTime()		{ return tr("Time"); }
    static uiString sTmpStor()		{ return tr("Temporary storage "
							       "location :"); }
    static uiString sToolbar()		{ return tr("Toolbar"); }
    static uiString sTools()		{ return tr("Tools"); }
    static uiString sTop()		{ return tr("Top"); }
    static uiString sTopHor()		{ return tr("Top Horizon"); }
    static uiString sTrace(int num=1)	{ return tr("Trace",0,num); }
    static uiString sTraceNumber()	{ return tr("Trace number"); }
    static uiString sTrack();
    static uiString sTracking()		{ return tr("Tracking"); }
    static uiString sTransparency()     { return tr("Transparency"); }
    static uiString sType()             { return tr("Type"); }
    static uiString sUndefVal()		{ return tr("Undefined Value"); }
    static uiString sUndo()		{ return tr("Undo"); }
    static uiString sUnit(int num=1)	{ return tr("Unit",0,num); }
    static uiString sUnlock()		{ return tr("Unlock"); }
    static uiString sUnload()		{ return tr("Unload"); }
    static uiString sUp()		{ return tr("Up"); }
    static uiString sUse()		{ return tr("Use"); }
    static uiString sUtilities()	{ return tr("Utilities"); }
    static uiString sValue(int num=1)	{ return tr("Value",0,num); }
    static uiString sVelocity()		{ return tr("Velocity"); }
    static uiString sVertical()		{ return tr("Vertical"); }
    static uiString sVolume();
    static uiString sVolumep(int num=1);
    static uiString sView()		{ return tr("View"); }
    static uiString sUseSingleColor()	{ return tr("Use Single Color"); }
    static uiString sWarning()		{ return tr("Warning"); }
    static uiString sWavelet(int num=1);
    static uiString sWaveNumber(int num=1);
    static uiString sWell(int num=1);
    static uiString sWells()		{ return sWell(mPlural); }
    static uiString sWellLog(int num=1);
    static uiString sWest(bool abb)	{ return abb ? tr("W"):tr("West"); }
    static uiString sWidth()		{ return tr("Width"); }
    static uiString sWiggle()		{ return tr("Wiggle"); }
    static uiString sWrite()		{ return tr("Write"); }
    static uiString sWriting()		{ return tr("Writing"); }
    static uiString sX()		{ return tr("X"); }
    static uiString sY()		{ return tr("Y"); }
    static uiString sY2()		{ return tr("Y2"); }
    static uiString sY1()		{ return tr("Y1"); }
    static uiString sZ()		{ return tr("Z"); }
    static uiString sYes()		{ return tr("Yes"); }
    static uiString sXcoordinate();
    static uiString sYcoordinate();
    static uiString sZip()		{ return tr("Zip"); }
    static uiString sZUnit()		{ return tr("Z-unit"); }
    static uiString sZSlice(int num=1)	{ return tr("Z-slice",0,num); }
    static uiString sZValue(int num=1)	{ return tr("Z value",0,num); }
    static uiString sZRange();

    static uiString sDistUnitString(bool isfeet,bool abbrevated,
				    bool withparentheses);
    /*!< returns "m", "ft", "meter", or "feet" */
    static uiString sTimeUnitString(bool abbrevated=true);
    /*!< returns "s" or "seconds" */
    static uiString sVolDataName(bool is2d,bool is3d,bool isprestack,
				     bool both_2d_3d_in_context=false,
				     bool both_pre_post_in_context=false);
    /*!<Returns names for data volumes such as "2D Data", "Cube",
	"Pre-stack Data", and similar */
};


#define m3Dots( txt ) uiStrings::phrThreeDots( txt, false )
#define mJoinUiStrs( txt1, txt2 )\
   uiStrings::phrJoinStrings( uiStrings::txt1, uiStrings::txt2 )

#define mTODONotImplPhrase() uiStrings::phrTODONotImpl( ::className(*this) )


#endif
