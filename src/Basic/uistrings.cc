/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Prajjaval
 Date:		2015
________________________________________________________________________

-*/

#include "uistrings.h"
#include "dbkey.h"

static const char* joinstring = "%1 %2";

uiPhrase uiStrings::phrAdd( const uiWord& string )
{ return toUiString(joinstring).arg( sAdd() ).arg( string ); }

uiPhrase uiStrings::phrASCII( const uiWord& string )
{ return toUiString(joinstring).arg( sASCII() ).arg( string ); }

uiPhrase uiStrings::phrCalculate( const uiWord& string )
{ return toUiString(joinstring).arg(sCalculate()).arg(string); }

uiPhrase uiStrings::phrCalculateFrom( const uiWord& string )
{ return toUiString(joinstring).arg(sCalculateFrom()).arg(string); }

uiPhrase uiStrings::phrCrossline( const uiWord& string )
{ return phrJoinStrings( sCrossline(), string ); }

uiPhrase uiStrings::phrExitOD()
{ return tr("Exit OpendTect"); }

uiPhrase uiStrings::phrTODONotImpl( const char* clssnm )
{ return toUiString( "[%1] TODO: Not Implemented" ).arg( clssnm ); }

uiPhrase uiStrings::phrNotImplInThisVersion( const char* fromver )
{ return tr("Not implemented in this version of OpendTect."
	  "\nPlease use version %1 or higher").arg( fromver ); }

uiPhrase uiStrings::phrThreeDots( const uiWord& string, bool immediate )
{ return immediate ? string : toUiString( "%1 ..." ).arg( string ); }

uiPhrase uiStrings::phrPlsSelectAtLeastOne( const uiWord& string )
{ return tr("Please select at least one %1").arg( string ); }

uiPhrase uiStrings::phrPlsSpecifyAtLeastOne( const uiWord& string )
{ return tr("Please specify at least one %1").arg( string ); }

uiPhrase uiStrings::phrSelect( const uiWord& string )
{ return toUiString(joinstring).arg( sSelect() ).arg( string ); }

uiPhrase uiStrings::phrSelectObjectWrongType( const uiWord& string )
{ return toUiString(joinstring).arg(tr("Select object is not a ")).arg(string);}

uiPhrase uiStrings::phrDoesntExist(const uiWord& string, int num )
{ return tr( "%1 does not exist", 0, num ).arg( string ); }

uiPhrase uiStrings::phrExport( const uiWord& string )
{ return toUiString(joinstring).arg( sExport() ).arg( string ); }

uiPhrase uiStrings::phrImport( const uiWord& string )
{ return toUiString(joinstring).arg( sImport() ).arg( string ); }

uiPhrase uiStrings::phrInternalError( const uiWord& string )
{ return tr( "Internal Error (pease contact support@dgbes.com):\n%1")
	 .arg( string ); }

uiPhrase uiStrings::phrInternalError( const char* string )
{ return tr( "Internal Error (pease contact support@dgbes.com):\n%1")
	 .arg( string ); }

uiPhrase uiStrings::phrCannotAdd( const uiWord& string )
{ return toUiString(joinstring).arg(sCannotAdd()).arg(string); }

uiPhrase uiStrings::phrCannotCompute( const uiWord& string )
{ return toUiString(joinstring).arg(sCannotCompute()).arg(string); }

uiPhrase uiStrings::phrCannotCopy( const uiWord& string )
{ return toUiString(joinstring).arg(sCannotCopy()).arg(string); }

uiPhrase uiStrings::phrCannotCreate( const uiWord& string )
{ return tr("Cannot create %1").arg( string ); }

uiPhrase uiStrings::phrCannotCreateDBEntryFor(const uiWord& string)
{ return phrCannotCreate( tr("database entry for %1").arg(string) ); }

uiPhrase uiStrings::phrCannotCreateDirectory( const uiWord& string )
{ return phrCannotCreate( tr("directory %1").arg(string) ); }

