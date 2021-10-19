/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Lammertink
 Date:		25/08/1999
________________________________________________________________________

-*/

#include "uistrings.h"
#include "multiid.h"
#include "nrbytes2string.h"
#include "odsysmem.h"

static const char* joinstring = "%1 %2";

uiString uiStrings::phrAdd( const uiString& string )
{ return toUiString(joinstring).arg( sAdd() ).arg( string ); }

uiString uiStrings::phrAllocating( od_int64 sz )
{ return tr("Allocating memory: %1").arg( sMemSizeString(sz) ); }

uiString uiStrings::phrASCII( const uiString& string )
{ return toUiString(joinstring).arg( sASCII() ).arg( string ); }

uiString uiStrings::phrCalculate( const uiString& string )
{ return toUiString(joinstring).arg(sCalculate()).arg(string); }

uiString uiStrings::phrCalculateFrom( const uiString& string )
{ return toUiString(joinstring).arg(sCalculateFrom()).arg(string); }

uiString uiStrings::phrCrossline( const uiString& string )
{ return phrJoinStrings( sCrossline(), string ); }

uiString uiStrings::phrTODONotImpl( const char* clssnm )
{ return toUiString( "[%1] TODO: Not Implemented" ).arg( clssnm ); }

uiString uiStrings::phrNotImplInThisVersion( const char* fromver )
{ return tr("Not implemented in this version of OpendTect."
	  "\nPlease use version %1 or higher").arg( fromver ); }

uiString uiStrings::phrThreeDots( const uiString& string, bool immediate )
{ return immediate ? string : toUiString( "%1 ..." ).arg( string ); }

uiString uiStrings::phrSelect( const uiString& string )
{ return toUiString(joinstring).arg( sSelect() ).arg( string ); }

uiString uiStrings::phrSelectObjectWrongType( const uiString& string )
{ return toUiString(joinstring).arg(tr("Select object is not a ")).arg(string);}

uiString uiStrings::phrDoesntExist(const uiString& string, int num )
{ return tr( "%1 does not exist", 0, num ).arg( string ); }

uiString uiStrings::phrExport( const uiString& string )
{ return toUiString(joinstring).arg( sExport() ).arg( string ); }

uiString uiStrings::phrImport( const uiString& string )
{ return toUiString(joinstring).arg( sImport() ).arg( string ); }

uiString uiStrings::phrCannotAdd( const uiString& string )
{ return toUiString(joinstring).arg(sCannotAdd()).arg(string); }

uiString uiStrings::phrCannotCopy( const uiString& string )
{ return toUiString(joinstring).arg(sCannotCopy()).arg(string); }

uiString uiStrings::phrCannotCreate( const uiString& string )
{ return tr("Cannot create %1").arg( string ); }

uiString uiStrings::phrCannotCreateDBEntryFor(const uiString& string)
{ return phrCannotCreate( tr("database entry for %1").arg(string) ); }

uiString uiStrings::phrCannotCreateDirectory( const uiString& string )
{ return phrCannotCreate( tr("directory %1").arg(string) ); }

uiString uiStrings::phrCannotEdit( const uiString& string )
{ return toUiString(joinstring).arg(sCannotEdit()).arg(string); }

uiString uiStrings::phrCannotExtract( const uiString& string )
{ return toUiString(joinstring).arg(sCannotExtract()).arg(string); }

uiString uiStrings::phrCannotFind( const uiString& string )
{ return tr("Cannot find %1").arg( string ); }

uiString uiStrings::phrCannotImport( const uiString& string )
{ return toUiString(joinstring).arg(sCannotImport()).arg(string); }

uiString uiStrings::phrCannotLoad( const uiString& string )
{ return toUiString(joinstring).arg(sCannotLoad()).arg(string); }

uiString uiStrings::phrCannotOpen( const uiString& string )
{ return toUiString(joinstring).arg(sCannotOpen()).arg( string ); }

uiString uiStrings::phrCannotOpen( const char* fnm, bool forread )
{
    return forread ? phrCannotOpen( toUiString(fnm).quote(true) )
		   : phrCannotCreate( toUiString(fnm).quote(true) );
}
uiString uiStrings::phrCannotOpenForRead( const char* fnm )
{ return phrCannotOpen( fnm, true ); }
uiString uiStrings::phrCannotOpenForWrite( const char* fnm )
{ return phrCannotOpen( fnm, false ); }

uiString uiStrings::phrCannotFindDBEntry( const uiString& string )
{ return phrCannotFind( tr("database entry for %1").arg( string ) ); }

