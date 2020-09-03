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
class TrcKey;


#define mDIAGNOSTIC(s) uiStrings::phrDiagnostic(s)
#define mINTERNAL(s) uiStrings::phrInternalErr(s)


/*!\brief Phrases and words that can (and should!) be re-used when possible.

  OpendTect has a tremendous amount of translatable strings; translating these
  into a non-English language can be a huge amount of work. To keep the work
  at least to a minimum we should try to re-use as many translations as
  possible.

  If you need the words here but maybe in another form, think about using:
  * toUpper() and toLower()
  * parenthesize(), embed(), optional(), quote(), ...

  Do not construct your own phrases by combining various words. Languages have
  different order, different translations for word combinations, and many more
  traps. The result will in general be ranging from incorrect to
  incomprehensible and usually hilarious.

*/

mExpClass(Basic) uiStrings
{ mODTextTranslationClass(uiStrings);
public:


// Phrases

    static uiPhrase phrAdd(const uiWord&);
    static uiPhrase phrAllocating(od_int64);
    static uiPhrase phrCalculate(const uiWord&);
    static uiPhrase phrCalculateFrom(const uiWord&);
    static uiPhrase phrCannotAdd(const uiWord&);
    static uiPhrase phrCannotCalculate(const uiWord&);
    static uiPhrase phrCannotCopy(const uiWord&);
    static uiPhrase phrCannotCreate(const uiWord&);
    static uiPhrase phrCannotCreateTempFile();
    static uiPhrase phrCannotCreateDBEntryFor(const uiWord&);
    static uiPhrase phrCannotCreateDirectory(const char*);
    static uiPhrase phrCannotCreateHor();
    static uiPhrase phrCannotEdit(const uiWord&);
    static uiPhrase phrCannotExtract(const uiWord&);
    static uiPhrase phrCannotFind(const uiWord&);
    static uiPhrase phrCannotFind(const char*);
    static uiPhrase phrCannotFindDBEntry(const uiString&);
    static uiPhrase phrCannotFindDBEntry(const DBKey&);
    static uiPhrase phrCannotImport(const uiWord&);
    static uiPhrase phrCannotLoad(const uiWord&);
    static uiPhrase phrCannotLoad(const char*);
    static uiPhrase phrCannotOpen(const uiWord&);
    static uiPhrase phrCannotOpen(const char*,bool forread);
    static uiPhrase phrCannotOpenForRead(const char*);
    static uiPhrase phrCannotOpenForWrite(const char*);
    static uiPhrase phrCannotParse(const char*);
    static uiPhrase phrCannotRead(const char*);
    static uiPhrase phrCannotRead(const uiWord&);
    static uiPhrase phrCannotRemove(const uiWord&);
    static uiPhrase phrCannotRemove(const char*);
    static uiPhrase phrCannotSave(const uiWord&);
    static uiPhrase phrCannotSave(const char*);
    static uiPhrase phrCannotStart(const char*);
    static uiPhrase phrCannotStart(const uiWord&);
    static uiPhrase phrCannotUnZip(const uiWord&);
    static uiPhrase phrCannotWrite(const uiWord&);
    static uiPhrase phrCannotWrite(const char*);
    static uiPhrase phrCannotWriteDBEntry(const uiWord&);
    static uiPhrase phrCannotZip(const uiWord&);
    static uiPhrase phrCheck(const uiWord&);
    static uiPhrase phrClose(const uiWord&);
    static uiPhrase phrCopy(const uiWord&);
    static uiPhrase phrCreate(const uiWord&);
    static uiPhrase phrCreateNew(const uiWord&);
    static uiPhrase phrCrossPlot(const uiWord&);
    static uiPhrase phrCrossline(const uiWord&);
    static uiPhrase phrData(const uiWord&);
    static uiPhrase phrDiskSpace();
    static uiPhrase phrDelete(const uiWord&);
    static uiPhrase phrDiagnostic(const char*);
    static uiPhrase phrDoesNotExist(const uiWord&);
    static uiPhrase phrEdit(const char*);
    static uiPhrase phrEdit(const uiWord&);
    static uiPhrase phrEnter(const uiWord&);
    static uiPhrase phrErrCalculating(const uiWord&);
    static uiPhrase phrErrDuringIO(bool read,const char* objnm=nullptr);
    static uiPhrase phrErrDuringIO(bool read,const uiWord&);
    static uiPhrase phrErrDuringRead( const char* objnm=nullptr )
		    { return phrErrDuringIO( true, objnm ); }
    static uiPhrase phrErrDuringRead( const uiWord& subj )
		    { return phrErrDuringIO( true, subj ); }
    static uiPhrase phrErrDuringWrite( const char* objnm=nullptr )
		    { return phrErrDuringIO( false, objnm ); }
    static uiPhrase phrErrDuringWrite( const uiWord& subj )
		    { return phrErrDuringIO( false, subj ); }
    static uiPhrase phrErrDuringCalculations();
    static uiPhrase phrExistsContinue(const uiWord&,bool overwrite);
    static uiPhrase phrExitOD();
    static uiPhrase phrExport(const uiWord&);
    static uiPhrase phrExtract(const uiWord&);
    static uiPhrase phrFileDoesNotExist(const char*);
    static uiPhrase phrGenerating(const uiWord&);
    static uiPhrase phrHandled(const uiWord&);
    static uiPhrase phrHandling(const uiWord&);
    static uiPhrase phrImport(const uiWord&);
    static uiPhrase phrInline(const uiWord&);
    static uiPhrase phrInput(const uiWord&);
    static uiPhrase phrParamMissing(const char* paramname);
    static uiPhrase phrPosNotFound(const TrcKey&);
    static uiPhrase phrInsert(const uiWord&);
    static uiPhrase phrInternalErr(const char*); // will add 'contact support'
    static uiPhrase phrInvalid(const uiWord&);
    static uiPhrase phrLoad(const uiWord&);
    static uiPhrase phrLoading(const uiWord&);
    static uiPhrase phrManage(const uiWord&);
    static uiPhrase phrMerge(const char*);
    static uiPhrase phrMerge(const uiWord&);
    static uiPhrase phrModify(const uiWord&);
    static uiPhrase phrNotImplInThisVersion(const char* fromver);
    static uiPhrase phrIsNotSaved(const uiWord&);
    static uiPhrase phrIsNotSavedSaveNow(const uiWord&);
    static uiPhrase phrOpen(const uiWord&);
    static uiPhrase phrOutput(const uiWord&);
    static uiPhrase phrOutputFileExistsOverwrite();
    static uiPhrase phrPlsContactSupport(bool firstconsultdoc);
    static uiPhrase phrPlsCheckThe(const uiWord&);
    static uiPhrase phrPlsSelectAtLeastOne(const uiWord&);
    static uiPhrase phrPlsSpecifyAtLeastOne(const uiWord&);
    static uiPhrase phrRead(const uiWord&);
    static uiPhrase phrReading(const uiWord&);
    static uiPhrase phrRemove(const char*);
    static uiPhrase phrRemove(const uiWord&);
    static uiPhrase phrRemoveSelected(const uiWord&);
    static uiPhrase phrRename(const uiWord&);
    static uiPhrase phrRestart(const uiWord&);
    static uiPhrase phrSave(const char*);
    static uiPhrase phrSave(const uiWord&);
    static uiPhrase phrSaveAs(const char*);
    static uiPhrase phrSaveAs(const uiWord&);
    static uiPhrase phrSelect(const uiWord&);
    static uiPhrase phrSelectObjectWrongType(const uiWord&);
    static uiPhrase phrSelectPos(const uiWord&);
    static uiPhrase phrSetAs(const uiWord&);
    static uiPhrase phrShowIn(const uiWord&);
    static uiPhrase phrSorting(const uiWord&);
    static uiPhrase phrSpecify(const uiWord&);
    static uiPhrase phrStart(const uiWord&);
    static uiPhrase phrStarting(const uiWord&);
    static uiPhrase phrStop(const uiWord&);
    static uiPhrase phrStorageDir(const uiWord&);
    static uiPhrase phrSuccessfullyExported(const uiWord&);
    static uiPhrase phrTODONotImpl(const char* clssname);
    static uiPhrase phrThreeDots(const uiWord&,bool immediate=false);
    static uiPhrase phrUnexpected(const uiWord&,const char* what=nullptr);
    static uiPhrase phrWrite(const uiWord&);
    static uiPhrase phrWriting(const uiWord&);
    static uiPhrase phrWritten(const uiWord&);
    static uiPhrase phrXcoordinate(const uiWord&);
    static uiPhrase phrYcoordinate(const uiWord&);
    static uiPhrase phrZIn(const uiWord&);
    static uiPhrase phrZRange(const uiWord&);


    //Phrases that don't need specifics, can be used when context is obvious
    static uiPhrase phrCannotAllocateMemory(od_int64 reqsz=-1);
    static uiPhrase phrCannotFindAttrName();
    static uiPhrase phrCannotFindObjInDB();
    static uiPhrase phrCannotOpenInpFile(int n=1);
    static uiPhrase phrCannotOpenOutpFile(int n=1);
    static uiPhrase phrCannotReadHor();
    static uiPhrase phrCannotReadInp();
    static uiPhrase phrCannotWriteSettings();
    static uiPhrase phrCheckPermissions();
    static uiPhrase phrCheckUnits();
    static uiPhrase phrDBIDNotValid();
    static uiPhrase phrEnterValidName();
    static uiPhrase phrSaveBodyFail();
    static uiPhrase phrSelOutpFile();
    static uiPhrase phrSpecifyOutput();


//Words

    static uiWord sDistUnitString(bool isfeet,bool abbr=true);
	/*!< returns "m", "ft", or translated "meter", or "feet" */
    static uiWord sTimeUnitString(bool ismilli=false,bool abbr=true);
	/*!< returns "s", "ms", or translated "seconds" or "milliseconds" */
    static uiWord sIsoMapType( bool istime );
	/*!< returns "Isochore" or "Isochron" */
    static uiWord sSeisObjName(bool is2d,bool is3d,bool isprestack,
				     bool both_2d_3d_in_context=false,
				     bool both_pre_post_in_context=false);
	/*!< returns names such as "2D Data", "Cube", "Prestack Data", ... */
    static uiWord sSeisGeomTypeName(bool is2d,bool isps);
    static inline uiWord sRangeTemplate( bool withstep )
    { return withstep ? toUiString("%1 - %2 [%3]") : toUiString("%1 - %2"); }

    static uiWord sMemSizeString(od_int64);

    // Words with no inline definition are incorrect;
    // do not use or turn into phrase

    static uiWord s2D()			{ return tr("2D"); }
    static uiWord s2DHorizon(int num=1) { return tr("2D Horizon",0,num); }
    static uiWord s3DHorizon(int num=1) { return tr("3D Horizon",0,num); }
    static uiWord s2DLine(int num=1)	{ return tr("2D line",0,num); }
    static uiWord s2DPlane(int n=1)	{ return tr("2D Plane",0,n); }
    static uiWord s3D()			{ return tr("3D"); }
    static uiWord sAbort()		{ return tr("Abort"); }
    static uiWord sAbout()		{ return tr("About"); }
    static uiWord sAbove()		{ return tr("Above"); }
    static uiWord sAbsolute()		{ return tr("Absolute"); }
    static uiWord sAccept()		{ return tr("Accept"); }
    static uiWord sAcoustic()		{ return tr("Acoustic"); }
    static uiWord sAction()		{ return tr("Action"); }
    static uiWord sAdd()		{ return tr("Add"); }
    static uiWord sAdvanced(const uiWord& subj=uiString::empty());
    static uiWord sAdvancedSettings()	{ return sAdvanced(sSettings()); }
    static uiWord sAlgorithm()		{ return tr("Algorithm"); }
    static uiWord sAlignment()		{ return tr("Alignment"); }
    static uiWord sAll()		{ return tr("All"); }
    static uiWord sAlpha()		{ return tr("Alpha"); }
    static uiWord sAmplitude(int n=1)	{ return tr("Amplitude",0,n); }
    static uiWord sAnalyze()		{ return tr("Analyze"); }
    static uiWord sAnalysis()		{ return tr("Analysis"); }
    static uiWord sAnd()		{ return tr("and"); }
    static uiWord sAngle()		{ return tr("Angle"); }
    static uiWord sAnnotation(int n=1)	{ return tr("Annotation",0,n); }
    static uiWord sApply()		{ return tr("Apply"); }
    static uiWord sArrow()		{ return tr("Arrow","Shape"); }
    static uiWord sArea()		{ return tr("Area"); }
    static uiWord sASCII()		{ return tr("ASCII"); }
    static uiWord sASCIIFile()		{ return tr("ASCII File"); }
    static uiWord sAt()			{ return tr("at"); }
    static uiWord sAttribName()		{ return tr("Attribute Name"); }
    static uiWord sAttribute(int n=1)	{ return tr("Attribute",0,n); }
    static uiWord sAttributeSet(int n=1){ return tr("Attribute Set",0,n);}
    static uiWord sAttributeStats()	{ return tr("Attribute Statistics"); }
    static uiWord sAuto()		{ return tr("Auto"); }
    static uiWord sAutomatic()		{ return tr("Automatic"); }
    static uiWord sAvailable()		{ return tr("Available"); }
    static uiWord sAverage()		{ return tr("Average"); }
    static uiWord sAxis()		{ return tr("Axis"); }
    static uiWord sAzimuth()		{ return tr("Azimuth"); }
    static uiWord sBack()		{ return tr("Back"); }
    static uiWord sBase( bool math )	{ return math ? tr("Base","in math")
						      : tr("Base","of layer"); }
    static uiWord sBasemap()		{ return tr("Basemap"); }
    static uiWord sBasic()		{ return tr("Basic"); }
    static uiWord sBatch()		{ return tr("Batch"); }
    static uiWord sBatchProgram()	{ return tr("Batch Program"); }
    static uiWord sBelow()		{ return tr("Below"); }
    static uiWord sBinary()		{ return tr("Binary"); }
    static uiWord sBlue()		{ return tr("Blue"); }
    static uiWord sBody(int n=1)	{ return tr("Body",0,n); }
    static uiWord sBoth()		{ return tr("Both"); }
    static uiWord sBottom()		{ return tr("Bottom"); }
    static uiWord sBottomHor()		{ return tr("Bottom Horizon"); }
    static uiWord sBoundary(int n=1)	{ return tr("Boundary",0,n); }
    static uiWord sBrowse()		{ return tr("Browse"); }
    static uiWord sByte(int n=1)	{ return tr("Byte",0,n); }
    static uiWord sCalculate()		{ return tr("Calculate"); }
    static uiWord sCalculated()		{ return tr("Calculated"); }
    static uiWord sCalculating()	{ return tr("Calculating"); }
    static uiWord sCalculateFrom()	{ return tr("Calculate From"); }
    static uiWord sCancel()		{ return tr("Cancel"); }
    static uiWord sCancelled()		{ return tr("Cancelled"); }
    static uiWord sCenter()		{ return tr("Center","Alignment"); }
    static uiWord sCentral()		{ return tr("Central"); }
    static uiWord sChange()		{ return tr("Change"); }
    static uiWord sCircle()		{ return tr("Circle","shape"); }
    static uiWord sClass()		{ return tr("Class"); }
    static uiWord sClassification()	{ return tr("Classification"); }
    static uiWord sClear()		{ return tr("Clear"); }
    static uiWord sClip()		{ return tr("Clip","verb"); }
    static uiWord sClose()		{ return tr("Close"); }
    static uiWord sCode(int n=1)	{ return tr("Code",0,n); }
    static uiWord sCoefficient()	{ return tr("Coefficient"); }
    static uiWord sCollectingData()	{ return tr("Collecting Data"); }
    static uiWord sColor(int n=1)	{ return tr("Color",0,n); }
    static uiWord sColorTable(int n=1)	{ return tr("Color Table",0,n); }
    static uiWord sColumn(int n=1)	{ return tr("Column",0,n); }
    static uiWord sComment()		{ return tr("Comment"); }
    static uiWord sCommand()		{ return tr("Command"); }
    static uiWord sComponent()		{ return tr("Component"); }
    static uiWord sConnection()		{ return tr("Connection"); }
    static uiWord sConstant( bool math )
    { return math ? tr("Constant","in math"):tr("Constant","not changing"); }
    static uiWord sContent()		{ return tr("Content"); }
    static uiWord sContinue()		{ return tr("Continue"); }
    static uiWord sContour(int n=1)	{ return tr("Contour",0,n); }
    static uiWord sConvert()		{ return tr("Convert"); }
    static uiWord sConversion()		{ return tr("Conversion"); }
    static uiWord sCoordSys()		{ return tr("Coordinate System"); }
    static uiWord sCoordinate(int n=1)	{ return tr("Coordinate",0,n); }
    static uiWord sCopy()		{ return tr("Copy"); }
    static uiWord sCorrelCoeff()	{ return tr("Correlation Coefficient");}
    static uiWord sCorrelation(int n=1)	{ return tr("Correlation",0,n); }
    static uiWord sCount()		{ return tr("Count"); }
    static uiWord sCrAt()		{ return tr("Created at"); }
    static uiWord sCrBy()		{ return tr("Created by"); }
    static uiWord sCreate()		{ return tr("Create"); }
    static uiWord sCrFrom()		{ return tr("Created from"); }
    static uiWord sCreateGroup()	{ return tr("Create Group"); }
    static uiWord sCreateNew()		{ return tr("Create New"); }
    static uiWord sCreateOutput()	{ return tr("Create Output"); }
    static uiWord sCrl()		{ return tr("Crl","abbr Cross-line");}
    static uiWord sCross()		{ return tr("Cross","Shape"); }
    static uiWord sCrossPlot()		{ return tr("Cross Plot"); }
    static uiWord sCrossline(int n=1)	{ return tr("Cross-line", 0, n ); }
    static uiWord sCrosslineDip()	{ return sLineDip(false); }
    static uiWord sCrosslineNumber(int n=1)
					{ return tr("Cross-line Number",0,n); }
    static uiWord sCrosslineRange()	{ return tr("Crossline Range"); }
    static uiWord sCrossPlotData()	   { return tr("Cross Plot Data"); }
    static uiWord sCube(int n=1)	{ return tr("Cube",0,n); }
    static uiWord sCurve()		{ return tr("Curve"); }
    static uiWord sCurvature()		{ return tr("Curvature"); }
    static uiWord sData()		{ return tr("Data"); }
    static uiWord sDate()		{ return tr("Date"); }
    static uiWord sDataRange()		{ return tr("Data Range"); }
    static uiWord sDataStore(int n=1)	{ return tr("Data Store",0,n); }
    static uiWord sDataSet()		{ return tr("Data Set"); }
    static uiWord sDataType()		{ return tr("Data Type"); }
    static uiWord sDBEntry()		{ return tr("DataBase Entry"); }
    static uiWord sDecimal()		{ return tr("Decimal"); }
    static uiWord sDeg()		{ return tr("deg","unit for angles"); }
    static uiWord sDegree(int num=1)	{ return tr("Degree",0,num); }
    static uiWord sDefault()		{ return tr("Default"); }
    static uiWord sDefine()		{ return tr("Define"); }
    static uiWord sDefined()		{ return tr("Defined"); }
    static uiWord sDefinition(int n=1)	{ return tr("Definition",0,n); }
    static uiWord sDelta()		{ return tr("Delta","Difference"); }
    static uiWord sDelete()		{ return tr("Delete"); }
    static uiWord sDensity()		{ return tr("Density"); }
    static uiWord sDepth()	        { return tr("Depth"); }
    static uiWord sDepthRange()		{ return tr("Depth Range"); }
    static uiWord sDescription()	{ return tr("Description"); }
    static uiWord sDesktop()		{ return tr("Desktop"); }
    static uiWord sDestination()	{ return tr("Destination"); }
    static uiWord sdGB()		{ return tr("dGB Earth Sciences"); }
    static uiWord sDifference()		{ return tr("Difference"); }
    static uiWord sDimension()		{ return tr("Dimension"); }
    static uiWord sDip()		{ return tr("Dip"); }
    static uiWord sDirection()		{ return tr("Direction"); }
    static uiWord sDirectional()	{ return tr("Directional"); }
    static uiWord sDirectory()		{ return tr("Directory"); }
    static uiWord sDisable()		{ return tr("Disable"); }
    static uiWord sDisabled()		{ return tr("Disabled"); }
    static uiWord sDiscard()		{ return tr("Discard"); }
    static uiWord sDismiss()		{ return tr("Dismiss"); }
    static uiWord sDisplay()		{ return tr("Display"); }
    static uiWord sDisplayProperties()	{ return tr("Display Properties"); }
    static uiWord sDistance()		{ return tr("Distance"); }
    static uiWord sDistribution(int n=1){ return tr("Distribution",0,n); }
    static uiWord sDone()		{ return tr("Done"); }
    static uiWord sDown()		{ return tr("Down"); }
    static uiWord sDraw()		{ return tr("Draw"); }
    static uiWord sDynamic()		{ return tr("Dynamic"); }
    static uiWord sEach()		{ return tr("Each"); }
    static uiWord sEast( bool abbr )
    { return abbr ? tr("E","abbr East") : tr("East"); }
    static uiWord sEdit()		{ return tr("Edit"); }
    static uiWord sEnable()		{ return tr("Enable"); }
    static uiWord sElastic()		{ return tr("Elastic"); }
    static uiWord sEmpty()		{ return tr("Empty"); }
    static uiWord sEnabled()		{ return tr("Enabled"); }
    static uiWord sEnter()		{ return tr("Enter"); }
    static uiWord sEpsilon()		{ return tr("Epsilon"); }
    static uiWord sError()		{ return tr("Error"); }
    static uiWord sEta()		{ return tr("Eta"); }
    static uiWord sEvaluate()		{ return tr("Evaluate"); }
    static uiWord sEvent(int n=1)	{ return tr("Event",0,n); }
    static uiWord sExamine()		{ return tr("Examine"); }
    static uiWord sExit()		{ return tr("Exit"); }
    static uiWord sExponential()	{ return tr("Exponential"); }
    static uiWord sExport()		{ return tr("Export"); }
    static uiWord sExported()		{ return tr("Exported"); }
    static uiWord sExpectation()	{ return tr("Expectation"); }
    static uiWord sExtension()		{ return tr("Extension"); }
    static uiWord sExtract()		{ return tr("Extract"); }
    static uiWord sExtrapolate()	{ return tr("Extrapolate"); }
    static uiWord sFactor(int n=1)	{ return tr("Factor",0,n); }
    static uiWord sFailed()		{ return tr("Failed"); }
    static uiWord sFault(int n=1)	{ return tr("Fault",0,n); }
    static uiWord sFaultData()		{ return tr("Fault Data"); }
    static uiWord sFaultName(int n=1)	{ return tr("Fault Name",0,n); }
    static uiWord sFaultStickSet(int n=1) { return tr("FaultStickSet",0,n); }
    static uiString sFaultSet(int num=1){ return tr("FaultSet",0,num); }
    static uiWord sFeet( bool abbr )
    { return abbr ? toUiString("ft") : tr("Feet","not meter"); }
    static uiWord sFFT()		{ return tr("FFT"); }
    static uiWord sFile(int n=1)        { return tr("File",0,n); }
    static uiWord sFileName(int n=1)	{ return tr("File Name",0,n); }
    static uiWord sFilter(int n=1)	{ return tr("Filter",0,n); }
    static uiWord sFiltering()		{ return tr("Filtering"); }
    static uiWord sFilters()		{ return sFilter(mPlural); }
    static uiWord sFinish()		{ return tr("Finish"); }
    static uiWord sFinished()		{ return tr("Finished"); }
    static uiWord sFirst()		{ return tr("First"); }
    static uiWord sFixed()		{ return tr("Fixed"); }
    static uiWord sFlatten()		{ return tr("Flatten"); }
    static uiWord sFlattened()		{ return tr("Flattened"); }
    static uiWord sFlip()		{ return tr("Flip"); }
    static uiWord sFlipLeftRight()	{ return tr("Flip left/right"); }
    static uiWord sFolder(int n=1)	{ return tr("Folder",0,n); }
    static uiWord sFont(int n=1)	{ return tr("Font",0,n); }
    static uiWord sFormat()		{ return tr("Format","noun"); }
    static uiWord sFormula()		{ return tr("Formula"); }
    static uiWord sFrequency(int n=1)	{ return tr("Frequency",0,n); }
    static uiWord sFull()		{ return tr("Full"); }
    static uiWord sGate()		{ return tr("Gate"); }
    static uiWord sGeneral()		{ return tr("General"); }
    static uiWord sGenerate()		{ return tr("Generate"); }
    static uiWord sGenerating()		{ return tr("Generating"); }
    static uiWord sGeometry(int n=1)	{ return tr("Geometry",0,n); }
    static uiWord sGo()			{ return tr("Go"); }
    static uiWord sGradient()		{ return tr("Gradient"); }
    static uiWord sGreen()		{ return tr("Green"); }
    static uiWord sGroup(int n=1)	{ return tr("Group",0,n); }
    static uiWord sGrid(int n=1)	{ return tr("Grid",0,n); }
    static uiWord sGridding()		{ return tr("Gridding"); }
    static uiWord sHalf()		{ return tr("Half"); }
    static uiWord sHeader()		{ return tr("Header"); }
    static uiWord sHeight()		{ return tr("Height"); }
    static uiWord sHelp()		{ return tr("Help"); }
    static uiWord sHide()		{ return tr("Hide"); }
    static uiWord sHigh()		{ return tr("High"); }
    static uiWord sHigher()		{ return tr("Higher"); }
    static uiWord sHighest()		{ return tr("Highest"); }
    static uiWord sHistogram()		{ return tr("Histogram"); }
    static uiWord sHorizon(int n=1)	{ return tr("Horizon",0,n); }
    static uiWord sHorizonData()	{ return tr("Horizon Data"); }
    static uiWord sHorizonName(int n=1) { return tr("Horizon Name",0,n); }
    static uiWord sHorizontal()		{ return tr("Horizontal"); }
    static uiWord sHost()		{ return tr("Host"); }
    static uiWord sHostName()		{ return tr("Host Name"); }
    static uiWord sID()			{ return tr("ID"); }
    static uiWord sIcon(int n=1)	{ return tr("Icon",0,n); }
    static uiWord sImage(int n=1)	{ return tr("Image",0,n); }
    static uiWord sImport()		{ return tr("Import"); }
    static uiWord sImported()		{ return tr("Imported"); }
    static uiWord sImporting()		{ return tr("Importing"); }
    static uiWord sImpSuccess()		{ return tr("Import successful"); }
    static uiWord sIncidenceAngle()	{ return tr("Incidence Angle"); }
    static uiWord sInclination()	{ return tr("Inclination"); }
    static uiWord sInfo()		{ return tr("Info"); }
    static uiWord sInformation()	{ return tr("Information"); }
    static uiWord sIngredient(int n=1)	{ return tr("Ingredient",0,n); }
    static uiWord sInner()		{ return tr("Inner"); }
    static uiWord sInitializing()	{ return tr("Initializing"); }
    static uiWord sInl()		{ return tr("Inl","abbr In-line"); }
    static uiWord sInline(int n=1)	{ return tr("In-line",0,n); }
    static uiWord sInlineDip()		{ return tr("Inline Dip"); }
    static uiWord sInlineNumber(int num=1) { return tr("Inline Number",0,num); }
    static uiWord sInlineRange()	{ return tr("Inline Range"); }
    static uiWord sInputASCIIFile()	{ return tr("Input ASCII File"); }
    static uiWord sIsochore()		{ return tr("Isochore"); }
    static uiWord sIsochron()		{ return tr("Isochron"); }
    static uiWord sParsMissing()	{ return tr("Parameters missing"); }
    static uiWord sInput()		{ return tr("Input"); }
    static uiWord sInputData()		{ return tr("Input Data"); }
    static uiWord sInputFile(int n=1)	{ return tr("Input File",0,n); }
    static uiWord sInputSelection()	{ return tr("Input Selection"); }
    static uiWord sInterpolate()	{ return tr("Interpolate"); }
    static uiWord sInterval()		{ return tr("Interval"); }
    static uiWord sInsert()		{ return tr("Insert"); }
    static uiWord sInside()		{ return tr("Inside"); }
    static uiWord sInterpolating()	{ return tr("Interpolating"); }
    static uiWord sInterpolation()	{ return tr("Interpolation"); }
    static uiWord sInterpretation(int n=1) { return tr("Interpretation",0,n); }
    static uiWord sIntersection()	{ return tr("Intersection","of sets"); }
    static uiWord sInvInpFile()		{ return tr("Invalid input file"); }
    static uiWord sInvalid()		{ return tr("Invalid"); }
    static uiWord sItem(int n=1)	{ return tr("Item",0,n); }
    static uiWord sIteration()		{ return tr("Iteration"); }
    static uiWord sIterating()		{ return tr("Iterating"); }
    static uiWord sJob()		{ return tr("Job"); }
    static uiWord sLanguage()		{ return tr("Language"); }
    static uiWord sKeep()		{ return tr("Keep"); }
    static uiWord sKeyword()		{ return tr("Keyword"); }
    static uiWord sLast()		{ return tr("Last"); }
    static uiWord sLastModified()	{ return tr("Last Modified"); }
    static uiWord sLatitude(bool abbr)	{ return abbr?tr("Lat"):tr("Latitude");}
    static uiWord sLayer(int n=1)	{ return tr("Layer",0,n); }
    static uiWord sLeft()		{ return tr("Left"); }
    static uiWord sLicense(int n=1)	{ return tr("License",0,n); }
    static uiWord sNoLicense()		{ return tr("No License"); }
    static uiWord sLength()		{ return tr("Length"); }
    static uiWord sLevel(int n=1)	{ return tr("Level",0,n); }
    static uiWord sLine(int n=1)	{ return tr("Line",0,n); }
    static uiWord sLinear()		{ return tr("Linear"); }
    static uiWord sLineDip(bool for2d)	{ return for2d ? tr("Line Dip")
						       : tr("Crossline Dip"); }
    static uiWord sLineGeometry()	{ return tr("Line Geometry"); }
    static uiWord sLineName(int n=1)	{ return tr("Line Name",0,n); }
    static uiWord sLineStyle(int n=1)	{ return tr("Line Style",0,n); }
    static uiWord sLink()		{ return tr("Link"); }
    static uiWord sLithology(int n=1)	{ return tr("Lithology",0,n); }
    static uiWord sLoad()		{ return tr("Load"); }
    static uiWord sLocation(int n=1)	{ return tr("Location",0,n); }
    static uiWord sLock()		{ return tr("Lock"); }
    static uiWord sLocked()		{ return tr("Locked"); }
    static uiWord sLog(int n=1)		{ return tr("Log",0,n); }
    static uiWord sLogarithmic()	{ return tr("Logarithmic"); }
    static uiWord sLogName()		{ return tr("Log Name"); }
    static uiWord sLogFile()		{ return tr("Log File"); }
    static uiWord sLogs()		{ return sLog(mPlural); }
    static uiWord sLongitude( bool abbr )
    { return abbr ? tr("Long","not Lat") : tr("Longitude"); }
    static uiWord sLooknFeel()		{ return tr("Look and Feel"); }
    static uiWord sLow()		{ return tr("Low"); }
    static uiWord sManage()		{ return tr("Manage"); }
    static uiWord sManual()		{ return tr("Manual"); }
    static uiWord sMarker(int n=1)	{ return tr("Marker",0,n); }
    static uiWord sMarkerNm(int n=1)	{ return tr("Marker Name",0,n); }
    static uiWord sMD()			{ return tr("MD","Measured Depth"); }
    static uiWord sMatch()		{ return tr("Match"); }
    static uiWord sMaximum()		{ return tr("Maximum"); }
    static uiWord sMedian()		{ return tr("Median"); }
    static uiWord sMenu()		{ return tr("Menu"); }
    static uiWord sMerge()		{ return tr("Merge"); }
    static uiWord sMerging()		{ return tr("Merging"); }
    static uiWord sMeter( bool abbr )
    { return abbr ? toUiString("m") : tr("Meter"); }
    static uiWord sMethod()		{ return tr("Method"); }
    static uiWord sMinimum()		{ return tr("Minimum"); }
    static uiWord sMode()		{ return tr("Mode"); }
    static uiWord sModel(int n=1)	{ return tr("Model",0,n); }
    static uiWord sModelNumber()	{ return tr("Model Number"); }
    static uiWord sModify()		{ return tr("Modify"); }
    static uiWord sMouse()		{ return tr("Mouse"); }
    static uiWord sMove()		{ return tr("Move"); }
    static uiWord sMoveDown()		{ return tr("Move Down"); }
    static uiWord sMoveToBottom()	{ return tr("Move To Bottom"); }
    static uiWord sMoveToTop()		{ return tr("Move To Top"); }
    static uiWord sMoveUp()		{ return tr("Move Up"); }
    static uiWord sMSec( bool abbr, int n=1 )
    { return abbr ? toUiString("ms") : tr("Millisecond",0,n); }
    static uiWord sMultiple()		{ return tr("Multiple"); }
    static uiWord sMute(int n=1)	{ return tr("Mute",0,n); }
    static uiWord sName(int n=1)	{ return tr("Name",0,n); }
    static uiWord sNegative()		{ return tr("Negative"); }
    static uiWord sNew()		{ return tr("New"); }
    static uiWord sNewName()		{ return tr("New Name"); }
    static uiWord sNext()		{ return tr("Next"); }
    static uiWord sNo()			{ return tr("No"); }
    static uiWord sNoInfoAvailable()	{ return tr("No info available"); }
    static uiWord sNode(int n=1)	{ return tr("Node",0,n); }
    static uiWord sNodeIndex(int n=1)	{ return tr("Node Index",0,n); }
    static uiWord sNoLogSel()		{ return tr("No log selected"); }
    static uiWord sNone()		{ return tr("None"); }
    static uiWord sNormal()		{ return tr("Normal"); }
    static uiWord sNormalize()		{ return tr("Normalize"); }
    static uiWord sNorth( bool abbr )
    { return abbr ? tr("N","abbr North") : tr("North"); }
    static uiWord sNotPresent()		{ return tr("Not Present"); }
    static uiWord sNoValidData()	{ return tr("No valid data found"); }
    static uiWord sNrSamples()		{ return tr("Number of Samples"); }
    static uiWord sNumber(int n=1)	{ return tr("Number",0,n); }
    static uiWord sObject()		{ return tr("Object"); }
    static uiWord sObjectID()		{ return tr("Object ID"); }
    static uiWord sOff()		{ return tr("Off","not in action"); }
    static uiWord sOffset()		{ return tr("Offset"); }
    static uiWord sOffsetRange()	{ return tr("Offset Range"); }
    static uiWord sOk()			{ return tr("OK"); }
    static uiWord sOn()			{ return tr("On","in action"); }
    static uiWord sOneWayTT()		{ return tr("One-Way Travel Time"); }
    static uiWord sOnlyAtSections()	{ return tr("Only at Sections"); }
    static uiWord sOpen()		{ return tr("Open","Verb"); }
    static uiWord sOpendTect()		{ return tr("OpendTect"); }
    static uiWord sOperator()		{ return tr("Operator"); }
    static uiWord sOption(int n=1)	{ return tr("Option",0,n); }
    static uiWord sOptions()		{ return sOption(mPlural); }
    static uiWord sOptional()		{ return tr("Optional"); }
    static uiWord sOr()			{ return tr("or"); }
    static uiWord sOrientation()	{ return tr("Orientation"); }
    static uiWord sOrder()		{ return tr("Order"); }
    static uiWord sOther()		{ return tr("Other"); }
    static uiWord sOuter()		{ return tr("Outer"); }
    static uiWord sOutpDataStore()	{ return tr("Output data store"); }
    static uiWord sOutpStatistic()	{ return tr("Output Statistic"); }
    static uiWord sOutput()		{ return tr("Output"); }
    static uiWord sOutputASCIIFile()	{ return tr("Ouput ASCII file"); }
    static uiWord sOutputFile()		{ return tr("Output file"); }
    static uiWord sOutputSelection()	{ return tr("Output Selection"); }
    static uiWord sOutside()		{ return tr("Outside"); }
    static uiWord sOverwrite()		{ return tr("Overwrite"); }
    static uiWord sPackage(int n=1)	{ return tr("Package",0,n); }
    static uiWord sParent(int n=1)	{ return tr("Parent",0,n); }
    static uiWord sParFile()		{ return tr("Parameter File"); }
    static uiWord sParameter(int n=1)	{ return tr("Parameter",0,n); }
    static uiWord sParsIncorrect()
			{ return tr("Missing or incorrect parameter(s)"); }
    static uiWord sPartial()		{ return tr("Partial"); }
    static uiWord sPass()		{ return tr("Pass"); }
    static uiWord sPassword()		{ return tr("Password"); }
    static uiWord sPattern(int n=1)	{ return tr("Pattern",0,n); }
    static uiWord sPause()		{ return tr("Pause"); }
    static uiWord sPercentageDone()	{ return tr("Percentage done"); }
    static uiWord sPhase( bool withunit, bool unitisdeg=true )
    {	uiString ret = tr("Phase");
	return withunit ? ret.withUnit(unitisdeg ? "deg" : "rad") : ret; }
    static uiWord sPick(int n=1)	{ return tr("Pick",0,n); }
    static uiWord sPickSet(int n=1)	{ return tr("PickSet",0,n); }
    static uiWord sPlane(int n=1)	{ return tr("Plane",0,n); }
    static uiWord sPlatform()		{ return tr("Platform"); }
    static uiWord sPlugin(int n=1)	{ return tr("Plugin",0,n); }
    static uiWord sPoint(int n=1)	{ return tr("Point",0,n); }
    static uiWord sPointsDone()		{ return tr("Points done"); }
    static uiWord sPointSet(int n=1)	{ return tr("PointSet",0,n); }
    static uiWord sPolygon(int n=1)	{ return tr("Polygon",0,n); }
    static uiWord sPolyLine(int n=1)	{ return tr("PolyLine",0,n); }
    static uiWord sPolyBody(int num=1)	{ return tr("Polygon Body",0,num); }
    static uiWord sPosition(int n=1)	{ return tr("Position",0,n); }
    static uiWord sPositioning()	{ return tr("Positioning"); }
    static uiWord sPositionsDone()	{ return tr("Positions done"); }
    static uiWord sPositive()		{ return tr("Positive"); }
    static uiWord sPostfix()		{ return tr("Postfix"); }
    static uiWord sPostStack()		{ return tr("Poststack"); }
    static uiWord sPrefix()		{ return tr("Prefix"); }
    static uiWord sPreProcessing()	{ return tr("PreProcessing"); }
    static uiWord sPreStack()		{ return tr("Prestack"); }
    static uiWord sPreStackData()	{ return tr("PreStack Data"); }
    static uiWord sPreStackEvent(int n=1) { return tr("Prestack Event",0,n); }
    static uiWord sPreStackEvents()	{ return sPreStackEvent(mPlural); }
    static uiWord sPreview()		{ return tr("Preview"); }
    static uiWord sPrevious()		{ return tr("Previous"); }
    static uiWord sProbDensFunc( bool abbr, int n=1 )
    { return abbr ? tr("PDF","abbr of Probability Density Function",n)
		  : tr("Probability Density Function", 0,n); }
    static uiWord sProbe(int n=1)	{ return tr("Probe",0,n); }
    static uiWord sProblem(int n=1)	{ return tr("Problem",0,n); }
    static uiWord sProcess(int n=1)	{ return tr("Process",0,n); }
    static uiWord sProcessing()		{ return tr("Processing"); }
    static uiWord sProcessingPars()	{ return tr("Processing parameters"); }
    static uiWord sProceed(const uiString& withwhat=uiString::empty());
    static uiWord sProgram()		{ return tr("Program"); }
    static uiWord sProgress()		{ return tr("Progress"); }
    static uiWord sProject()		{ return tr("Project"); }
    static uiWord sProperties()		{ return sProperty(mPlural); }
    static uiWord sProperty(int n=1)	{ return tr("Property",0,n); }
    static uiWord sRadian(int num=1)	{ return tr("Radian",0,num); }
    static uiWord sRandius()		{ return tr("Randius"); }
    static uiWord sRandom()		{ return tr("Random"); }
    static uiWord sRandomLine(int n=1)	{ return tr("Random Line",0,n); }
    static uiWord sRange(int n=1)	{ return tr("Range",0,n); }
    static uiWord sRead()		{ return tr("Read"); }
    static uiWord sReadingData()	{ return tr("Reading data"); }
    static uiWord sRectangle()		{ return tr("Rectangle"); }
    static uiWord sRed()		{ return tr("Red"); }
    static uiWord sRedo()		{ return tr("Redo"); }
    static uiWord sRegion(int n=1)	{ return tr("Region",0,n); }
    static uiWord sRegionalMarker()	{ return tr("Regional Marker"); }
    static uiWord sRelative()		{ return tr("Relative"); }
    static uiWord sReload()		{ return tr("Reload"); }
    static uiWord sRemove()		{ return tr("Remove"); }
    static uiWord sRemoveSelected()	{ return tr("Remove Selected"); }
    static uiWord sRename()		{ return tr("Rename"); }
    static uiWord sReplace()		{ return tr("Replace"); }
    static uiWord sReport()		{ return tr("Report"); }
    static uiWord sRequired()		{ return tr("Required"); }
    static uiWord sReservoir()		{ return tr("Reservoir"); }
    static uiWord sReset()		{ return tr("Reset"); }
    static uiWord sResolution()		{ return tr("Resolution"); }
    static uiWord sRestart()		{ return tr("Restart"); }
    static uiWord sResume()		{ return tr("Resume"); }
    static uiWord sReversed()		{ return tr("Reversed"); }
    static uiWord sRight()		{ return tr("Right"); }
    static uiWord sRightClick()		{ return tr("<right-click>"); }
    static uiWord sRMO(bool err=false)	{ return err ? tr("RMO error")
						     : tr("RMO"); }
    static uiWord sRMS(bool err=false)	{ return err ? tr("RMS error")
						     : tr("RMS"); }
    static uiWord sRockPhy()		{ return tr("Rock Physics"); }
    static uiWord sRow(int n=1)		{ return tr("Row",0,n); }
    static uiWord sRun()		{ return tr("Run"); }
    static uiWord sSample(int n=1)	{ return tr("Sample",0,n); }
    static uiWord sSampleIntrvl(int n=1){ return tr("Sample Interval",0,n); }
    static uiWord sSave()		{ return tr("Save"); }
    static uiWord sSaveAs()		{ return tr("Save As"); }
    static uiWord sSaveAsDefault()	{ return tr("Save as Default"); }
    static uiWord sSaved()		{ return tr("Saved"); }
    static uiWord sSavingChanges()	{ return tr("Saving changes"); }
    static uiWord sSavingData()		{ return tr("Saving data"); }
    static uiWord sScale()		{ return tr("Scale"); }
    static uiWord sScaling()		{ return tr("Scaling"); }
    static uiWord sScanning()		{ return tr("Scanning"); }
    static uiWord sScene(int n=1)	{ return tr("Scene",0,n); }
    static uiWord sScenes()		{ return sScene(mPlural); }
    static uiWord sSceneWithNr(int nr)	{ return sScene().withNumber(nr); }
    static uiWord sSchedule()		{ return tr("Schedule"); }
    static uiWord sScore()		{ return tr("Score"); }
    static uiWord sScope()		{ return tr("Scope"); }
    static uiWord sSearch()		{ return tr("Search"); }
    static uiWord sSearching()		{ return tr("Searching"); }
    static uiWord sSearchRadius()	{ return tr("Search Radius"); }
    static uiWord sSec( bool abbr, int n=1 )
    { return abbr ? toUiString("s") : tr("Second","unit of time",n); }
    static uiWord sSEGY()		{ return tr("SEG-Y"); }
    static uiWord sSeed(int n=1)	{ return tr("Seed",0,n); }
    static uiWord sSegment()		{ return tr("Segment"); }
    static uiWord sSegmentation()	{ return tr("Segmentation"); }
    static uiWord sSeismic(int n=1)	{ return tr("Seismic",0,n); }
    static uiWord sSeismics()		{ return sSeismic(mPlural); }
    static uiWord sSeismicData()	{ return tr("Seismic Data"); }
    static uiWord sSelAttrib()		{ return tr("Select Attribute"); }
    static uiWord sSelOutpFile();
    static uiWord sSelect()		{ return tr("Select"); }
    static uiWord sSelected()		{ return tr("Selected"); }
    static uiWord sSelectPos()		{ return tr("Select Position"); }
    static uiWord sSelectedLog(int n=1)	{ return tr("Selected Log",0,n); }
    static uiWord sSelection(int n=1)	{ return tr("Selection",0,n); }
    static uiWord sSemblance()		{ return tr("Semblance"); }
    static uiWord sServer()		{ return tr("Server"); }
    static uiWord sSession(int n=1)	{ return tr("Session",0,n); }
    static uiWord sSet(bool verb=true,int n=1)
    { return verb ? tr("Set","verb") : tr("Set","collection",n); }
    static uiWord sSetAs()		{ return tr("Set As"); }
    static uiWord sSettings()		{ return tr("Settings"); }
    static uiWord sSetup()		{ return tr("Setup"); }
    static uiWord sShape()		{ return tr("Shape"); }
    static uiWord sShapefile(int n=1)	{ return tr("Shapefile",0,n); }
    static uiWord sShift()		{ return tr("Shift"); }
    static uiWord sShow()		{ return tr("Show"); }
    static uiWord sShowIn()		{ return tr("Show In"); }
    static uiWord sSimilarity()		{ return tr("Similarity"); }
    static uiWord sSingle()		{ return tr("Single"); }
    static uiWord sSize()		{ return tr("Size"); }
    static uiWord sSkip()		{ return tr("Skip"); }
    static uiWord sSlice()		{ return tr("Slice"); }
    static uiWord sSmooth()		{ return tr("Smooth","verb"); }
    static uiWord sSmoothing()		{ return tr("Smoothing"); }
    static uiWord sSnapping()		{ return tr("Snapping"); }
    static uiWord sSorting()		{ return tr("Sorting","noun"); }
    static uiWord sSource(int n=1)	{ return tr("Source",0,n); }
    static uiWord sSouth( bool abbr )
    { return abbr ? tr("S","abbr South") : tr("South"); }
    static uiWord sSpecify()		{ return tr("Specify"); }
    static uiWord sSpecifyOut();
    static uiWord sSphere()		{ return tr("Sphere"); }
    static uiWord sSPNumber(bool abbr=false)
    { return abbr ? tr("SP") : tr("Shotpoint Number"); }
    static uiWord sSquare()		{ return tr("Square"); }
    static uiWord sStandard()		{ return tr("Standard"); }
    static uiWord sStack()		{ return tr("Stack","verb, seismics"); }
    static uiWord sStart()		{ return tr("Start"); }
    static uiWord sStatistics()		{ return tr("Statistics"); }
    static uiWord sStatus()		{ return tr("Status"); }
    static uiWord sStdDev()		{ return tr("Standard Deviation"); }
    static uiWord sSteering()		{ return tr("Steering"); }
    static uiWord sSteeringCube(int n=1){ return tr("Steering Cube",0,n); }
    static uiWord sStep(int n=1)	{ return tr("Step",0,n); }
    static uiWord sStepout()		{ return tr("Stepout"); }
    static uiWord sSteps()		{ return sStep(mPlural); }
    static uiWord sStick(int n=1)	{ return tr("Stick",0,n); }
    static uiWord sStickIndex(int n=1)	{ return tr("Stick Index",0,n); }
    static uiWord sStop()		{ return tr("Stop"); }
    static uiWord sStorage()		{ return tr("Storage"); }
    static uiWord sStorageType()	{ return tr("Storage type"); }
    static uiWord sStorageDir()		{ return tr("Storage Directory"); }
    static uiWord sStore()		{ return tr("Store"); }
    static uiWord sStored()		{ return tr("Stored" ); }
    static uiWord sStratigraphy()	{ return tr("Stratigraphy"); }
    static uiWord sStyle(int n=1)	{ return tr("Style",0,n); }
    static uiWord sSubSel()		{ return tr("Subselection"); }
    static uiWord sSuccess()		{ return tr("Success"); }
    static uiWord sSum()		{ return tr("Sum"); }
    static uiWord sSurface(int n=1)	{ return tr("Surface",0,n); }
    static uiWord sSurvey(int n=1)	{ return tr("Survey",0,n); }
    static uiWord sSurveys()		{ return sSurvey(mPlural); }
    static uiWord sSynthetic(int n=1)	{ return tr("Synthetic",0,n); }
    static uiWord sSynthetics()		{ return sSynthetic(mPlural); }
    static uiWord sTable(int n=1)	{ return tr("Table",0,n); }
    static uiWord sTaper()		{ return tr("Taper"); }
    static uiWord sTask()		{ return tr("Task"); }
    static uiWord sTension()		{ return tr("Tension"); }
    static uiWord sTakeSnapshot()	{ return tr("Take Snapshot"); }
    static uiWord sTerminate()		{ return tr("Terminate"); }
    static uiWord sText()		{ return tr("Text"); }
    static uiWord sTexture()		{ return tr("Texture"); }
    static uiWord sTheme()		{ return tr("Theme"); }
    static uiWord sThickness()		{ return tr("Thickness"); }
    static uiWord sThinning()		{ return tr("Thinning"); }
    static uiWord sThreshold()		{ return tr("Threshold"); }
    static uiWord sTile()		{ return tr("Tile"); }
    static uiWord sTime()		{ return tr("Time"); }
    static uiWord sTimeSort()		{ return tr("Time Sort"); }
    static uiWord sTimeRange()		{ return tr("Time Range"); }
    static uiWord sTitle()		{ return tr("Title"); }
    static uiWord sTmpStor()	    { return tr("Temporary storage location"); }
    static uiWord sToolBar(int n=1)	{ return tr("Tool Bar",0,n); }
    static uiWord sTools()		{ return tr("Tools"); }
    static uiWord sTop()		{ return tr("Top"); }
    static uiWord sTopHor()		{ return tr("Top Horizon"); }
    static uiWord sTrc()		{ return tr("Trc","abbr trace"); }
    static uiWord sTrace(int n=1)	{ return tr("Trace",0,n); }
    static uiWord sTraceNumber(int n=1)	{ return tr("Trace number",0,n); }
    static uiWord sTraceRange()		{ return tr("Trace Range"); }
    static uiWord sTrack()		{ return tr("Track","verb"); }
    static uiWord sTracking()		{ return tr("Tracking"); }
    static uiWord sTrackPad()		{ return tr("Track Pad"); }
    static uiWord sTransform()		{ return tr("Transform"); }
    static uiWord sTranslator()		{ return tr("Translator"); }
    static uiWord sTransparency()	{ return tr("Transparency"); }
    static uiWord sTransparent()	{ return tr("Transparent"); }
    static uiWord sTriangle()		{ return tr("Triangle"); }
    static uiWord sTriangulation()	{ return tr("Triangulation"); }
    static uiWord sTVD()		{ return tr("TVD", "True Vert Dpth"); }
    static uiWord sTVDRelSRD()		{ return tr("TVD rel SRD",
						    "TVD relative to SRD");}
    static uiWord sTVDRelKB()		{ return tr("TVD rel KB",
						    "TVD relative to KB");}
    static uiWord sTVDRelGL()		{ return tr("TVD rel GL",
						    "TVD relative to GL");}
    static uiWord sTVDSS()		{ return tr("TVDSS",
					    "True Vertical Depth Sub Sea"); }
    static uiWord sTWT(bool abbr=true)
    { return abbr ? tr("TWT") : tr("Two Way Travel Time"); }
    static uiWord sType()		{ return tr("Type"); }
    static uiWord sUndef(bool abbr=true)
    { return abbr ? tr("undef") : tr("Undefined"); }
    static uiWord sUndefVal()		{ return tr("Undefined Value"); }
    static uiWord sUndo()		{ return tr("Undo"); }
    static uiWord sUnion()		{ return tr("Union","of sets"); }
    static uiWord sUnit(int n=1)	{ return tr("Unit",0,n); }
    static uiWord sUnknown()		{ return tr("Unknown"); }
    static uiWord sUnload()		{ return tr("Unload"); }
    static uiWord sUnlock()		{ return tr("Unlock"); }
    static uiWord sUp()			{ return tr("Up"); }
    static uiWord sUpward()		{ return tr("Upward"); }
    static uiWord sUpdate()		{ return tr("Update"); }
    static uiWord sUpdatingDB()		{ return tr("Updating database"); }
    static uiWord sUpdatingDisplay()	{ return tr("Updating display"); }
    static uiWord sUse()		{ return tr("Use"); }
    static uiWord sUsing()		{ return tr("Using"); }
    static uiWord sUseFirst()		{ return tr("Use First"); }
    static uiWord sUseSingleColor()	{ return tr("Use Single Color"); }
    static uiWord sUser()		{ return tr("User"); }
    static uiWord sUserDefined()	{ return tr("User Defined"); }
    static uiWord sUserSettings()	{ return tr("User Settings"); }
    static uiWord sUtilities()		{ return tr("Utilities"); }
    static uiWord sVariable( bool math )
    { return math ? tr("Variable","in math") : tr("Variable","changing"); }
    static uiWord sValue(int n=1)	{ return tr("Value",0,n); }
    static uiWord sVD(bool abbr=false)
    { return abbr ? tr("VD","abbr Var Density") : tr("Variable Density"); }
    static uiWord sVelocity(int n=1)	{ return tr("Velocity",0,n); }
    static uiWord sValRange()		{ return tr("Value Range"); }
    static uiWord sVelRange()		{ return tr("Velocity Range"); }
    static uiWord sVertex(int n=1)	{ return tr("Vertex",0,n); }
    static uiWord sVertical()		{ return tr("Vertical"); }
    static uiWord sVideo()		{ return tr("Video"); }
    static uiWord sView()		{ return tr("View"); }
    static uiWord sVintage()		{ return tr("Vintage"); }
    static uiWord sVisualization()	{ return tr("Visualization"); }
    static uiWord sVolume(int n=1)	{ return tr("Volume",0,n); }
    static uiWord sWarning()		{ return tr("Warning"); }
    static uiWord sWaveNumber(int num=1){ return tr("Wavenumber", 0, num ); }
    static uiWord sWavelet(int n=1)	{ return tr("Wavelet",0,n); }
    static uiWord sWeb()		{ return tr("Web"); }
    static uiWord sWeight()		{ return tr("Weight"); }
    static uiWord sWell(int n=1)	{ return tr("Well",0,n); }
    static uiWord sWellData(int n=1)	{ return tr("Well Data",0,n); }
    static uiWord sWellLog(int n=1)	{ return tr("Well Log",0,n); }
    static uiWord sWellName()		{ return tr("Well Name"); }
    static uiWord sWellTrack(int n=1)	{ return tr("Well Track",0,n); }
    static uiWord sWellMarker(int n=1)	{ return tr("Well Marker",0,n); }
    static uiWord sWells()		{ return sWell( mPlural ); }
    static uiWord sWellsHandled()	{ return tr("Wells handled"); }
    static uiWord sWest( bool abbr )
    { return abbr ? tr("W","abbr West") : tr("West"); }
    static uiWord sWidth()		{ return tr("Width"); }
    static uiWord sWiggle()		{ return tr("Wiggle"); }
    static uiWord sWindow()		{ return tr("Window"); }
    static uiWord sWizBack()		{ return toUiString("<< %1")
							.arg(sBack()); }
    static uiWord sWizNext()		{ return toUiString("%1 >>")
							.arg(sNext()); }
    static uiWord sWorkflow(int n=1)	{ return tr("Workflow",0,n); }
    static uiWord sWrite()		{ return tr("Write"); }
    static uiWord sWriting()		{ return tr("Writing"); }
    static uiWord sWVA(bool abbr=false)
    { return abbr ? tr("WVA","abbr Wiggle/Var Area") : tr("Wiggle/VarArea"); }
    static uiWord sX()			{ return tr("X"); }
    static uiWord sXcoordinate()	{ return tr("X-coordinate"); }
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


//! Adds '...' to string, usable for menu items
#define m3Dots( txt ) \
    uiStrings::phrThreeDots( txt, false )

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
