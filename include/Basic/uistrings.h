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

//Common strings. Use these and extend when needed

mExpClass(Basic) uiStrings
{ mODTextTranslationClass(uiStrings);
public:
//Phrases
    static uiString phrAdd(const uiString&);
    //!<"Add <string>"
    static uiString phrASCII(const uiString& string);
    //!<"ASCII <string>"
    static uiString phrCalculateFrom(const uiString& string);
    //!<"Calculate from <string>" 
    static uiString phrCannotCreate(const uiString& string);
    //!<"Cannot create <string>"
    static uiString phrCannotCreateDBEntryFor(const uiString& string);
    //!<"Cannot create database entry for <string>"
    static uiString phrCannotCreateDirectory(const uiString& string);
    //!<"Cannot create directory <string>"
    static uiString phrCannotFind(const uiString& string);
    //!<"Cannot find <string>"
    static uiString phrCannotFindDBEntry(const uiString&);
    //!<"Cannot find database entry for
    static uiString phrCannotImport(const uiString&);
    //!"Cannot Import <string>"
    static uiString phrCannotOpen(const uiString& string);
    //!<"Cannot open <string>"
    static uiString phrCannotRead(const uiString& string);
    //!<"Cannot read <string>"
    static uiString phrCannotSave(const uiString&);
    //!<"Cannot Save <string>"
    static uiString phrCannotRemove(const uiString& string);
    //!<"Cannot remove <string>"
    static uiString phrCannotWrite(const uiString& string);
    //!<"Cannot write <string>"
    static uiString phrCannotWriteDBEntry(const uiString&);
    static uiString phrCrossline(const uiString&);
    //!<"Cross-line <string>
    static uiString phrCopy(const uiString&);
    //!<"Copy <string>"
    static uiString phrCreate(const uiString& string);
    //!<"Create <string>"
    static uiString phrDoesntExist(const uiString& string,int num=1);
    //!<"<string> does/do not exist"
    static uiString phrEdit(const uiString& string);
    //!<"Edit <string>"
    static uiString phrExistsConinue(const uiString&,bool overwrite);
    //!<"<string> exists. Continue?" or "<string> exists. Overwrite?
    static uiString phrExport(const uiString& string);
    //!<"Export <string>"
    static uiString phrImport(const uiString& string);
    //!<"Import <string>"
    static uiString phrInline(const uiString&);
    //!<"In-line <string>
    static uiString phrInput(const uiString&);
    //!<"Input <string>"
    static uiString phrInvalid(const uiString& string);
    //!<"Invalid <string>"
    static uiString phrJoinStrings(const uiString& a,const uiString& b);
    //!<"<a> <b>
    static uiString phrLoad(const uiString&);
    //!"Load <string>"
    static uiString phrMerge(const uiString&);
    //!"Merge <string>"
    static uiString phrModify(const uiString&);
    //!"Modify <string>"
    static uiString phrOutput(const uiString&);
    //!<"Output <string>"
    static uiString phrReading(const uiString&);
    //!<"Reading <string>"
    static uiString phrSave(const uiString&);
    //!<"Save <string>"
    static uiString phrSelect(const uiString& string);
    //!<"Select <string>"
    static uiString phrSuccessfullyExported(const uiString&);
    //!<"Successfully exported <string>"
    static uiString phrThreeDots(const uiString& string,bool immediate=false);
    //!<string> ...
    static uiString phrWriting(const uiString&);
    //!<"Writing <string>"
    static uiString phrZIn(const uiString&);
    //!"Z in <string>"
    static uiString phrZRange(const uiString&);
    //!"Z Range <string>"
    

//Words	 
    static uiString s2D();
    static uiString s3D();
    static uiString sAbort()		{ return tr("Abort"); }
    static uiString sAction()		{ return tr("Action"); }
    static uiString sAdd();
    static uiString sAddColBlend()	{ return tr("Add Color Blended"); }
    static uiString sAdvanced()		{ return tr("Advanced"); }
    static uiString sAll()		{ return tr("All"); }
    static uiString sAmplitude()	{ return tr("Amplitude"); }
    static uiString sAnalysis()		{ return tr("Analysis"); }
    static uiString sApply()		{ return tr("Apply"); }
    static uiString sASCII();
    static uiString sAttribName()	{ return tr("Attribute Name"); }
    static uiString sAttribute()	{ return tr("Attribute"); }
    static uiString sAttributes();
    static uiString sAzimuth()		{ return tr("Azimuth"); }
    static uiString sBody(int num=1)	{ return tr("Body", 0, num); }
    static uiString sBottom()		{ return tr("Bottom"); }
    static uiString sBottomHor()	{ return tr("Bottom Horizon"); }
    static uiString sCalculate()	{ return tr("Calculate"); }
    static uiString sCalculateFrom();
    static uiString sCancel()		{ return tr("Cancel"); }
    static uiString sCantCreateHor();
    static uiString sCantFindAttrName();
    static uiString sCantFindODB();
    static uiString sCantFindSurf();
    static uiString sCannotImport();
    static uiString sCantReadHor();
    static uiString sCantReadInp();
    static uiString sCantWriteSettings();
    static uiString sCantOpenInpFile(int num=1);
    static uiString sCantOpenOutpFile(int num=1);
    static uiString sCannotSave();
    static uiString sClear()		{ return tr("Clear"); }
    static uiString sClose()		{ return tr("Close"); }
    static uiString sColor()		{ return tr("Color"); }
    static uiString sColorTable();
    static uiString sComponent()	{ return tr("Component"); }
    static uiString sContinue()		{ return tr("Continue"); }
    static uiString sCopy();		
    static uiString sCreateGroup()	{ return tr("Create Group"); }
    static uiString sCreate();
    static uiString sCreateProbDesFunc();
    static uiString sCrossline(int num=1) { return tr("Cross-line", 0, num ); }
    static uiString sData()		{ return tr("Data"); }
    static uiString sDelete()		{ return tr("Delete"); }
    static uiString sDepth()	        { return tr("Depth"); }
    static uiString sDip()		{ return tr("Dip"); }
    static uiString sDisplay()		{ return tr("Display"); }
    static uiString sDone()		{ return tr("Done"); }

    static uiString sDown()		{ return tr("Down"); }
    static uiString sEdit();
    static uiString sEmptyString()	{ return tr(""); }
    static uiString sEnterValidName();
    static uiString sExamine()		{ return tr("Examine"); }
    static uiString sExitOD()		{ return tr("Exit OpendTect"); }
    static uiString sExit()		{ return tr("Exit"); }
    static uiString sExport();
    static uiString sFaultStickSet(int num=1);
    static uiString sFeet()		{ return tr("Feet"); }
    static uiString sFault(int num=1);
    static uiString sFile()	        { return tr("File"); }
    static uiString sFileDoesntExist()	{ return phrDoesntExist(sFile(),1); }
    static uiString sFilter()		{ return tr("Filter"); }
    static uiString sFrequency(bool smallcase=false, bool plural=false);
    static uiString sGo()	        { return tr("Go"); }
    static uiString sHelp();
    static uiString sHide()		{ return tr("Hide"); }
    static uiString sHistogram();
    static uiString sHorizon(int num=1);
    static uiString sHorizontal()	{ return tr("Horizontal"); }
    static uiString sImport();		
    static uiString sImpSuccess()	{ return tr("Import successful"); }
    static uiString sInfo()		{ return tr("info"); }
    static uiString sInline(int num=1)	{ return tr("In-line",0,num); }
    static uiString sInputParamsMissing();
    static uiString sInput();
    static uiString sInputSelection()	{ return phrInput( sSelection(true) ); }
    static uiString sInputASCIIFile();
    static uiString sInputData()	{ return tr("Input Data"); }
    static uiString sInvalid();
    static uiString sInvInpFile()	{ return tr("Invalid input file"); }
    static uiString sLoad();
    static uiString sLock()		{ return tr("Lock"); }
    static uiString sLogs();
    static uiString sLogFile()		{ return tr("Log File"); }
    static uiString sManual()		{ return tr("Manual"); }
    static uiString sManWav()		{ return tr("Manage Wavelets"); }
    static uiString sMarker(int num=1);
    static uiString sMeter()		{ return tr("Meter"); }
    static uiString sMerge();
    static uiString sModify();
    static uiString sMsec()		{ return tr("msec"); }
    static uiString sMute(int num=1)	{ return tr("Mute",0,num); }
    static uiString sName()		{ return tr("Name"); }
    static uiString sNew();
    static uiString sNext()		{ return tr("Next >"); }
    static uiString sNo()		{ return tr("No"); }
    static uiString sNoLogSel()		{ return tr("No log selected"); }
    static uiString sNone()		{ return tr("None"); }
    static uiString sNormal()		{ return tr("Normal"); }
    static uiString sNoValidData()	{ return tr("No valid data found"); }
    static uiString sOffset()		{ return tr("Offset"); }
    static uiString sOk()		{ return tr("OK"); }
    static uiString sOpen();
    static uiString sOperator()		{ return tr("Operator"); }
    static uiString sOptions();
    static uiString sOutpDataStore()	{ return tr("Output data store"); }
    static uiString sOutputFile()	{ return tr("Output file"); }
    static uiString sOutputStatistic()	{ return phrOutput( tr("statistic") ); }
    static uiString sOutputFileExistsOverwrite();
    static uiString sOutput();
    static uiString sOutputSelection()	{ return phrOutput(sSelection(true)); }
    static uiString sOutputASCIIFile();
    static uiString sOverwrite()        { return tr("Overwrite"); }
    static uiString sPause()            { return tr("Pause"); }
    static uiString sPickSet()		{ return tr("PickSet"); }
    static uiString sPolygon()		{ return tr("Polygon"); }
    static uiString sPreStackEvents()	{ return tr("Prestack Events"); }
    static uiString sProcessing()	{ return tr("Processing"); }
    static uiString sProbDensFunc(bool abbrevation=false);
    static uiString sProperties();
    static uiString sRange(int num=1)	{ return tr("Range",0,1); }
    static uiString sRandomLine(int num=1) { return tr("Random Line",0,num); }
    static uiString sRectangle()	{ return tr("Rectangle"); }
    static uiString sRedo()		{ return tr("Redo"); }
    static uiString sReload()		{ return tr("Reload"); }
    static uiString sRename()		{ return tr("Rename"); }
    static uiString sReset()		{ return tr("Reset"); }
    static uiString sResume()		{ return tr("Resume"); }
    static uiString sRightClick()	{ return tr("<right-click>"); }
    static uiString sRemove();
    static uiString sReversed()		{ return tr("Reversed"); }
    static uiString sSave();
    static uiString sSaveAs();
    static uiString sSaveAsDefault()    { return tr("Save as Default"); }
    static uiString sSaveBodyFail()	{ return tr("Save body failed"); }
    static uiString sScanning()		{ return tr("Scanning"); }
    static uiString sScene(int num=1)	{ return tr("Scene",0,1); }
    static uiString sScenes()		{ return sScene(mPlural); }
    static uiString sSec()		{ return tr("sec"); }
    static uiString sSEGY()		{ return tr("SEG-Y"); }
    static uiString sSeismic(int num);
    static uiString sSeismics()		{ return sSeismic(mPlural); }
    static uiString sSeismics(bool is2d,bool isps,int num);
    static uiString sSelAttrib()	{ return tr("Select Attribute"); }
    static uiString sSelection(bool smallletters);
    static uiString sSelect();
    static uiString sSelOutpFile();
    static uiString sSetting(int num=1);
    static uiString sSettings()		{ return sSetting(mPlural); }
    static uiString sSetup()		{ return tr("Setup"); }
    static uiString sShift();
    static uiString sShow()             { return tr("Show"); }
    static uiString sSpecify()		{ return tr("Please specify"); }
    static uiString sSteering()		{ return tr("Steering"); }
    static uiString sStep()		{ return tr("Step"); }
    static uiString sStepout()		{ return tr("Stepout"); }
    static uiString sStop()		{ return tr("Stop"); }
    static uiString sStored();
    static uiString sStratigraphy();
    static uiString sSurface()		{ return tr("Surface"); }
    static uiString sSurvey()		{ return tr("Survey"); }
    static uiString sTakeSnapshot()	{ return tr("Take Snapshot"); }
    static uiString sStart()		{ return tr("Start"); }
    static uiString sTile()		{ return tr("Tile"); }
    static uiString sTime()		{ return tr("Time"); }
    static uiString sToolbar()		{ return tr("Toolbar"); }
    static uiString sTools()		{ return tr("Tools"); }
    static uiString sTopHor()		{ return tr("Top Horizon"); }
    static uiString sTrace(int num=1)	{ return tr("Trace", 0, num); }
    static uiString sTrack();
    static uiString sTransparency()     { return tr("Transparency"); }
    static uiString sType()             { return tr("Type"); }
    static uiString sUndo()		{ return tr("Undo"); }
    static uiString sUp()		{ return tr("Up"); }
    static uiString sUse()		{ return tr("Use"); }
    static uiString sUtilities()	{ return tr("Utilities"); }
    static uiString sValue()		{ return tr("Value"); }
    static uiString sVelocity()		{ return tr("Velocity"); }
    static uiString sVertical()		{ return tr("Vertical"); }
    static uiString sVolume();
    static uiString sView()		{ return tr("View"); }
    static uiString sWarning()		{ return tr("Warning"); }
    static uiString sWavelet(int num=1);
    static uiString sWaveNumber(bool smallcase=false, bool plural=false);
    static uiString sWell(int num=1);
    static uiString sWells()		{ return sWell(mPlural); }
    static uiString sWellLog(int num=1);
    static uiString sWiggle()		{ return tr("Wiggle"); }

    static uiString sWrite()		{ return tr("Write"); }

    static uiString sWriting()		{ return tr("Writing"); }
    static uiString sYes()		{ return tr("Yes"); }
    static uiString sSet()		{ return tr("Set"); }
    static uiString sZUnit()		{ return tr("Z-unit"); }
    static uiString sZSlice(int num=1)	{ return tr("Z-slice",0,num); }
    static uiString sZValue(int num=1)	{ return tr("Z value",0,num); }
    static uiString sZRange();

    static uiString sDistUnitString(bool isfeet,bool abbrevated,
				    bool withparentheses);
    /*!< returns "m", "ft", "meter", or "feet" */
    static uiString sVolDataName(bool is2d,bool is3d,bool isprestack,
				     bool both_2d_3d_in_context=false,
				     bool both_pre_post_in_context=false);
    /*!<Returns names for data volumes such as "2D Data", "Cube",
	"Pre-stack Data", and similar */
};


#define m3Dots( txt ) uiStrings::phrThreeDots( txt, false )
#define mJoinUiStrs( txt1, txt2 )\
   uiStrings::phrJoinStrings( uiStrings::txt1, uiStrings::txt2 )

#endif