uiPhrase uiStrings::phrCannotEdit( const uiWord& string )
{ return toUiString(joinstring).arg(sCannotEdit()).arg(string); }

uiPhrase uiStrings::phrCannotExtract( const uiWord& string )
{ return toUiString(joinstring).arg(sCannotExtract()).arg(string); }

uiPhrase uiStrings::phrCannotFind( const uiWord& string )
{ return toUiString(joinstring).arg(sCannotFind()).arg(string); }

uiPhrase uiStrings::phrCannotImport( const uiWord& string )
{ return toUiString(joinstring).arg(sCannotImport()).arg(string); }

uiPhrase uiStrings::phrCannotLoad( const uiWord& string )
{ return toUiString(joinstring).arg(sCannotLoad()).arg(string); }

uiPhrase uiStrings::phrCannotOpen( const uiWord& string )
{ return toUiString(joinstring).arg(sCannotOpen()).arg( string ); }

uiPhrase uiStrings::phrCannotParse( const uiWord& string )
{ return toUiString(joinstring).arg(sCannotParse()).arg(string); }

uiPhrase uiStrings::phrCannotFindDBEntry( const uiWord& string )
{ return phrCannotFind( tr("database entry for %1").arg( string ) ); }

uiPhrase uiStrings::phrCannotRead( const uiWord& string )
{ return tr("Cannot read %1").arg( string ); }

uiPhrase uiStrings::phrCannotRemove( const uiWord& string )
{ return toUiString(joinstring).arg(sCannotRemove()).arg(string); }

uiPhrase uiStrings::phrCannotWrite( const uiWord& string )
{ return toUiString(joinstring).arg(sCannotWrite()).arg( string ); }

uiPhrase uiStrings::phrCannotWriteDBEntry( const uiWord& string )
{ return phrCannotWrite( tr("database entry for %1").arg(string) ); }

uiPhrase uiStrings::phrCannotSave( const uiWord& string )
{ return toUiString(joinstring).arg(sCannotSave()).arg(string); }

uiPhrase uiStrings::phrCannotStart( const uiWord& string )
{ return toUiString(joinstring).arg(sCannotStart()).arg(string); }

uiPhrase uiStrings::phrCannotUnZip( const uiWord& string )
{ return toUiString(joinstring).arg(sCannotUnZip()).arg(string); }

uiPhrase uiStrings::phrCannotZip( const uiWord& string )
{ return toUiString(joinstring).arg(sCannotZip()).arg(string); }

uiPhrase uiStrings::phrCheck( const uiWord& string )
{ return toUiString(joinstring).arg(sCheck()).arg(string); }

uiPhrase uiStrings::phrClose( const uiWord& string )
{ return toUiString(joinstring).arg(sClose()).arg(string); }

uiPhrase uiStrings::phrCopy( const uiWord& string )
{ return toUiString(joinstring).arg(sCopy()).arg(string); }

uiPhrase uiStrings::phrCreate( const uiWord& string )
{ return toUiString(joinstring).arg(sCreate()).arg(string); }

uiPhrase uiStrings::phrCreateNew( const uiWord& string )
{ return toUiString(joinstring).arg(sCreateNew()).arg(string); }

uiPhrase uiStrings::phrCrossPlot( const uiWord& string )
{ return toUiString(joinstring).arg(sCrossPlot()).arg(string); }

uiPhrase uiStrings::phrColonString( const uiWord& string )
{ return tr(": %1").arg( string ); }

uiPhrase uiStrings::phrData( const uiWord& string )
{ return toUiString(joinstring).arg(sData()).arg(string); }

uiPhrase uiStrings::phrDelete( const uiWord& string )
{ return toUiString(joinstring).arg(sDelete()).arg(string); }

uiPhrase uiStrings::phrEdit( const uiWord& string )
{ return toUiString(joinstring).arg( sEdit() ).arg( string ); }

uiPhrase uiStrings::phrEnter( const uiWord& string )
{ return toUiString(joinstring).arg(sEnter()).arg(string); }