uiString uiStrings::phrCannotFindDBEntry( const MultiID& dbkey )
{ return phrCannotFind( tr("database entry for %1").arg( dbkey ) ); }

uiString uiStrings::phrCannotRead( const uiString& string )
{ return tr("Cannot read %1").arg( string ); }

uiString uiStrings::phrCannotRead( const char* string )
{ return tr("Cannot read %1").arg( string ); }

uiString uiStrings::phrCannotRemove( const uiString& string )
{ return toUiString(joinstring).arg(sCannotRemove()).arg(string); }

uiString uiStrings::phrCannotRemove( const char* string )
{ return toUiString(joinstring).arg(sCannotRemove()).arg(string); }

uiString uiStrings::phrCannotWrite( const uiString& string )
{ return toUiString(joinstring).arg(sCannotWrite()).arg( string ); }

uiString uiStrings::phrCannotWrite( const char* string )
{ return toUiString(joinstring).arg(sCannotWrite()).arg( string ); }

uiString uiStrings::phrCannotWriteDBEntry( const uiString& string )
{ return phrCannotWrite( tr("database entry for %1").arg(string) ); }

uiString uiStrings::phrCannotSave( const uiString& string )
{ return toUiString(joinstring).arg(sCannotSave()).arg(string); }

uiString uiStrings::phrCannotStart( const uiString& string )
{ return toUiString(joinstring).arg(sCannotStart()).arg(string); }

uiString uiStrings::phrCannotUnZip( const uiString& string )
{ return toUiString(joinstring).arg(sCannotUnZip()).arg(string); }

uiString uiStrings::phrCannotZip( const uiString& string )
{ return toUiString(joinstring).arg(sCannotZip()).arg(string); }

uiString uiStrings::phrCheck( const uiString& string )
{ return toUiString(joinstring).arg(sCheck()).arg(string); }

uiString uiStrings::phrCopy( const uiString& string )
{ return toUiString(joinstring).arg(sCopy()).arg(string); }

uiString uiStrings::phrCreate( const uiString& string )
{ return toUiString(joinstring).arg(sCreate()).arg(string); }

uiString uiStrings::phrCreateNew( const uiString& string )
{ return toUiString(joinstring).arg(sCreateNew()).arg(string); }

uiString uiStrings::phrCrossPlot( const uiString& string )
{ return toUiString(joinstring).arg(sCrossPlot()).arg(string); }

uiString uiStrings::phrColonString( const uiString& string )
{ return tr(": %1").arg( string ); }

uiString uiStrings::phrData( const uiString& string )
{ return toUiString(joinstring).arg(sData()).arg(string); }

uiString uiStrings::phrDelete( const uiString& string )
{ return toUiString(joinstring).arg(sDelete()).arg(string); }

uiString uiStrings::phrDiagnostic( const char* msg )
{ return toUiString("'%1'").arg(msg); }

uiString uiStrings::phrEdit( const uiString& string )
{ return toUiString(joinstring).arg( sEdit() ).arg( string ); }

uiString uiStrings::phrEnter( const uiString& string )
{ return toUiString(joinstring).arg(sEnter()).arg(string); }

uiString uiStrings::phrErrDuringIO( bool read, const uiString& subj )
{
	return (read ? tr("Error during %1 read")
		     : tr("Error during %1 write")).arg( subj );
}

uiString uiStrings::phrErrDuringIO( bool read, const char* nm )
{
    if ( !nm || !*nm )
	return read ? tr("Error during data read")
		    : tr("Error during data write");
    else
	return (read ? tr("Error during read of '%1'")
		     : tr("Error during write of '%1'")).arg( nm );
}
uiString uiStrings::phrExistsContinue( const uiString& string, bool overwrite )
{
    return tr( "%1 exists. %2?")
	.arg( string )
	.arg( overwrite ? sOverwrite() : sContinue() );
}

uiString uiStrings::phrExtract( const uiString& string )
{ return toUiString(joinstring).arg(sExtract()).arg(string); }

uiString uiStrings::phrFileDoesNotExist( const char* fnm )
{ return phrDoesntExist(toUiString("%1 '%2'").arg(sFile()).arg(fnm)); }

uiString uiStrings::phrGenerating( const uiString& string )
{ return toUiString(joinstring).arg(sGenerating()).arg(string); }

uiString uiStrings::phrInline( const uiString& string )
{ return phrJoinStrings( sInline(), string ); }

uiString uiStrings::phrInput( const uiString& string )
{ return toUiString(joinstring).arg( sInput() ).arg( string ); }

