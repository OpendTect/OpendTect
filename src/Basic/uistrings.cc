/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Lammertink
 Date:		25/08/1999
 RCS:		$Id$
________________________________________________________________________

-*/

#include "uistrings.h"
#include "multiid.h"

static const char* joinstring = "%1 %2";

uiString uiStrings::phrAdd( const uiString& string )
{ return toUiString(joinstring).arg( sAdd() ).arg( string ); }

uiString uiStrings::phrASCII( const uiString& string )
{ return toUiString(joinstring).arg( sASCII() ).arg( string ); }

uiString uiStrings::phrCalculateFrom( const uiString& string )
{ return toUiString(joinstring).arg(sCalculateFrom()).arg(string); }

uiString uiStrings::phrCrossline( const uiString& string )
{ return phrJoinStrings( sCrossline(), string ); }

uiString uiStrings::phrThreeDots( const uiString& string, bool immediate )
{ return immediate ? string : toUiString( "%1 ..." ).arg( string ); }

uiString uiStrings::phrSelect( const uiString& string )
{ return toUiString(joinstring).arg( sSelect() ).arg( string ); }

uiString uiStrings::phrDoesntExist(const uiString& string, int num )
{ return tr( "%1 does not exist", 0, num ); }

uiString uiStrings::phrExport( const uiString& string )
{ return toUiString(joinstring).arg( sExport() ).arg( string ); }

uiString uiStrings::phrImport( const uiString& string )
{ return toUiString(joinstring).arg( sImport() ).arg( string ); }

uiString uiStrings::phrCannotCreate( const uiString& string )
{ return tr("Cannot create %1").arg( string ); }

uiString uiStrings::phrCannotCreateDBEntryFor(const uiString& string)
{ return phrCannotCreate( tr("database entry for %1").arg(string) ); }

uiString uiStrings::phrCannotCreateDirectory( const uiString& string )
{ return phrCannotCreate( tr("directory %1").arg(string) ); }

uiString uiStrings::phrCannotFind( const uiString& string )
{ return tr("Cannot find %1").arg( string ); }

uiString uiStrings::phrCannotImport( const uiString& string )
{ return toUiString(joinstring).arg(sCannotImport()).arg(string); }

uiString uiStrings::phrCannotOpen( const uiString& string )
{ return tr("Cannot open %1").arg( string ); }

uiString uiStrings::phrCannotFindDBEntry( const uiString& string )
{ return phrCannotFind( tr("database entry for %1").arg( string ) ); }

uiString uiStrings::phrCannotRead( const uiString& string )
{ return tr("Cannot read %1").arg( string ); }

uiString uiStrings::phrCannotWrite( const uiString& string )
{ return tr("Cannot write %1").arg( string ); }

uiString uiStrings::phrCannotWriteDBEntry( const uiString& string )
{ return phrCannotWrite( tr("database entry for %1").arg(string) ); }

uiString uiStrings::phrCannotSave( const uiString& string )
{ return toUiString(joinstring).arg(sCannotSave()).arg(string); }

uiString uiStrings::phrCopy( const uiString& string )
{ return toUiString(joinstring).arg(sCopy()).arg(string); }

uiString uiStrings::phrCreate( const uiString& string )
{ return toUiString(joinstring).arg( sCreate() ).arg( string ); }

uiString uiStrings::phrEdit( const uiString& string )
{ return toUiString(joinstring).arg( sEdit() ).arg( string ); }

uiString uiStrings::phrExistsConinue( const uiString& string, bool overwrite )
{
    return tr( "%1 exists. %2?")
	.arg( string )
	.arg( overwrite ? sOverwrite() : sContinue() );
}

uiString uiStrings::phrInline( const uiString& string )
{ return phrJoinStrings( sInline(), string ); }

uiString uiStrings::phrInput( const uiString& string )
{ return toUiString(joinstring).arg( sInput() ).arg( string ); }

uiString uiStrings::phrInvalid( const uiString& string )
{ return toUiString(joinstring).arg(sInvalid()).arg(string); }

uiString uiStrings::phrJoinStrings( const uiString& a, const uiString& b )
{ return toUiString(joinstring).arg( a ).arg( b ); }

uiString uiStrings::phrJoinStrings( const uiString& a, const uiString& b,
				    const uiString& c)
{ return toUiString("%1 %2 %3").arg( a ).arg( b ).arg( c ); }