uiPhrase uiStrings::phrExistsContinue( const uiWord& string, bool overwrite )
{
    return tr( "%1 exists. %2?")
	.arg( string )
	.arg( overwrite ? sOverwrite() : sContinue() );
}

uiPhrase uiStrings::phrExtract( const uiWord& string )
{ return toUiString(joinstring).arg(sExtract()).arg(string); }

uiPhrase uiStrings::phrGenerating( const uiWord& string )
{ return toUiString(joinstring).arg(sGenerating()).arg(string); }

uiPhrase uiStrings::phrHandling( const uiWord& string )
{ return tr( "Handling %1").arg( string ); }

uiPhrase uiStrings::phrHandled( const uiWord& string )
{ return tr( "%1 handled").arg( string ); }

uiPhrase uiStrings::phrInline( const uiWord& string )
{ return phrJoinStrings( sInline(), string ); }

uiPhrase uiStrings::phrInput( const uiWord& string )
{ return toUiString(joinstring).arg( sInput() ).arg( string ); }

uiPhrase uiStrings::phrInsert( const uiWord& string )
{ return phrJoinStrings( sInsert(), string ); }

uiPhrase uiStrings::phrInvalid( const uiWord& string )
{ return toUiString(joinstring).arg(sInvalid()).arg(string); }

uiPhrase uiStrings::phrJoinStrings( const uiWord& a, const uiWord& b )
{ return toUiString(joinstring).arg( a ).arg( b ); }

uiPhrase uiStrings::phrJoinStrings( const uiWord& a, const uiWord& b,
				    const uiPhrase& c)
{ return toUiString("%1 %2 %3").arg( a ).arg( b ).arg( c ); }

uiPhrase uiStrings::phrManage( const uiWord& string )
{ return toUiString(joinstring).arg(sManage()).arg(string); }

uiPhrase uiStrings::phrModify( const uiWord& string )
{ return toUiString(joinstring).arg(sModify()).arg(string); }

uiPhrase uiStrings::phrMerge( const uiWord& string )
{ return toUiString(joinstring).arg(sMerge()).arg(string); }

uiPhrase uiStrings::phrOpen( const uiWord& string )
{ return toUiString(joinstring).arg(sOpen()).arg(string); }

uiPhrase uiStrings::phrOutput( const uiWord& string )
{ return toUiString(joinstring).arg( sOutput() ).arg( string ); }

uiPhrase uiStrings::phrPlsContactSupport( bool firstdoc )
{
    if ( !firstdoc )
	return tr( "Please contact OpendTect support at support@dgbes.com." );
    return tr( "Please consult the documentation at opendtect.org."
	    "\nIf that fails you may want to contact OpendTect support at "
	    "support@dgbes.com.");
}

uiPhrase uiStrings::phrReading( const uiWord& string )
{ return tr( "Reading %1").arg( string ); }

uiPhrase uiStrings::phrRead( const uiWord& string )
{ return tr( "%1 read").arg( string ); }

uiPhrase uiStrings::phrRemove( const uiWord& string )
{ return toUiString(joinstring).arg(sRemove()).arg(string); }

uiPhrase uiStrings::phrRemoveSelected( const uiWord& string )
{ return toUiString(joinstring).arg(sRemoveSelected()).arg(string); }

uiPhrase uiStrings::phrRename( const uiWord& string )
{ return toUiString(joinstring).arg(sRename()).arg(string); }

uiPhrase uiStrings::phrSelectPos( const uiWord& string )
{ return toUiString(joinstring).arg(sSelectPos()).arg(string); }

uiPhrase uiStrings::phrSetAs( const uiWord& string )
{ return toUiString(joinstring).arg(sSetAs()).arg(string); }

uiPhrase uiStrings::phrSuccessfullyExported( const uiWord& string )
{ return tr( "Successfully exported %1").arg( string );}

uiPhrase uiStrings::phrZIn( const uiWord& string )
{ return tr( "Z in %1" ).arg( string ); }

uiPhrase uiStrings::phrWriting( const uiWord& string )
{ return tr( "Writing %1").arg( string ); }