uiString uiStrings::phrInsert( const uiString& string )
{ return phrJoinStrings( sInsert(), string ); }

uiString uiStrings::phrInternalErr( const char* string )
{ return tr("Internal Error (please contact support@dgbes.com).\n\n%1")
	 .arg( string ); }

uiString uiStrings::phrInvalid( const uiString& string )
{ return toUiString(joinstring).arg(sInvalid()).arg(string); }

uiString uiStrings::phrJoinStrings( const uiString& a, const uiString& b )
{ return toUiString(joinstring).arg( a ).arg( b ); }

uiString uiStrings::phrJoinStrings( const uiString& a, const uiString& b,
				    const uiString& c)
{ return toUiString("%1 %2 %3").arg( a ).arg( b ).arg( c ); }

uiString uiStrings::phrManage( const uiString& string )
{ return toUiString(joinstring).arg(sManage()).arg(string); }

uiString uiStrings::phrModify( const uiString& string )
{ return toUiString(joinstring).arg(sModify()).arg(string); }

uiString uiStrings::phrMerge( const uiString& string )
{ return toUiString(joinstring).arg(sMerge()).arg(string); }

uiString uiStrings::phrOpen( const uiString& string )
{ return toUiString(joinstring).arg(sOpen()).arg(string); }

uiString uiStrings::phrOutput( const uiString& string )
{ return toUiString(joinstring).arg( sOutput() ).arg( string ); }

uiString uiStrings::phrPlsSelectAtLeastOne( const uiString& string )
{ return tr("Please select at least one %1").arg(string); }

uiString uiStrings::phrReading( const uiString& string )
{ return tr( "Reading %1").arg( string ); }

uiString uiStrings::phrRemove( const uiString& string )
{ return toUiString(joinstring).arg(sRemove()).arg(string); }

uiString uiStrings::phrRemoveSelected( const uiString& string )
{ return toUiString(joinstring).arg(sRemoveSelected()).arg(string); }

uiString uiStrings::phrRename( const uiString& string )
{ return toUiString(joinstring).arg(sRename()).arg(string); }

uiString uiStrings::phrSelectPos( const uiString& string )
{ return toUiString(joinstring).arg(sSelectPos()).arg(string); }

uiString uiStrings::phrSetAs( const uiString& string )
{ return toUiString(joinstring).arg(sSetAs()).arg(string); }

uiString uiStrings::phrSuccessfullyExported( const uiString& string )
{ return tr( "Successfully exported %1").arg( string );}

uiString uiStrings::phrUnexpected( const uiString& obj, const char* what )
{
    uiPhrase ret = tr("Unexpected %1%2").arg( obj );
    if ( what && *what )
	ret.arg( BufferString(": ",what) );
    else
	ret.arg( "" );
    return ret;
}

uiString uiStrings::phrZIn( const uiString& string )
{ return tr( "Z in %1" ).arg( string ); }

uiString uiStrings::phrWriting( const uiString& string )
{ return tr( "Writing %1").arg( string ); }

uiString uiStrings::phrSave( const uiString& string )
{ return toUiString(joinstring).arg(sSave()).arg(string); }

uiString uiStrings::phrShowIn( const uiString& string )
{ return toUiString(joinstring).arg(sShowIn()).arg(string); }

uiString uiStrings::phrSpecify( const uiString& string )
{ return toUiString(joinstring).arg(sSpecify()).arg(string); }

uiString uiStrings::phrStorageDir( const uiString& string )
{ return toUiString(joinstring).arg(sStorageDir()).arg(string); }

uiString uiStrings::phrLoad( const uiString& string )
{ return toUiString(joinstring).arg(sLoad()).arg(string); }

uiString uiStrings::phrXcoordinate( const uiString& string )
{ return toUiString(joinstring).arg(sXcoordinate()).arg(string); }

uiString uiStrings::phrYcoordinate( const uiString& string )
{ return toUiString(joinstring).arg(sYcoordinate()).arg(string); }

uiString uiStrings::phrZRange( const uiString& string )
{ return toUiString(joinstring).arg(sZRange()).arg(string); }

uiString uiStrings::sBatchProgramFailedStart()
{ return tr("Batch program failed to start"); }

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

uiString uiStrings::sCheckPermissions()
{ return tr("Please check your permissions."); }

uiString uiStrings::sCantOpenOutpFile( int num )
{ return phrCannotOpen( tr("output file", 0, num ) ); }

uiString uiStrings::sCreateProbDesFunc()
{ return phrCreate( sProbDensFunc(false) ); }

uiString uiStrings::sEnterValidName()
{ return uiStrings::phrEnter(tr("a valid name")); }

