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

#include "uibasemod.h"
#include "fixedstring.h"
#include "uistring.h"

//Common strings. Use these and extend when needed

mExpClass(uiBase) uiStrings
{ mODTextTranslationClass(uiStrings);
public:
    static uiString s2D()		{ return tr("&2D"); }
    static uiString s3D()		{ return tr("&3D"); }
    static uiString s2DHorizons()	{ return tr("2D &Horizons"); }
    static uiString s3DHorizons()	{ return tr("3D &Horizons"); }
    static uiString s2DSeismics()	{ return tr("2D &Seismics"); }
    static uiString s3DSeismics()	{ return tr("3D &Seismics"); }
    static uiString s2DViewer()		{ return tr("2D &Viewer"); }
    static uiString sAbort()		{ return tr("&Abort"); }
    static uiString sAccept()           { return tr("&Accept"); }
    static uiString sAction()		{ return tr("&Action"); }
    static uiString sAdd(bool immediate);
    static uiString sAdvanced()		{ return tr("&Advanced"); }
    static uiString sAll()		{ return tr("&All"); }
    static uiString sAmplitude()	{ return tr("Show &Amplitude"
                                                    " Spectrum"); }
    static uiString sAnalysis()		{ return tr("&Analysis"); }
    static uiString sApply()		{ return tr("&Apply"); }
    static uiString sASCII()		{ return tr("&ASCII"); }
    static uiString sASCII2D()	        { return tr("&ASCII2D"); }
    static uiString sASCII3D()	        { return tr("&ASCII3D"); }
    static uiString sAttribute()	{ return tr("&Attribute"); }
    static uiString sAttributes()	{ return tr("&Attributes"); }
    static uiString sAttributes2D()	{ return tr("&Attributes 2D"); }
    static uiString sAttributes3D()	{ return tr("&Attributes 3D"); }
    static uiString sAuto()		{ return tr("&Auto"); }
    static uiString sBack()		{ return tr("&< Back"); }
    static uiString sBase()		{ return tr("&Base"); }
    static uiString sBodies()		{ return tr("&Bodies"); }
    static uiString sBottom()		{ return tr("&Bottom"); }
    static uiString sBulk()	        { return tr("&Bulk"); }
    static uiString sBulk3D()	        { return tr("&Bulk3D"); }
    static uiString sCalculate()	{ return tr("&Calculate"); }
    static uiString sCancel()		{ return tr("&Cancel"); }
    static uiString sCascade()		{ return tr("&Cascade"); }
    static uiString sCBVS()	        { return tr("&CBVS"); }
    static uiString sChange()           { return tr("&Change"); }
    static uiString sClose()		{ return tr("&Close"); }
    static uiString sContinue()		{ return tr("&Continue"); }
    static uiString sColor()		{ return tr("&Color"); }
    static uiString sColorTable()	{ return tr("&Color Table"); }
    static uiString sComponent()        { return tr("&Component"); }
    static uiString sCopy()		{ return tr("&Copy"); }
    static uiString sCreate(bool immediate);
    static uiString sCreateMap()	{ return tr("&Create Map"); }
    static uiString sCrossline()        { return tr("&Crossline"); }
    static uiString sCrossplot()	{ return tr("&Cross-plot data"); }
    static uiString sDefault()		{ return tr("&Default"); }
    static uiString sDensity()		{ return tr("Probability &Density"
                                                    " Functions"); }
    static uiString sDepth()	        { return tr("&Depth"); }
    static uiString sDisplay()		{ return tr("&Display"); }
    static uiString sDoesNotExist();
    static uiString sDown()		{ return tr("&Down"); }
    static uiString sDummy()		{ return tr("Dummy"); }
    static uiString sDuplicate()	{ return tr("&Duplicate"); }
    static uiString sEdit(bool immediate);
    static uiString sEmptyString()	{ return uiString(""); }
    static uiString sError()		{ return tr("Error"); }
    static uiString sEvaluate()		{ return tr("&Evaluate"); }
    static uiString sEvent()		{ return tr("&Event"); }
    static uiString sExit()		{ return tr("&Exit"); }
    static uiString sExport()		{ return tr("&Export"); }
    static uiString sFaults()		{ return tr("F&aults"); }
    static uiString sFaultSticks()	{ return tr("F&aulStickSets"); }
    static uiString sFile()	        { return tr("&File"); }
    static uiString sFinish()	        { return tr("&Finish"); }
    static uiString sFilter()	        { return tr("&Filter"); }
    static uiString sFKSpectrum()	{ return tr("FK &Spectrum"); }
    static uiString sGeometry2D()	{ return tr("&Geometry2D"); }
    static uiString sGeometry3D()	{ return tr("&Geometry3D"); }
    static uiString sGo()	        { return tr("&Go"); }
    static uiString sHelp()		{ return tr("&Help"); }
    static uiString sHide()		{ return tr("&Hide"); }
    static uiString sHistogram()        { return tr("&Histogram"); }
    static uiString sHorizon()		{ return tr("&Horizon"); }
    static uiString sHorizons()		{ return tr("&Horizons"); }
    static uiString sHorizontal()	{ return tr("&Horizontal"); }
    static uiString sIcons()		{ return tr("&Icons"); }
    static uiString sImport()		{ return tr("&Import"); }
    static uiString sInfo()		{ return tr("&Info"); }
    static uiString sInline()		{ return tr("&Inline"); }
    static uiString sInput()		{ return tr("&Input"); }
    static uiString sInside()		{ return tr("&Inside"); }
    static uiString sInstallation()	{ return tr("&Installation"); }
    static uiString sLayer()		{ return tr("&Layer Modeling"); }
    static uiString sLanguage()		{ return tr("&Language"); }
    static uiString sLoad()		{ return tr("&Load ..."); }
    static uiString sLock()		{ return tr("&Lock"); }
    static uiString sLogs()	        { return tr("&Logs"); }
    static uiString sMarkers()	        { return tr("&Markers"); }
    static uiString sManage()		{ return tr("&Manage"); }
    static uiString sManual()		{ return tr("&Manual"); }
    static uiString sMenu()		{ return tr("&Menu"); }
    static uiString sMode()		{ return tr("&Mode"); }
    static uiString sMove()		{ return tr("&Move"); }
    static uiString sMultiwell()	{ return tr("&Multi-well"); }
    static uiString sMute()		{ return tr("&Mute Functions"); }
    static uiString sName()		{ return tr("&Name"); }
    static uiString sNew(bool immediate);
    static uiString sNext()		{ return tr("Next &>"); }
    static uiString sNo()		{ return tr("&No"); }
    static uiString sOff()		{ return tr("&Off"); }
    static uiString sOk()		{ return tr("&OK"); }
    static uiString sOn()		{ return tr("&On"); }
    static uiString sOpen(bool immediate);
    static uiString sOperator()		{ return tr("&Operator"); }
    static uiString sOtherSurvey()	{ return tr("&Other Survey"); }
    static uiString sOutput()           { return tr("&Output"); }
    static uiString sOutside()		{ return tr("&Outside"); }
    static uiString sOverwrite()        { return tr("&Overwrite"); }
    static uiString sPause()            { return tr("&Pause"); }
    static uiString sPickSet()		{ return tr("&Pickset"); }
    static uiString sPickSets()		{ return tr("&PickSets/Polygons"); }
    static uiString sPolygon()		{ return tr("&Polygon"); }
    static uiString sPreload()		{ return tr("&Pre-load"); }
    static uiString sPrestack()	        { return tr("Pre&stack"); }
    static uiString sPrestack2D()	{ return tr("Pre&stack2D"); }
    static uiString sPrestack3D()	{ return tr("Pre&stack3D"); }
    static uiString sProperties(bool immediate);
    static uiString sProcessing()	{ return tr("&Processing"); }
    static uiString sRedo()		{ return tr("&Redo"); }
    static uiString sReload()		{ return tr("&Reload"); }
    static uiString sRemove(bool immediate);
    static uiString sReplace()		{ return tr("&Replace"); }
    static uiString sRectangle()	{ return tr("&Rectangle"); }
    static uiString sReset()		{ return tr("Re&set"); }
    static uiString sRestore()		{ return tr("Re&store"); }
    static uiString sResume()		{ return tr("Re&sume"); }
    static uiString sRokDoc()		{ return tr("&Rok Doc"); }
    static uiString sRun()		{ return tr("&Run"); }
    static uiString sSave(bool immediate);
    static uiString sSaveAs()		{ return tr("Save &as ..."); }
    static uiString sSaveAsDefault();
    static uiString sScanned()		{ return tr("&Scanned"); }
    static uiString sScenes()		{ return tr("S&cenes"); }
    static uiString sSEGY()		{ return tr("SEG &Y"); }
    static uiString sSeismics()		{ return tr("&Seismics"); }
    static uiString sSelect(bool arg=false,bool plural=false);
    static uiString sSession()		{ return tr("&Session"); }
    static uiString sSettings()		{ return tr("&Settings"); }
    static uiString sSetup()		{ return tr("&Setup"); }
    static uiString sShift()		{ return tr("&Shift"); }
    static uiString sShow()             { return tr("&Show"); }
    static uiString sSimilarity()	{ return tr("&Similarity"); }
    static uiString sSimple()		{ return tr("&Simple file"); }
    static uiString sSlice()		{ return tr("&Slice"); }
    static uiString sStart()		{ return tr("St&art"); }
    static uiString sSteering()		{ return tr("St&eering"); }
    static uiString sSteps()		{ return tr("&Steps"); }
    static uiString sStop()		{ return tr("St&op"); }
    static uiString sStored()		{ return tr("St&ored"); }
    static uiString sStratigraphy()	{ return tr("St&ratigraphy"); }
    static uiString sSurvey()		{ return tr("Survey"); }
    static uiString sTile()		{ return tr("&Tile"); }
    static uiString sTime()		{ return tr("&Time"); }
    static uiString sToBottom()		{ return tr("&To bottom"); }
    static uiString sToTop()		{ return tr("&To top"); }
    static uiString sTools()		{ return tr("&Tools"); }
    static uiString sTop()		{ return tr("&Top"); }
    static uiString sTrack()	        { return tr("&Track"); }
    static uiString sTransparency()     { return tr("&Transparency"); }
    static uiString sType()             { return tr("&Type"); }
    static uiString sUndo()		{ return tr("&Undo"); }
    static uiString sUnlock()           { return tr("&Unlock"); }
    static uiString sUp()		{ return tr("&Up"); }
    static uiString sUse()		{ return tr("&Cancel"); }
    static uiString sUtilities()	{ return tr("&Utilities"); }
    static uiString sValue()		{ return tr("&Value"); }
    static uiString sVelocity()		{ return tr("&Velocity Functions"); }
    static uiString sVertical()		{ return tr("&Vertical"); }
    static uiString sView()		{ return tr("View"); }
    static uiString sViewI()		{ return tr("View &In-line"); }
    static uiString sViewLog()		{ return tr("View &Log"); }
    static uiString sViewMap()		{ return tr("View &Map"); }
    static uiString sViewN()		{ return tr("View &North"); }
    static uiString sViewNZ()		{ return tr("View &North Z"); }
    static uiString sViewX()		{ return tr("View &Cross-line"); }
    static uiString sViewZ()		{ return tr("View &Z"); }
    static uiString sVolume()		{ return tr("Volume Builder"); }
    static uiString sVSP()	        { return tr("&VSP"); }
    static uiString sWavelets()		{ return tr("&Wavelets"); }
    static uiString sWells()		{ return tr("&Wells"); }
    static uiString sWiggle()		{ return tr("&Wiggle"); }
    static uiString sYes()		{ return tr("&Yes"); }
};

