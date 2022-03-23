#pragma once

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

#define mDIAGNOSTIC(s) uiStrings::phrDiagnostic(s)
#define mINTERNAL(s) uiStrings::phrInternalErr(s)

/*!Common strings that are localized. Using these keeps the translation
   at a minimum.
*/
mExpClass(Basic) uiStrings
{ mODTextTranslationClass(uiStrings)
public:
//Phrases
    static uiString phrAdd(const uiString& string);
    //!<Add + string"
    static uiString phrAllocating(od_int64);
    static uiString phrASCII(const uiString& string);
    //!<ASCII + string
    static uiString phrCalculate(const uiString&);
    //!<Calculate + string
    static uiString phrCalculateFrom(const uiString& string);
    //!<Calculate from + string
    static uiString phrCannotAdd(const uiString&);
    //!<Cannot Add + string
    static uiString phrCannotCopy(const uiString&);
    //!<Cannot Copy + string
    static uiString phrCannotCreate(const uiString& string);
    //!<Cannot create + string
    static uiString phrCannotCreateDBEntryFor(const uiString& string);
    //!<Cannot create database entry for + string
    static uiString phrCannotCreateDirectory(const uiString& string);
    //!<Cannot create directory + string
    static uiString phrCannotEdit(const uiString&);
    //!<Cannot edit + string
    static uiString phrCannotExtract(const uiString&);
    //!<Cannot extract + string
    static uiString phrCannotFind(const uiString& string);
    //!<Cannot find + string
    static uiString phrCannotFindDBEntry(const uiString&);
    static uiString phrCannotFindDBEntry(const MultiID&);
    //!<Cannot find database entry for
    static uiString phrCannotImport(const uiString&);
    //!"Cannot Import + string
    static uiString phrCannotLoad(const uiString&);
    //!"Cannot Load + string
    static uiString phrCannotOpen(const uiString& string);
    //!<Cannot open + string
    static uiString phrCannotOpen(const char*,bool forread);
    static uiString phrCannotOpenForRead(const char*);
    static uiString phrCannotOpenForWrite(const char*);
    static uiString phrCannotRead(const uiString& string);
    static uiString phrCannotRead(const char*);
    //!<Cannot read + string
    static uiString phrCannotSave(const uiString&);
    //!<Cannot Save + string
    static uiString phrCannotRemove(const uiString& string);
    static uiString phrCannotRemove(const char*);
    //!<Cannot remove + string
    static uiString phrCannotUnZip(const uiString&);
    //!<Cannot UnZip + string
    static uiString phrCannotZip(const uiString&);
    //!<Cannot Zip + string
    static uiString phrCannotWrite(const uiString& string);
    //!<Cannot write + string
    static uiString phrCannotWriteDBEntry(const uiString&);
    //!<Cannot write database entry for + string
    static uiString phrCannotStart(const uiString&);
    //!<Cannot Start + string
    static uiString phrColonString(const uiString& string);
    //!<colon + string
    static uiString phrCheck(const uiString&);
    //!<Check + string
    static uiString phrCreateNew(const uiString&);
    //!<Create New + string
    static uiString phrCrossline(const uiString&);
    //!<Cross-line + string
    static uiString phrCrossPlot(const uiString&);
    //!<Cross Plot + string
    static uiString phrCopy(const uiString&);
    //!<Copy + string
    static uiString phrCreate(const uiString& string);
    //!<Create + string
    static uiString phrData(const uiString&);
    //!<Data + string
    static uiString phrDelete(const uiString&);
    //!<Delete + string
    static uiString phrDiagnostic(const char*);
    static uiString phrDoesntExist(const uiString& string,int num=1);
    //!<string + "does/do not exist"
    static uiString phrEdit(const uiString& string);
    //!<Edit + string
    static uiString phrEnter(const uiString&);
    //!<Enter + string
    static uiString phrErrDuringIO(bool read,const char* objnm=nullptr);
    static uiString phrErrDuringIO(bool read,const uiString&);
    static uiString phrErrDuringRead( const char* objnm=nullptr )
		    { return phrErrDuringIO( true, objnm ); }
    static uiString phrErrDuringRead( const uiString& subj )
		    { return phrErrDuringIO( true, subj ); }
    static uiString phrErrDuringWrite( const char* objnm=nullptr )
		    { return phrErrDuringIO( false, objnm ); }
    static uiString phrErrDuringWrite( const uiString& subj )
		    { return phrErrDuringIO( false, subj ); }
    static uiString phrExistsConinue(const uiString&,bool overwrite);
    //!<"string exists. Continue?" or "string exists. Overwrite?"
    static uiString phrExport(const uiString& string);
    //!<Export + string
    static uiString phrExtract(const uiString&);
    //!<Extract + string
    static uiString phrFileDoesNotExist(const char*);
    static uiString phrGenerating(const uiString&);
    //!<Generating + string
    static uiString phrImport(const uiString& string);
    //!<Import + string
    static uiString phrInline(const uiString&);
    //!<In-line + string
    static uiString phrInput(const uiString&);
    //!<Input + string
    static uiString phrInsert(const uiString&);
    //!<Insert + string
    static uiString phrInternalErr(const char*);
    static uiString phrInvalid(const uiString& string);
    //!<Invalid + string
    static uiString phrJoinStrings(const uiString& a,const uiString& b);
    //!<Concatenates strings a and b
    static uiString phrJoinStrings(const uiString& a,const uiString& b,
				   const uiString& c);
    //!<Concatenates strings a, b and c
    static uiString phrLoad(const uiString&);
    //!<Load + string
    static uiString phrManage(const uiString&);
    //!<Manage + string
    static uiString phrMerge(const uiString&);
    //!<Merge + string
    static uiString phrModify(const uiString&);
    //!<Modify + string
    static uiString phrOpen(const uiString&);
    //!<Open + string
    static uiString phrOutput(const uiString&);
    //!<Output + string
    static uiString phrPlsSelectAtLeastOne(const uiString&);
    static uiString phrReading(const uiString&);
    //!<Reading + string
    static uiString phrRemove(const uiString&);
    //!<Remove + string
    static uiString phrRemoveSelected(const uiString&);
    //!<Remove Selected + string
    static uiString phrRename(const uiString&);
    //!<Rename + string
    static uiString phrSave(const uiString&);
    //!<Save + string
    static uiString phrSelect(const uiString& string);
    //!<Select + string
    static uiString phrSelectObjectWrongType(const uiString& string);
    //!<Selected object is not a + string
    static uiString phrSelectPos(const uiString& string);
    //!<Select Position + string
    static uiString phrSetAs(const uiString& string);
    //<!<Set As + string
    static uiString phrShowIn(const uiString& string);
    //<!<Show in + string
    static uiString phrSpecify(const uiString& string);
    //<!<Specify + string
    static uiString phrStorageDir(const uiString& string);
    //!<Storage Directory + string
    static uiString phrSuccessfullyExported(const uiString&);
    //!<Successfully exported + string
    static uiString phrTODONotImpl(const char* clssname);
    //!<[clssname] TO DO: Not Implemented
    static uiString phrNotImplInThisVersion(const char* fromver);
    //!<Not impl in this version of OpendTect. Please use version xx or up
    static uiString phrThreeDots(const uiString& string,bool immediate=false);
    //!<string + ...
    static uiString phrUnexpected(const uiString&,const char* what=nullptr);
    static uiString phrWriting(const uiString&);
    //!<Writing + string
    static uiString phrXcoordinate(const uiString&);
    //!<X-coordinate + string
    static uiString phrYcoordinate(const uiString&);
    //!<Y-coordinate + string
    static uiString phrZIn(const uiString&);
    //!<Z in + string
    static uiString phrZRange(const uiString&);
    //!<Z Range + string

    //Phrases that don't need specifics, can be used when context is obvious
    static uiString phrCannotAllocateMemory(od_int64 reqsz=-1);
    static uiString phrCannotFindAttrName();
    static uiString phrCannotFindObjInDB();
    static uiString phrCannotOpenInpFile(int n=1);
    static uiString phrCannotOpenOutpFile(int n=1);
    static uiString phrCannotReadHor();
    static uiString phrCannotReadInp();
    static uiString phrCannotWriteSettings();
    static uiString phrCheckPermissions();
    static uiString phrCheckUnits();
    static uiString phrDBIDNotValid();
    static uiString phrEnterValidName();
    static uiString phrSaveBodyFail();
    static uiString phrSelOutpFile();
    static uiString phrSpecifyOutput();

//Words
    static uiString s2D();
    static uiString s2DHorizon(int num=1) { return tr("2D Horizon",0,num); }
    static uiString s3DHorizon(int num=1) { return tr("3D Horizon",0,num); }
    static uiString s2DLine(int num=1)	{ return tr("2D line",0,num); }
    static uiString s2DPlane(int n=1)	{ return tr("2D Plane",0,n); }
    static uiString s3D();
    static uiString sAbort()		{ return tr("Abort"); }
    static uiString sAbove()		{ return tr("Above"); }
    static uiString sAction()		{ return tr("Action"); }
    static uiString sAdd();
    static uiString sAddColBlend()	{ return tr("Add Color Blended"); }
    static uiString sAdvanced()		{ return tr("Advanced"); }
    static uiString sAlignment()	{ return tr("Alignment"); }
    static uiString sAll()		{ return tr("All"); }
    static uiString sAlpha()		{ return tr("Alpha"); }
    static uiString sAmplitude(int num=1){ return tr("Amplitude",0,num); }
    static uiString sAnalysis()		{ return tr("Analysis"); }
    static uiString sAnd()		{ return tr("and"); }
    static uiString sApply()		{ return tr("Apply"); }
    static uiString sArea()		{ return tr("Area"); }
    static uiString sASCII();
    static uiString sAttribName()	{ return tr("Attribute Name"); }
    static uiString sAxis()		{ return tr("Axis"); }
    static uiString sAzimuth()		{ return tr("Azimuth"); }
    static uiString sAttribute(int num=1) { return tr("Attribute",0,num); }
    static uiString sAttributes()	{ return sAttribute(mPlural); }
    static uiString sAttributeSet(int num=1)
					{ return tr("Attribute Set",0,num); }
    static uiString sAvailable()	{ return tr("Available"); }
    static uiString sAverage()		{ return tr("Average"); }
    static uiString sBack()		{ return tr("Back"); }
    static uiString sBatch()		{ return tr("Batch"); }
    static uiString sBatchProgram();
    static uiString sBatchProgramFailedStart();
    static uiString sBelow()		{ return tr("Below"); }
    static uiString sBinary()		{ return tr("Binary"); }
    static uiString sBlue()		{ return tr("Blue"); }
    static uiString sBody(int num=1)	{ return tr("Body", 0, num); }
    static uiString sBottom()		{ return tr("Bottom"); }
    static uiString sBottomHor()	{ return tr("Bottom Horizon"); }
    static uiString sBrowse()		{ return tr("Browse"); }
    static uiString sByte(int n=1)	{ return tr("Byte",0,n); }
    static uiString sCalculate();
    static uiString sCalculateFrom();
    static uiString sCancel()		{ return tr("Cancel"); }
    static uiString sCancelled()	{ return tr("Cancelled"); }
    static uiString sCantCreateHor();
    static uiString sCannotAllocate()	{ return tr("Cannot allocate memory"); }
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
    static uiString sCenter()		{ return tr("Center","Alignment"); }
    static uiString sCheck();
    static uiString sCheckPermissions();
    static uiString sCannotZip();
    static uiString sCreateNew();
    static uiString sCreateOutput();
    static uiString sClass()		{ return tr("Class"); }
    static uiString sClear()		{ return tr("Clear"); }
    static uiString sClose()		{ return tr("Close"); }
    static uiString sCode(int num=1)	{ return tr("Code",0,num); }
    static uiString sCollapse()		{ return tr("Collapse"); }
    static uiString sCollectingData()	{ return tr("Collecting Data"); }
    static uiString sColorTable(int num=1);
    static uiString sColor(int num=1)	{ return tr("Color",0,num); }
    static uiString sColumn(int num=1)	{ return tr("Column",0,num); }
    static uiString sCommand(int num=1);
    static uiString sComponent()	{ return tr("Component"); }
    static uiString sConnection()	{ return tr("Connection"); }
    static uiString sConstant()		{ return tr("Constant"); }
    static uiString sContent()		{ return tr("Content"); }
    static uiString sContinue()		{ return tr("Continue"); }
    static uiString sContour(int num=1)	{ return tr("Contour",0,num); }
    static uiString sConvert()		{ return tr("Convert"); }
    static uiString sCoordinate(int num=1) { return tr("Coordinate",0,num); }
    static uiString sCoordSys()		{ return tr("Coordinate System"); }
    static uiString sCopy();
    static uiString sCrAt()		{ return tr("Created at"); }
    static uiString sCrBy()		{ return tr("Created by"); }
    static uiString sCreate()		{ return tr("Create"); }
    static uiString sCrFrom()		{ return tr("Created from"); }
    static uiString sCreateGroup()	{ return tr("Create Group"); }
    static uiString sCoefficient()	{ return tr("Coefficient"); }
    static uiString sCorrelation(int num=1)  { return tr("Correlation",0,num); }
    static uiString sCorrelCoeff()	{ return tr("Correlation Coefficient");}
    static uiString sCreateProbDesFunc();
    static uiString sCrossline(int num=1) { return tr("Cross-line", 0, num ); }
    static uiString sCrosslineDip()	{ return sLineDip(false); }
    static uiString sCrosslineRange()	{ return tr("Cross-line range"); }
    static uiString sCrossPlot();
    static uiString sCube(int num=1)	{ return tr("Cube",0,num); }
    static uiString sCurve()		{ return tr("Curve"); }
    static uiString sData();
    static uiString sDataStore()	{ return tr("Data Store"); }
    static uiString sDataType()		{ return tr("Data Type"); }
    static uiString sDecimal()		{ return tr("Decimal"); }
    static uiString sDefault()		{ return tr("Default"); }
    static uiString sDefine()		{ return tr("Define"); }
    static uiString sDelete();
    static uiString sDepth()		{ return tr("Depth"); }
    static uiString sDepthRange()	{ return tr("Depth range"); }
    static uiString sDescription()	{ return tr("Description"); }
    static uiString sDip()		{ return tr("Dip"); }
    static uiString sDimension()	{ return tr("Dimension"); }
    static uiString sDirectory()	{ return tr("Directory"); }
    static uiString sDisable()		{ return tr("Disable"); }
    static uiString sDisabled()		{ return tr("Disabled"); }
    static uiString sDiscard()		{ return tr("Discard"); }
    static uiString sDisplay()		{ return tr("Display"); }
    static uiString sDistance()		{ return tr("Distance"); }
    static uiString sDone()		{ return tr("Done"); }
    static uiString sDown()		{ return tr("Down"); }
    static uiString sDraw()		{ return tr("Draw"); }
    static uiString sEast(bool abb)	{ return abb ? tr("E"):tr("East"); }
    static uiString sEdit();
    static uiString sEnable()		{ return tr("Enable"); }
    static uiString sEmptyString()	{ return uiString::emptyString(); }
    static uiString sEnabled()		{ return tr("Enabled"); }
    static uiString sEnter();
    static uiString sEnterValidName();
    static uiString sErrors(int num=1)	{ return tr("Error",0,num); }
    static uiString sExamine()		{ return tr("Examine"); }
    static uiString sExitOD()		{ return tr("Exit OpendTect"); }
    static uiString sExit()		{ return tr("Exit"); }
    static uiString sExpand()		{ return tr("Expand"); }
    static uiString sExport();
    static uiString sExtract();
    static uiString sFactor(int num=1)	{ return tr("Factor",0,num); }
    static uiString sFault(int num=1);
    static uiString sFaultData()	{ return tr("Fault Data"); }
    static uiString sFaultStickSet(int num=1);
    static uiString sFaultSet(int num=1){ return tr("FaultSet",0,num); }
    static uiString sFeet()		{ return tr("Feet"); }
    static uiString sFile()		{ return tr("File"); }
    static uiString sFileDoesntExist()	{ return phrDoesntExist(sFile(),1); }
    static uiString sFileName()		{ return tr("File name"); }
    static uiString sFinish()		{ return tr("Finish"); }
    static uiString sFinished()		{ return tr("Finished"); }
    static uiString sFilter(int num=1)  { return tr("Filter",0,num); }
    static uiString sFilters()		{ return sFilter(mPlural); }
    static uiString sFiltering()	    { return tr("Filtering"); }
    static uiString sFlip()		{ return tr("Flip"); }
    static uiString sFlipLeftRight()	{ return tr("Flip left/right"); }
    static uiString sFull()		{ return tr("Full"); }
    static uiString sFolder(int n=1)	{ return tr("Folder", 0, n); }
    static uiString sFormat()		{ return tr("Format"); }
    static uiString sFrequency(int num=1);
    static uiString sGeneral()		{ return tr("General"); }
    static uiString sGenerate()		{ return tr("Generate"); }
    static uiString sGenerating()	{ return tr("Generating"); }
    static uiString sGeometry(int num=1) { return tr("Geometry",0,num); }
    static uiString sGo()		{ return tr("Go"); }
    static uiString sGreen()		{ return tr("Green"); }
    static uiString sGridding()		{ return tr("Gridding"); }
    static uiString sHelp();
    static uiString sHeight()		{ return tr("Height"); }
    static uiString sHide()		{ return tr("Hide"); }
    static uiString sHistogram();
    static uiString sHost()		{ return tr("Host"); }
    static uiString sHostName()		{ return tr("Host Name"); }
    static uiString sHorizon(int num=1);
    static uiString sHorizonData()	{ return tr("Horizon Data"); }
    static uiString sHorizontal()	{ return tr("Horizontal"); }
    static uiString sID()		{ return tr("ID"); }
    static uiString sImport();
    static uiString sImpSuccess()	{ return tr("Import successful"); }
    static uiString sInfo()		{ return tr("info"); }
    static uiString sInformation()	{ return tr("Information"); }
    static uiString sInline(int num=1)	{ return tr("In-line",0,num); }
    static uiString sInlineDip()	{ return tr("Inline Dip"); }
    static uiString sInlineRange()	{ return tr("In-line range"); }
    static uiString sInputParamsMissing();
    static uiString sInput();
    static uiString sInputFile();
    static uiString sInputSelection();
    static uiString sInputASCIIFile();
    static uiString sInputData()	{ return tr("Input Data"); }
    static uiString sInsert();
    static uiString sInvalid();
    static uiString sInvInpFile()	{ return tr("Invalid input file"); }
    static uiString sLastModified()	{ return tr("Last Modified"); }
    static uiString sLat()		{ return tr("Latitude"); }
    static uiString sLayer()		{ return tr("Layer"); }
    static uiString sLevel(int n=1)	{ return tr("Level",0,n); }
    static uiString sLine(int num=1)	{ return tr("Line",0,num); }
    static uiString sLineDip(bool for2d) { return for2d ? tr("Line Dip")
							: tr("Crossline Dip"); }
    static uiString sLineName(int num=1) { return tr("Line Name",0,num); }
    static uiString sLineStyle(int num=1) { return tr("Line Style",0,num); }
    static uiString sLeft()		{ return tr("Left"); }
    static uiString sLicense(int n=1)	{ return tr("License",0,n); }
    static uiString sNoLicense()	{ return tr("No License"); }
    static uiString sLithology(int num=1){ return tr("Lithology",0,num); }
    static uiString sLoad();
    static uiString sLocation(int num=1)	{ return tr("Location",0,num); }
    static uiString sLock()		{ return tr("Lock"); }
    static uiString sLog(int num=1)	{ return tr("Log",0,num); }
    static uiString sLogName(int num=1)	{ return tr("Log Name",0,num); }
    static uiString sLogs();
    static uiString sLogFile()		{ return tr("Log File"); }
    static uiString sLongitude()	{ return tr("Longitude"); }
    static uiString sManage();
    static uiString sManual()		{ return tr("Manual"); }
    static uiString sManWav(){ return uiStrings::phrManage( sWavelet(mPlural));}
    static uiString sMarker(int num=1);
    static uiString sMD()		{ return tr("MD","Measured Depth"); }
    static uiString sMedian()		{ return tr("Median"); }
    static uiString sMemSizeString(od_int64);
    static uiString sMenu()		{ return tr("Menu"); }
    static uiString sMeter()		{ return tr("Meter"); }
    static uiString sMerge();
    static uiString sMode()		{ return tr("Mode"); }
    static uiString sModel(int num=1)	{ return tr("Model",0,num); }
    static uiString sModify();
    static uiString sMouse()		{ return tr("Mouse"); }
    static uiString sMove()		{ return tr("Move"); }
    static uiString sMoveToBottom()	{ return tr("Move To Bottom"); }
    static uiString sMoveToTop()	{ return tr("Move To Top"); }
    static uiString sMoveDown()		{ return tr("Move Down"); }
    static uiString sMoveUp()		{ return tr("Move Up"); }
    static uiString sMsec()		{ return tr("Millisecond"); }
    static uiString sMute(int num=1)	{ return tr("Mute",0,num); }
    static uiString sName(int num=1)	{ return tr("Name",0,num); }
    static uiString sNew();
    static uiString sNext()		{ return tr("Next"); }
    static uiString sNo()		{ return tr("No"); }
    static uiString sNoInfoAvailable(){ return tr("No information available"); }
    static uiString sNorth(bool abb)	{ return abb ? tr("N") : tr("North"); }
    static uiString sNoLogSel()		{ return tr("No log selected"); }
    static uiString sNone()		{ return tr("None"); }
    static uiString sNormal()		{ return tr("Normal"); }
    static uiString sNotPresent()	{ return tr("Not Present"); }
    static uiString sNoValidData()	{ return tr("No valid data found"); }
    static uiString sObject()		{ return tr("Object"); }
    static uiString sObjectID()		{ return tr("Object ID"); }
    static uiString sODTColTab();
    static uiString sOff()		{ return tr("Off","not in action"); }
    static uiString sOffset()		{ return tr("Offset"); }
    static uiString sOk()		{ return tr("OK"); }
    static uiString sOn()		{ return tr("On","in action"); }
    static uiString sOnlyAtSections()	{ return tr("Only at Sections"); }
    static uiString sOpen();
    static uiString sOperator()		{ return tr("Operator"); }
    static uiString sOptions();
    static uiString sOr()		{ return tr("or"); }
    static uiString sOther();
    static uiString sOtherUser();
    static uiString sOutpDataStore()	{ return tr("Output data store"); }
    static uiString sOutputFile()	{ return tr("Output file"); }
    static uiString sOutputStatistic()	{ return phrOutput( tr("statistic") ); }
    static uiString sOutputFileExistsOverwrite();
    static uiString sOutput();
    static uiString sOutputSelection();
    static uiString sOutputASCIIFile();
    static uiString sOverwrite()	{ return tr("Overwrite"); }
    static uiString sPackage(int n=1)	{ return tr("Package",0,n); }
    static uiString sParFile()		{ return tr("Par File"); }
    static uiString sParsMissing()	{ return tr("Parameters missing"); }
    static uiString sPass()		{ return tr("Pass"); }
    static uiString sPause()		{ return tr("Pause"); }
    static uiString sParameter(int num=1) { return tr("Parameter",0,num); }
    static uiString sPercentageDone()	{ return tr("Percentage done"); }
    static uiString sPetrelAlut();
    static uiString sPickSet(int num=1)	{ return tr("PickSet",0,num); }
    static uiString sPlatform()		{ return tr("Platform"); }
    static uiString sPoint(int n=1)	{ return tr("Point",0,n); }
    static uiString sPointsDone()	{ return tr("Points done"); }
    static uiString sPointSet(int num=1){ return tr("PointSet",0,num); }
    static uiString sPolarity()		{ return tr("Polarity"); }
    static uiString sPolygon(int num=1)	{ return tr("Polygon",0,num); }
    static uiString sPolyLine(int num=1) { return tr("PolyLine",0,num); }
    static uiString sPosition(int num=1){ return tr("Position",0,num); }
    static uiString sPositionsDone()	{ return tr("Positions Done"); }
    static uiString sPreStack()		{ return tr("Prestack"); }
    static uiString sPreStackEvents()	{ return tr("Prestack Events"); }
    static uiString sPrevious()		{ return tr("Previous"); }
    static uiString sProceed()		{ return tr("Proceed"); }
    static uiString sProperty()		{ return tr("Property"); }
    static uiString sProcessing()	{ return tr("Processing"); }
    static uiString sProcessingPars()	{ return tr("Processing parameters"); }
    static uiString sProbDensFunc(bool abbrevation=false, int num=1);
    static uiString sProgram()		{ return tr("Program"); }
    static uiString sProperties();
    static uiString sRange(int num=1)	{ return tr("Range",0,num); }
    static uiString sRandomLine(int num=1) { return tr("Random Line",0,num); }
    static uiString sRectangle()	{ return tr("Rectangle"); }
    static uiString sRed()		{ return tr("Red"); }
    static uiString sRedo()		{ return tr("Redo"); }
    static uiString sRegion(int n=1)	{ return tr("Region",0,n); }
    static uiString sRegionalMarker()	{ return tr("Regional Marker"); }
    static uiString sReload()		{ return tr("Reload"); }
    static uiString sRemove();
    static uiString sRemoveSelected();
    static uiString sRename();
    static uiString sRequired()		{ return tr("Required"); }
    static uiString sReservoir()	{ return tr("Reservoir"); }
    static uiString sReset()		{ return tr("Reset"); }
    static uiString sResolution()	{ return tr("Resolution"); }
    static uiString sResolutionValue(int resval);
    static uiString sRestart()		{ return tr("Restart"); }
    static uiString sResume()		{ return tr("Resume"); }
    static uiString sRight()		{ return tr("Right"); }
    static uiString sRightClick()	{ return tr("<right-click>"); }
    static uiString sReversed()		{ return tr("Reversed"); }
    static uiString sRockPhy()		{ return tr("Rock Physics"); }
    static uiString sRow(int num=1)	{ return tr("Row",0,num); }
    static uiString sRMS()		{ return tr("RMS"); }
    static uiString sSampleInterval()	{ return tr("Sample interval"); }
    static uiString sSave();
    static uiString sSaveAs();
    static uiString sSaveAsDefault()    { return tr("Save as Default"); }
    static uiString sSaveBodyFail()	{ return tr("Save body failed"); }
    static uiString sSavingChanges()	{ return tr("Saving changes"); }
    static uiString sScale()		{ return tr("Scale"); }
    static uiString sScaling()		{ return tr("Scaling"); }
    static uiString sScanning()		{ return tr("Scanning"); }
    static uiString sScene(int num=1)	{ return tr("Scene",0,1); }
    static uiString sScenes()		{ return sScene(mPlural); }
    static uiString sSec()		{ return tr("Second"); }
    static uiString sSEGY()		{ return tr("SEG-Y"); }
    static uiString sSeismic(int num=1);
    static uiString sSeismicData()	{ return tr("Seismic Data"); }
    static uiString sSeismics()		{ return sSeismic(mPlural); }
    static uiString sSeismics(bool is2d,bool isps,int num);
    static uiString sSelAttrib()	{ return tr("Select Attribute"); }
    static uiString sSelection(int num=1);
    static uiString sSelect();
    static uiString sSelectedLog(int num =1) {return tr("Selected Log",0,num);}
    static uiString sSelectIcon();
    static uiString sSelectPos();
    static uiString sSelOutpFile();
    static uiString sSession(int num=1)	{ return tr("Session",0,num); }
    static uiString sSet(int num=1)	{ return tr("Set",0,num); }
    static uiString sSetAs();
    static uiString sSetting(int num=1);
    static uiString sSettings()		{ return sSetting(mPlural); }
    static uiString sSetup()		{ return tr("Setup"); }
    static uiString sShift();
    static uiString sSPNumber()		{ return tr("Shotpoint number"); }
    static uiString sShow()		{ return tr("Show"); }
    static uiString sShowIn();
    static uiString sSize()		{ return tr("Size"); }
    static uiString sSkip()		{ return tr("Skip"); }
    static uiString sSlice()		{ return tr("Slice"); }
    static uiString sSource(int num=1)	{ return tr("Source",0,num); }
    static uiString sSouth(bool abb)	{ return abb ? tr("S"):tr("South"); }
    static uiString sSpecify();
    static uiString sSpecifyOut();
    static uiString sStatistics()	{ return tr("Statistics"); }
    static uiString sStatus()		{ return tr("Status"); }
    static uiString sStdDev()		{ return tr("Standard Deviation"); }
    static uiString sSteering()		{ return tr("Steering"); }
    static uiString sStep(int num=1)	{ return tr("Step",0,num); }
    static uiString sSteps()		{ return sStep(mPlural); }
    static uiString sStepout()		{ return tr("Stepout"); }
    static uiString sStop()		{ return tr("Stop"); }
    static uiString sStorage()		{ return tr("Storage"); }
    static uiString sStorageDir();
    static uiString sStored();
    static uiString sStratigraphy();
    static uiString sSurface()		{ return tr("Surface"); }
    static uiString sSurvey(int num=1)  { return tr("Survey",0,num); }
    static uiString sSurveys()		{ return sSurvey(mPlural); }
    static uiString sTakeSnapshot()	{ return tr("Take Snapshot"); }
    static uiString sStart()		{ return tr("Start"); }
    static uiString sTable(int num=1)	{ return tr("Table",0,num); }
    static uiString sTension()		{ return tr("Tension"); }
    static uiString sTerminate()	{ return tr("Terminate"); }
    static uiString sTile()		{ return tr("Tile"); }
    static uiString sTime()		{ return tr("Time"); }
    static uiString sTitle()		{ return tr("Title"); }
    static uiString sTmpStor()		{ return tr("Temporary storage "
								"location :"); }
    static uiString sToolbar()		{ return tr("Toolbar"); }
    static uiString sTools()		{ return tr("Tools"); }
    static uiString sTooltip();
    static uiString sTop()		{ return tr("Top"); }
    static uiString sTopHor()		{ return tr("Top Horizon"); }
    static uiString sTrace(int num=1)	{ return tr("Trace",0,num); }
    static uiString sTraceNumber()	{ return tr("Trace number"); }
    static uiString sTraceRange()	{ return tr("Trace range"); }
    static uiString sTrack();
    static uiString sTracking()		{ return tr("Tracking"); }
    static uiString sTransform()	{ return tr("Transform"); }
    static uiString sTransparency()	{ return tr("Transparency"); }
    static uiString sTVD()		{ return tr("TVD", "True Vert Dpth"); }
    static uiString sTVDRelSRD()	{ return tr("TVD rel SRD",
						"TVD relative to SRD");}
    static uiString sTVDRelKB()		{ return tr("TVD rel KB",
						"TVD relative to KB");}
    static uiString sTVDRelGL()		{ return tr("TVD rel GL",
						"TVD relative to GL");}
    static uiString sTVDSS()		{ return tr("TVDSS",
						"True Vertical Depth Sub Sea");}
    static uiString sTWT(bool abbr=true)
    { return abbr ? tr("TWT") : tr("Two Way Travel Time"); }
    static uiString sType()		{ return tr("Type"); }
    static uiString sUndefVal()		{ return tr("Undefined Value"); }
    static uiString sUndo()		{ return tr("Undo"); }
    static uiString sUnit(int num=1)	{ return tr("Unit",0,num); }
    static uiString sUnknown()		{ return tr("Unknown"); }
    static uiString sUnlock()		{ return tr("Unlock"); }
    static uiString sUnload()		{ return tr("Unload"); }
    static uiString sUp()		{ return tr("Up"); }
    static uiString sUse()		{ return tr("Use"); }
    static uiString sUtilities()	{ return tr("Utilities"); }
    static uiString sValue(int num=1)	{ return tr("Value",0,num); }
    static uiString sVariable( bool math )
    { return math ? tr("Variable","in math") : tr("Variable","changing"); }
    static uiString sVelocity()		{ return tr("Velocity"); }
    static uiString sVertical()		{ return tr("Vertical"); }
    static uiString sVideo()		{ return tr("Video"); }
    static uiString sVolume();
    static uiString sVolumep(int num=1);
    static uiString sView()		{ return tr("View"); }
    static uiString sUseSingleColor()	{ return tr("Use Single Color"); }
    static uiString sWarning()		{ return tr("Warning"); }
    static uiString sWavelet(int num=1);
    static uiString sWaveNumber(int num=1);
    static uiString sWell(int num=1);
    static uiString sWells()		{ return sWell(mPlural); }
    static uiString sWellsHandled()	{ return tr("Wells handled"); }
    static uiString sWellLog(int num=1);
    static uiString sWest(bool abb)	{ return abb ? tr("W"):tr("West"); }
    static uiString sWidth()		{ return tr("Width"); }
    static uiString sWiggle()		{ return tr("Wiggle"); }
    static uiString sWindow()		{ return tr("Window"); }
    static uiString sWizBack()		{ return toUiString("<< %1")
							.arg(sBack()); }
    static uiString sWizNext()		{ return toUiString("%1 >>")
							.arg(sNext()); }
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
	"Prestack Data", and similar */
};


//! Adds '...' to string, usable for menu items
#define m3Dots( txt ) \
    uiStrings::phrThreeDots( txt, false )

#define mJoinUiStrs( txt1, txt2 ) \
   uiStrings::phrJoinStrings( uiStrings::txt1, uiStrings::txt2 )

//! Shortcut handy macro for during development
#define mTODONotImplPhrase() \
    uiStrings::phrTODONotImpl( ::className(*this) )

//! Puts untranslated internal in pErrMsg and in uiRetVal and returns that
#define mPutInternalInUiRv( uirv, msg, act ) \
{ \
    pErrMsg( msg ); uirv.add( mINTERNAL(msg) ); act; \
}

//! As mPutInternalInUiRv but also returns the uiRetVal
#define mRetInternalInUiRv( uirv, msg ) \
    mPutInternalInUiRv( uirv, msg, return uirv )