uiString uiStrings::phrModify( const uiString& string )
{ return toUiString(joinstring).arg(sModify()).arg(string); }

uiString uiStrings::phrMerge( const uiString& string )
{ return toUiString(joinstring).arg(sMerge()).arg(string); }

uiString uiStrings::phrOutput( const uiString& string )
{ return toUiString(joinstring).arg( sOutput() ).arg( string ); }

uiString uiStrings::phrReading( const uiString& string )
{ return tr( "Reading %1").arg( string ); }

uiString uiStrings::phrSuccessfullyExported( const uiString& string )
{ return tr( "Successfully exported %1").arg( string );}

uiString uiStrings::phrZIn( const uiString& string )
{ return tr( "Z in %1" ).arg( string ); }

uiString uiStrings::phrWriting( const uiString& string )
{ return tr( "Writing %1").arg( string ); }

uiString uiStrings::phrSave( const uiString& string )
{ return toUiString(joinstring).arg(sSave()).arg(string); }

uiString uiStrings::phrLoad( const uiString& string )
{ return toUiString(joinstring).arg(sLoad()).arg(string); }

uiString uiStrings::phrZRange( const uiString& string )
{ return toUiString(joinstring).arg(sZRange()).arg(string); }

uiString uiStrings::s2D()
{ return tr("2D"); }

uiString uiStrings::s3D()
{ return tr("3D"); }

uiString uiStrings::sAdd()
{ return tr("Add"); }

uiString uiStrings::sASCII()
{ return tr("ASCII"); }

uiString uiStrings::sAttributes()
{ return tr("Attributes"); }

uiString uiStrings::sColorTable()
{ return tr("ColorTable"); }

uiString uiStrings::sCreate()
{ return tr("Create"); }

uiString uiStrings::sCalculateFrom()
{ return tr("Calculate From"); }

uiString uiStrings::sCannotImport()
{ return tr("Cannot Import"); }

uiString uiStrings::sCannotSave()
{ return tr("Cannot Save"); }

uiString uiStrings::sCantCreateHor()
{ return phrCannotCreate( tr("horizon") ); }

uiString uiStrings::sCantFindAttrName()
{ return phrCannotFind( tr("attribute name") ); }

uiString uiStrings::sCantFindODB()
{ return phrCannotFind( tr("object in data base") ); }

uiString uiStrings::sCantFindSurf()
{ return phrCannotFind( tr("surface") ); }

uiString uiStrings::sCantReadHor()
{ return phrCannotRead( tr("horizon") ); }

uiString uiStrings::sCantReadInp()
{ return phrCannotRead( tr("input") ); }

uiString uiStrings::sCantWriteSettings()
{ return phrCannotWrite(tr("settings"));}

uiString uiStrings::sCantOpenInpFile( int num )
{ return phrCannotOpen( tr("input file", 0, num ) ); }

uiString uiStrings::sOutput()
{ return tr("Output"); }

uiString uiStrings::sCantOpenOutpFile( int num )
{ return phrCannotOpen( tr("output file", 0, num ) ); }

uiString uiStrings::sCopy()
{ return tr("Copy"); }

uiString uiStrings::sCreateProbDesFunc()
{ return phrCreate( sProbDensFunc() ); }

uiString uiStrings::sEdit()
{ return tr("Edit"); }

uiString uiStrings::sEnterValidName()
{ return tr("Please enter a valid name"); }

uiString uiStrings::sExport()
{ return tr("Export"); }

uiString uiStrings::sFault( int num )
{ return tr("Fault", 0, num ); }

uiString uiStrings::sFaultStickSet( int num )
{ return tr( "FaultStickSet", 0, num ); }

uiString uiStrings::sFrequency(bool smallcase, bool plural)
{ 
    if ( smallcase )
    {
	if( plural )
	    return tr("frequencies");
	
	else
	    return tr("frequency");
    }
    else
    {
	if( plural )
	    return tr("Frequencies");
	else
	    return tr("Frequency");
    }
}

uiString uiStrings::sHelp()
{ return tr("Help"); }

uiString uiStrings::sHistogram( )
{ return tr("Histogram"); }

uiString uiStrings::sHorizon( int num )
{ return tr("Horizon", 0, num ); }

uiString uiStrings::sImport()
{ return tr("Import"); }

uiString uiStrings::sInput()
{ return tr("Input"); }

uiString uiStrings::sInputASCIIFile()
{ return phrInput( phrASCII( sFile() )); }