/*Old strings, move to uiStrings class and replace globally.
  DONT USE IN NEW CODE!
*/

inline FixedString sApply()			{ return "&Apply"; }
inline FixedString sCancel()			{ return "&Cancel"; }
inline FixedString sClose()			{ return "&Close"; }
inline FixedString sHelp()			{ return "&Help"; }
inline FixedString sLoad()			{ return "&Load ..."; }
inline FixedString sNew()			{ return "&New"; }
inline FixedString sNo()			{ return "&No"; }
inline FixedString sOk()			{ return "&OK"; }
inline FixedString sOpen(bool immediate=false);
inline FixedString sRun()			{ return "&Run"; }
inline FixedString sSave(bool immediate=true);
inline FixedString sSaveAs()			{ return "Save &as ..."; }
inline FixedString sSaveAsDefault();
inline FixedString sYes()			{ return "&Yes"; }

//Implementations
inline FixedString sOpen(bool immediate)
{ return immediate ? "&Open" : "&Open ..."; }


inline FixedString sSave(bool immediate)
{ return immediate ? "&Save" : "&Save ..."; }


inline FixedString sSaveAsDefault()
{ return "Save as &default"; }


inline uiString uiStrings::sAdd( bool immediate )
{
    return immediate ? tr("&Add") : tr("&Add ...");
}