uiPhrase uiStrings::phrWritten( const uiWord& string )
{ return tr( "%1 written").arg( string ); }

uiPhrase uiStrings::phrSave( const uiWord& string )
{ return toUiString(joinstring).arg(sSave()).arg(string); }

uiPhrase uiStrings::phrSaveAs( const uiWord& string )
{ return tr( "Save %1 as" ).arg( string ); }

uiPhrase uiStrings::phrShowIn( const uiWord& string )
{ return toUiString(joinstring).arg(sShowIn()).arg(string); }

uiPhrase uiStrings::phrSpecify( const uiWord& string )
{ return toUiString(joinstring).arg(sSpecify()).arg(string); }

uiPhrase uiStrings::phrStart( const uiWord& word )
{ return tr( "Start %1" ).arg( word ); }

uiPhrase uiStrings::phrStorageDir( const uiWord& string )
{ return toUiString(joinstring).arg(sStorageDir()).arg(string); }

uiPhrase uiStrings::phrLoad( const uiWord& string )
{ return toUiString(joinstring).arg(sLoad()).arg(string); }

uiPhrase uiStrings::phrLoading( const uiWord& string )
{ return tr("Loading %1").arg(string); }

uiPhrase uiStrings::phrXcoordinate( const uiWord& string )
{ return toUiString(joinstring).arg(sXcoordinate()).arg(string); }

uiPhrase uiStrings::phrYcoordinate( const uiWord& string )
{ return toUiString(joinstring).arg(sYcoordinate()).arg(string); }

uiPhrase uiStrings::phrZRange( const uiWord& string )
{ return toUiString(joinstring).arg(sZRange()).arg(string); }



uiWord uiStrings::sCannotAdd()
{ return tr("Cannot add"); }

uiWord uiStrings::sCannotCompute()
{ return tr("Cannot compute"); }

uiWord uiStrings::sCannotCopy()
{ return tr("Cannot copy"); }

uiWord uiStrings::sCannotEdit()
{ return tr("Cannot edit"); }

uiWord uiStrings::sCannotExtract()
{ return tr("Cannot extract"); }

uiWord uiStrings::sCannotFind()
{ return tr("Cannot find"); }

uiWord uiStrings::sCannotImport()
{ return tr("Cannot Import"); }

uiWord uiStrings::sCannotLoad()
{ return tr("Cannot load"); }

uiWord uiStrings::sCannotSave()
{ return tr("Cannot Save"); }

uiWord uiStrings::sCannotWrite()
{ return tr("Cannot Write"); }

uiWord uiStrings::sCannotUnZip()
{ return tr("Cannot UnZip"); }

uiWord uiStrings::sCannotZip()
{ return tr("Cannot Zip"); }

uiWord uiStrings::sCantCreateHor()
{ return phrCannotCreate( tr("horizon") ); }

uiWord uiStrings::sCantFindAttrName()
{ return phrCannotFind( tr("attribute name") ); }

uiWord uiStrings::sCantFindODB()
{ return phrCannotFind( tr("object in data base") ); }

uiWord uiStrings::sCantFindSurf()
{ return phrCannotFind( tr("surface") ); }

uiWord uiStrings::sCannotOpen()
{ return tr("Cannot open"); }

uiWord uiStrings::sCannotParse()
{ return tr("Cannot parse"); }

uiWord uiStrings::sCantReadHor()
{ return phrCannotRead( tr("horizon") ); }

uiWord uiStrings::sCantReadInp()
{ return phrCannotRead( tr("input") ); }

uiWord uiStrings::sCantWriteSettings()
{ return phrCannotWrite(tr("settings"));}

uiWord uiStrings::sCantOpenInpFile( int num )
{ return phrCannotOpen( tr("input file", 0, num ) ); }

uiWord uiStrings::sCannotStart()
{ return tr("Cannot Start"); }

uiWord uiStrings::sCheck()
{ return tr("Check"); }

uiWord uiStrings::sCheckPermissions()
{ return tr("Please check your permissions."); }