uiString uiStrings::sInputParamsMissing()
{ return tr("Input parameters missing"); }

uiString uiStrings::sInvalid()
{ return tr("Invalid"); }
uiString uiStrings::sLoad()
{ return tr("Load"); }

uiString uiStrings::sLogs()
{ return tr("Logs"); }

uiString uiStrings::sMarker( int num )
{ return tr("Marker", 0, num); }

uiString uiStrings::sMerge()
{ return tr("Merge"); }

uiString uiStrings::sModify()
{ return tr("Modify"); }

uiString uiStrings::sNew()
{ return tr("New"); }

uiString uiStrings::sOpen()
{ return tr("Open" ); }

uiString uiStrings::sOptions()
{ return tr("Options"); }

uiString uiStrings::sOutputASCIIFile()
{ return phrOutput( phrASCII( sFile() )); }

uiString uiStrings::sOutputFileExistsOverwrite()
{ return phrExistsConinue( tr("Output file"), true); }

uiString uiStrings::sProbDensFunc( bool abbrevation )
{
    return abbrevation
       ? tr( "PDF" )
       : tr("Probability Density Function");
}

uiString uiStrings::sProperties()
{ return tr("Properties"); }

uiString uiStrings::sRemove()
{ return tr("Remove"); }

uiString uiStrings::sSave()
{ return tr("Save"); }

uiString uiStrings::sSaveAs()
{ return tr("Save as"); }

uiString uiStrings::sSeismic( int num )
{ return tr("Seismic", 0, num ); }

uiString uiStrings::sSeismics( bool is2d, bool isps, int num )
{
    return toUiString( "%1 %2%3" )
	.arg( is2d ? s2D() : s3D() )
	.arg( isps ? tr("prestack ") : uiString::emptyString() )
	.arg( sSeismic( num ) );
}

uiString uiStrings::sSelect()
{ return tr("Select"); }

uiString uiStrings::sSelOutpFile()
{ return tr("Please select output file"); }

uiString uiStrings::sSelection(bool smallletters)
{ return smallletters ? tr("selection") : tr("Selection"); }

uiString uiStrings::sSetting( int num )
{ return tr("Setting", 0, num ); }

uiString uiStrings::sShift()
{ return tr("Shift" ); }

uiString uiStrings::sStored()
{ return tr("Stored" ); }

uiString uiStrings::sStratigraphy()
{ return tr( "Stratigraphy" ); }

uiString uiStrings::sTrack()
{ return tr("Track" ); }

uiString uiStrings::sVolume()
{ return tr("Volume"); }

uiString uiStrings::sWaveNumber(bool smallcase, bool plural)
{ 
    if ( smallcase )
    {
	if( plural )
	    return tr("wavenumbers");
	
	else
	    return tr("wavenumber");
    }
    else
    {
	if( plural )
	    return tr("Wavenumbers");
	else
	    return tr("Wavenumber");
    }
}

uiString uiStrings::sWavelet( int num )
{ return tr("Wavelet", 0, num ); }

uiString uiStrings::sWell( int num )
{ return tr("Well", 0, num ); }

uiString uiStrings::sWellLog( int num )
{ return tr("Well log", 0, num ); }

uiString uiStrings::sDistUnitString(bool isfeet,bool abb, bool withparentheses)
{
    return withparentheses
	? toUiString("(%1)").arg( sDistUnitString( isfeet, abb, false ) )
	: isfeet
	    ? abb ? tr("ft") : tr("feet" )
	    : abb ? tr("m") : tr("meter");
}

uiString uiStrings::sZRange()
{ return tr("Z Range"); }


uiString uiStrings::sVolDataName(bool is2d, bool is3d, bool isprestack,
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
		return tr( "Pre-Stack 2D Data" );
	    }

	    return tr( "Pre-Stack Data" );
	}

	if ( both_2d_3d_in_context )
	{
	    if ( both_pre_post_in_context )
	    {
		return tr( "Post-Stack 2D Data" );
	    }

	    return tr("2D Data (attribute)");
	}

	if ( both_pre_post_in_context )
	{
	    return tr("Post-Stack Data");
	}

	return tr("2D Data (attribute)");
    }

    if ( is3d )
    {
	if ( isprestack )
	{
	    if ( both_2d_3d_in_context )
	    {
		return tr( "Pre-Stack 3D Data");
	    }

	    return tr( "Pre-Stack Data" );
	}

	return tr("Cube");
    }

    return tr("Data");
}