inline uiString uiStrings::sCreate( bool immediate )
{
    return immediate ? tr("&Create") : tr("&Create ...");
}

inline uiString uiStrings::sDoesNotExist()
{ return tr( "%1 does not exist."); }


inline uiString uiStrings::sEdit( bool immediate )
{
    return immediate ? tr("&Edit") : tr("&Edit ...");
}


inline uiString uiStrings::sNew( bool immediate )
{
    return immediate ? tr("&New") : tr("&New ...");
}


inline uiString uiStrings::sOpen(bool immediate)
{ return immediate ? "&Open" : "&Open ..."; }


inline uiString uiStrings::sProperties(bool immediate)
{ return immediate ? tr("&Properties") : tr("&Properties ..."); }


inline uiString uiStrings::sRemove(bool immediate)
{ return immediate ? tr("&Remove") : tr("&Remove ..."); }


inline uiString uiStrings::sSave(bool immediate)
{ return immediate ? "&Save" : "&Save ..."; }


inline uiString uiStrings::sSelect(bool arg,bool plural)
{
    if ( !arg )
	return tr("&Select");

    return plural ? tr( "Select %1(s)" ) : tr( "Select %1" );
}



inline uiString uiStrings::sSaveAsDefault()
{ return "Save as &default"; }


#endif