uiWord uiStrings::sOutput()
{ return tr("Output"); }

uiWord uiStrings::sCantOpenOutpFile( int num )
{ return phrCannotOpen( tr("output file", 0, num ) ); }

uiWord uiStrings::sCannotRemove()
{ return tr("Cannot remove"); }

uiWord uiStrings::sCopy()
{ return tr("Copy"); }

uiWord uiStrings::sCreateNew()
{ return mJoinUiStrs(sCreate(),sNew()); }

uiWord uiStrings::sCreateOutput()
{ return mJoinUiStrs(sCreate(),sOutput()); }

uiWord uiStrings::sCreateProbDesFunc()
{ return phrCreate( sProbDensFunc(false) ); }

uiWord uiStrings::sCrossPlot()
{ return tr("Cross Plot"); }

uiWord uiStrings::sData()
{ return tr("Data"); }

uiWord uiStrings::sDelete()
{ return tr("Delete"); }

uiWord uiStrings::sEdit()
{ return tr("Edit"); }

uiWord uiStrings::sEnter()
{ return tr("Enter"); }

uiWord uiStrings::sEnterValidName()
{ return uiStrings::phrEnter(tr("a valid name")); }

uiWord uiStrings::sExport()
{ return tr("Export"); }

uiWord uiStrings::sExtract()
{ return tr("Extract"); }

uiWord uiStrings::sFault( int num )
{ return tr("Fault", 0, num ); }

uiWord uiStrings::sFaultStickSet( int num )
{ return tr( "FaultStickSet", 0, num ); }

uiWord uiStrings::sFrequency( int num )
{
    return tr( "Frequency", 0, num );
}

uiWord uiStrings::sHelp()
{ return tr("Help"); }

uiWord uiStrings::sHistogram( )
{ return tr("Histogram"); }

uiWord uiStrings::sHorizon( int num )
{ return tr("Horizon", 0, num ); }

uiWord uiStrings::sImport()
{ return tr("Import"); }

uiWord uiStrings::sInput()
{ return tr("Input"); }

uiWord uiStrings::sInputFile()
{ return phrInput( sFile().toLower() ); }

uiWord uiStrings::sInputSelection()
{ return phrInput( sSelection().toLower() ); }

uiWord uiStrings::sInputASCIIFile()
{ return phrInput( phrASCII( sFile() )); }

uiWord uiStrings::sInputParamsMissing()
{ return tr("Input parameters missing"); }

uiWord uiStrings::sInsert()
{ return tr("Insert"); }

uiWord uiStrings::sInvalid()
{ return tr("Invalid"); }

uiWord uiStrings::sLatitude( bool abbrev )
{ return abbrev ? tr("Lat") : tr("Latitude"); }

uiWord uiStrings::sLoad()
{ return tr("Load"); }

uiWord uiStrings::sLogs()
{ return sLog(mPlural); }

uiWord uiStrings::sLongitude( bool abbrev )
{ return abbrev ? tr("Long") : tr("Longitude"); }

uiWord uiStrings::sManage()
{ return tr("Manage"); }

uiWord uiStrings::sMarker( int num )
{ return tr("Marker", 0, num); }

uiWord uiStrings::sMerge()
{ return tr("Merge"); }

uiWord uiStrings::sModify()
{ return tr("Modify"); }

uiWord uiStrings::sNew()
{ return tr("New"); }

uiWord uiStrings::sOpen()
{ return tr("Open" ); }

uiWord uiStrings::sOptions()
{ return tr("Options"); }

uiWord uiStrings::sOutputSelection()
{ return phrOutput(sSelection().toLower()); }

uiWord uiStrings::sOutputASCIIFile()
{ return phrOutput( phrASCII( sFile() )); }

uiWord uiStrings::sOutputFileExistsOverwrite()
{ return phrExistsContinue( tr("Output file"), true); }

uiWord uiStrings::sProbDensFunc( bool abbrevation, int num )
{
    return abbrevation
        ? tr( "PDF", 0, num )
        : tr("Probability Density Function", 0, num );
}