uiString uiStrings::sFrequency( int num )
{ return tr( "Frequency", 0, num ); }

uiString uiStrings::sInputASCIIFile()
{ return phrInput( phrASCII( sFile().toLower() )); }

uiString uiStrings::sInputParamsMissing()
{ return tr("Input parameters missing"); }

uiWord uiStrings::sMemSizeString( od_int64 memsz )
{
    NrBytesToStringCreator cr;
    return toUiString( cr.getString(memsz) );
}

uiString uiStrings::sOutputASCIIFile()
{ return phrOutput( phrASCII( sFile().toLower() )); }

uiString uiStrings::sOutputFileExistsOverwrite()
{ return phrExistsContinue( tr("Output file"), true); }

uiString uiStrings::sProbDensFunc( bool abbrevation, int num )
{
    return abbrevation
	? tr( "PDF", 0, num )
	: tr("Probability Density Function", 0, num );
}

uiString uiStrings::sSeismic( int num )
{ return tr("Seismic", 0, num ); }

uiString uiStrings::sSeismics( bool is2d, bool isps, int num )
{
    return toUiString( "%1 %2%3" )
	.arg( is2d ? s2D() : s3D() )
	.arg( isps ? tr("Prestack ") : uiString::emptyString() )
	.arg( sSeismic( num ) );
}

uiString uiStrings::sStorageDir()
{ return tr("Storage Directory"); }

uiString uiStrings::sWaveNumber( int num )
{ return tr("Wavenumber", 0, num ); }

uiString uiStrings::sDistUnitString(bool isfeet,bool abb, bool withparentheses)
{
    return withparentheses
	? toUiString("(%1)").arg( sDistUnitString( isfeet, abb, false ) )
	: isfeet
	    ? abb ? tr("ft") : uiStrings::sFeet().toLower()
	    : abb ? tr("m") : uiStrings::sMeter().toLower();
}

uiString uiStrings::sTimeUnitString( bool abb )
{ return abb ? tr( "s" ) : uiStrings::sSec(); }


//--- phrases without 'real' args

uiString uiStrings::phrCannotAllocateMemory( od_int64 szneeded )
{
    uiString insuffstr = tr("Insufficient memory available");
    if ( szneeded <= 0 )
	return insuffstr;

    od_int64 totmem, freemem;
    OD::getSystemMemory( totmem, freemem );
    NrBytesToStringCreator b2s( totmem );

    return toUiString("%1 (%2: %3, %4: %5/%6)")
	.arg( insuffstr )
	.arg( sRequired() )
	.arg( b2s.getString(szneeded) )
	.arg( sAvailable() )
	.arg( b2s.getString(freemem) )
	.arg( b2s.getString(totmem) );
}

uiString uiStrings::phrCannotFindAttrName()
{ return phrCannotFind( tr("attribute name") ); }

uiString uiStrings::phrCannotFindObjInDB()
{ return phrCannotFind( tr("object in data base") ); }

uiString uiStrings::phrCannotOpenInpFile( int num )
{ return phrCannotOpen( tr("input file",0,num) ); }

uiString uiStrings::phrCannotOpenOutpFile( int num )
{ return phrCannotOpen(tr("output file",0,num) ); }

uiString uiStrings::phrCannotReadHor()
{ return phrCannotRead( sHorizon().toLower() ); }

uiString uiStrings::phrCannotReadInp()
{ return phrCannotRead( sInput().toLower() ); }

uiString uiStrings::phrCannotWriteSettings()
{ return phrCannotWrite(sSettings());}

uiString uiStrings::phrCheckPermissions()
{ return tr("You may want to check the access permissions"); }

uiString uiStrings::phrCheckUnits()
{ return tr("You may want to check the units of measure"); }

uiString uiStrings::phrDBIDNotValid()
{ return tr("Database ID is not valid"); }

uiString uiStrings::phrEnterValidName()
{ return uiStrings::phrEnter(tr("a valid name")); }

uiString uiStrings::phrSaveBodyFail()
{ return tr("Save body failed"); }

uiString uiStrings::phrSelOutpFile()
{ return uiStrings::phrSelect(tr("output file")); }

uiString uiStrings::phrSpecifyOutput()
{ return uiStrings::phrSpecify( uiStrings::sOutput() ); }


uiString uiStrings::sVolDataName(bool is2d, bool is3d, bool isprestack,
			     bool both_2d_3d_in_context,
			     bool both_pre_post_in_context )
{
    if ( is2d && is3d )
	return tr( "Seismic Data" );

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
