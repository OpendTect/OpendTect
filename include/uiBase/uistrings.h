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
    static inline uiString s2D()		{ return tr("&2D"); }
    static inline uiString s3D()		{ return tr("&3D"); }
    static inline uiString s2dViewer()		{ return tr("2d &Viewer"); }
    static inline uiString sAbort()		{ return tr("&Abort"); }
    static inline uiString sAdd(bool immediate);
    static inline uiString sAccept()            { return tr("&Accept"); }
    static inline uiString sAction()		{ return tr("&Action"); }
    static inline uiString sAmplitude()	        { return tr("&Amplitude"); }
    static inline uiString sAnalysis()		{ return tr("&Analysis"); }
    static inline uiString sApply()		{ return tr("&Apply"); }
    static inline uiString sASCII()		{ return tr("&ASCII"); }
    static inline uiString sASCII2D()	        { return tr("&ASCII2D"); }
    static inline uiString sASCII3D()	        { return tr("&ASCII3D"); }
    static inline uiString sAttribute()		{ return tr("&Attribute"); }
    static inline uiString sAttributes3D()	{ return tr("&Attributes3D"); }
    static inline uiString sAutoload()		{ return tr("&Auto-load"); }
    static inline uiString sBack()		{ return tr("&< Back"); }
    static inline uiString sBase()		{ return tr("&Base"); }
    static inline uiString sBodies()		{ return tr("&Bodies"); }
    static inline uiString sBulk()	        { return tr("&Bulk"); }
    static inline uiString sBulk3D()	        { return tr("&Bulk3D"); }
    static inline uiString sCancel()		{ return tr("&Cancel"); }
    static inline uiString sCBVS()    	        { return tr("&CBVS"); }
    static inline uiString sChange()            { return tr("&Change"); }
    static inline uiString sClose()		{ return tr("&Close"); }
    static inline uiString sCreateMap()		{ return tr("&Create Map"); }
    static inline uiString sContinue()		{ return tr("&Continue"); }
    static inline uiString sColorTable()	{ return tr("&Color Table"); }
    static inline uiString sCopy()		{ return tr("&Copy"); }
    static inline uiString sCreate(bool immediate);
    static inline uiString sDoesNotExist();
    static inline uiString sCrossplot()		{ return tr("&Cross-plot"); }
    static inline uiString sDensity()		{ return tr("&Density"); }
    static inline uiString sDepth()	        { return tr("&Depth"); }
    static inline uiString sDisplay()		{ return tr("&Display"); }
    static inline uiString sDuplicate()		{ return tr("&Duplicate"); }
    static inline uiString sDown()		{ return tr("&Down"); }
    static inline uiString sEdit(bool immediate);
    static inline uiString sEmptyString()	{ return uiString(""); }
    static inline uiString sError()		{ return tr("Error"); }
    static inline uiString sExit()		{ return tr("&Exit"); }
    static inline uiString sExport()		{ return tr("&Export"); }
    static inline uiString sFaults()		{ return tr("F&aults"); }
    static inline uiString sFaultSticks()	{ return tr("F&ault Sticks"); }
    static inline uiString sFile()	        { return tr("&File"); }
    static inline uiString sFKSpectrum()	{ return tr("FK &Spectrum"); }
    static inline uiString sGeometry2D()	{ return tr("&Geometry2D"); }
    static inline uiString sGeometry3D()	{ return tr("&Geometry3D"); }
    static inline uiString sHelp()		{ return tr("&Help"); }
    static inline uiString sHide()		{ return tr("&Hide"); }
    static inline uiString sHistogram()         { return tr("&Histogram"); }
    static inline uiString sHorizon()		{ return tr("&Horizons"); }
    static inline uiString sImport()		{ return tr("&Import"); }
    static inline uiString sInside()		{ return tr("&Inside"); }
    static inline uiString sLayers()		{ return tr("&Layers"); }
    static inline uiString sLoad()		{ return tr("&Load ..."); }
    static inline uiString sLock()		{ return tr("&Lock"); }
    static inline uiString sLogs()	        { return tr("&Logs"); }
    static inline uiString sMarkers()	        { return tr("&Markers"); }
    static inline uiString sManage()		{ return tr("&Manage"); }
    static inline uiString sMove()		{ return tr("&Move"); }
    static inline uiString sMultiwell()	        { return tr("&Multi-well"); }
    static inline uiString sMute()		{ return tr("&Mute"); }
    static inline uiString sName()		{ return tr("&Name"); }
    static inline uiString sNew(bool immediate);
    static inline uiString sNext()		{ return tr("Next &>"); }
    static inline uiString sNo()		{ return tr("&No"); }
    static inline uiString sOk()		{ return tr("&OK"); }
    static inline uiString sOpen(bool immediate);
    static inline uiString sOtherSurvey()	{ return tr("&Other Survey"); }
    static inline uiString sOutput()            { return tr("&Output"); }
    static inline uiString sOutside()		{ return tr("&Outside"); }
    static inline uiString sOverwrite()         { return tr("&Overwrite"); }
    static inline uiString sPause()             { return tr("&Pause"); }
    static inline uiString sPickSets()		{ return tr("&Pick Sets"); }
    static inline uiString sPreload()		{ return tr("&Pre-load"); }
    static inline uiString sPrestack()	        { return tr("Pre&stack"); }
    static inline uiString sPrestack2D()	{ return tr("Pre&stack2D"); }
    static inline uiString sPrestack3D()	{ return tr("Pre&stack3D"); }
    static inline uiString sProperties(bool immediate);
    static inline uiString sProcessing()	{ return tr("&Processing"); }
    static inline uiString sReload()		{ return tr("&Reload"); }
    static inline uiString sRemove(bool immediate);
    static inline uiString sReplace()		{ return tr("&Replace"); }
    static inline uiString sReset()		{ return tr("Re&set"); }
    static inline uiString sRestore()		{ return tr("Re&store"); }
    static inline uiString sRokDoc()		{ return tr("&Rok Doc"); }
    static inline uiString sRun()		{ return tr("&Run"); }
    static inline uiString sSave(bool immediate);
    static inline uiString sSaveAs()		{ return tr("Save &as ..."); }
    static inline uiString sSaveAsDefault();
    static inline uiString sScanned()		{ return tr("&Scanned"); }
    static inline uiString sScenes()		{ return tr("S&cenes"); }
    static inline uiString sSEGY()		{ return tr("SEG &Y"); }
    static inline uiString sSeismics()		{ return tr("&Seismics"); }
    static inline uiString sSelect(bool arg=false,bool plural=false); 
    static inline uiString sSession()		{ return tr("&Session"); }
    static inline uiString sSettings()		{ return tr("&Settings"); }
    static inline uiString sSetup()		{ return tr("&Setup"); }
    static inline uiString sSimple()		{ return tr("&Simple"); }
    static inline uiString sStart()		{ return tr("St&art"); }
    static inline uiString sStop()		{ return tr("St&op"); }
    static inline uiString sStratigraphy()	{ return tr("St&ratigraphy"); }
    static inline uiString sSurvey()		{ return tr("&Survey"); }
    static inline uiString sToBottom()		{ return tr("To &Bottom"); }
    static inline uiString sToTop()		{ return tr("To &Top"); }
    static inline uiString sTrack()	        { return tr("&Track"); }
    static inline uiString sTransparency()      { return tr("&Transparency"); }
    static inline uiString sType()              { return tr("&Type"); }
    static inline uiString sUp()		{ return tr("&Up"); }
    static inline uiString sUtilities()		{ return tr("&Utilities"); }
    static inline uiString sValue()		{ return tr("&Value"); }
    static inline uiString sVelocity()		{ return tr("&Velocity"); }
    static inline uiString sView()		{ return tr("&View"); }
    static inline uiString sViewMap()		{ return tr("&View Map"); }
    static inline uiString sViewLog()		{ return tr("&View Log"); }
    static inline uiString sVolume()		{ return tr("&Volume"); }
    static inline uiString sVSP()	        { return tr("&VSP"); }
    static inline uiString sWavelets()		{ return tr("&Wavelets"); }
    static inline uiString sWells()		{ return tr("&Wells"); }
    static inline uiString sYes()		{ return tr("&Yes"); }
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