uiWord uiStrings::sProperties()
{ return tr("Properties"); }

uiWord uiStrings::sRemove()
{ return tr("Remove"); }

uiWord uiStrings::sRemoveSelected()
{ return tr("Remove Selected"); }

uiWord uiStrings::sRename()
{ return tr("Rename"); }

uiWord uiStrings::sSave()
{ return tr("Save"); }

uiWord uiStrings::sSaveAs()
{ return tr("Save as"); }

uiWord uiStrings::sSelect()
{ return tr("Select"); }

uiWord uiStrings::sSelectPos()
{ return tr("Select Position"); }

uiWord uiStrings::sSelOutpFile()
{ return uiStrings::phrSelect(tr("output file")); }

uiWord uiStrings::sSelection( int num )
{ return tr("Selection", 0, num ); }

uiWord uiStrings::sSetting( int num )
{ return tr("Setting", 0, num ); }

uiWord uiStrings::sSetAs()
{ return tr("Set As"); }

uiWord uiStrings::sShift()
{ return tr("Shift" ); }

uiWord uiStrings::sShowIn()
{ return tr("Show in"); }

uiWord uiStrings::sSpecify()
{ return tr("Specify"); }

uiWord uiStrings::sSpecifyOut()
{ return uiStrings::phrJoinStrings(tr("Specify"), uiStrings::sOutput()); }


uiWord uiStrings::sStorageDir()
{ return tr("Storage Directory"); }

uiWord uiStrings::sStored()
{ return tr("Stored" ); }

uiWord uiStrings::sStratigraphy()
{ return tr( "Stratigraphy" ); }

uiWord uiStrings::sTrack()
{ return tr("Track" ); }

uiWord uiStrings::sVolume(int num)
{ return tr("Volume",0,num); }

uiWord uiStrings::sWaveNumber( int num )
{ return tr("Wavenumber", 0, num ); }

uiWord uiStrings::sWavelet( int num )
{ return tr("Wavelet", 0, num ); }

uiWord uiStrings::sWell( int num )
{ return tr("Well", 0, num ); }

uiWord uiStrings::sWellLog( int num )
{ return tr("Well log", 0, num ); }

uiWord uiStrings::sDistUnitString(bool isfeet,bool abb, bool withparentheses)
{
    return withparentheses
	? toUiString("(%1)").arg( sDistUnitString( isfeet, abb, false ) )
	: isfeet
	    ? abb ? tr("ft") : tr("feet" )
	    : abb ? tr("m") : tr("meter");
}

uiWord uiStrings::sTimeUnitString( bool abb )
{ return abb ? tr( "s" ) : uiStrings::sSec(); }

uiWord uiStrings::sXcoordinate()
{ return tr("X-coordinate"); }

uiWord uiStrings::sYcoordinate()
{ return tr("Y-coordinate"); }uiWord uiStrings::sZRange()
{ return tr("Z Range"); }


uiWord uiStrings::sVolDataName( bool is2d, bool is3d, bool isprestack,
			      bool both_2d_3d_in_context,
			      bool both_pre_post_in_context )
{
    if ( is2d && is3d )
	return tr( "Seismic data" );

    if ( is2d )
    {
	if ( isprestack )
	{
	    if ( both_2d_3d_in_context )
	    {
		return tr( "Prestack 2D Data" );
	    }

	    return tr( "Prestack Data" );
	}

	if ( both_2d_3d_in_context )
	{
	    if ( both_pre_post_in_context )
	    {
		return tr( "Poststack 2D Data" );
	    }

	    return tr("2D Data (attribute)");
	}

	if ( both_pre_post_in_context )
	{
	    return tr("Poststack Data");
	}

	return tr("2D Data (attribute)");
    }

    if ( is3d )
    {
	if ( isprestack )
	{
	    if ( both_2d_3d_in_context )
	    {
		return tr( "Prestack 3D Data");
	    }

	    return tr( "Prestack Data" );
	}

	return tr("Cube");
    }

    return tr("Data");
}
