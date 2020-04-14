/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Prajjaval
 Date:		2015
________________________________________________________________________

-*/

#include "uistrings.h"
#include "dbkey.h"
#include "nrbytes2string.h"
#include "odsysmem.h"
#include "file.h"
#include "trckey.h"

#define mJoinStr toUiString("%1 %2")


//--- phrases with 'real' args

uiPhrase uiStrings::phrAdd( const uiWord& string )
{ return tr("Add %1").arg( string ); }

uiPhrase uiStrings::phrAllocating( od_int64 sz )
{ return tr("Allocating memory: %1").arg( sMemSizeString(sz) ); }

uiPhrase uiStrings::phrCalculate( const uiWord& string )
{ return tr("Calculate %1").arg( string ); }

uiPhrase uiStrings::phrCalculateFrom( const uiWord& string )
{ return tr("Calculate From %1").arg( string ); }

uiPhrase uiStrings::phrCrossline( const uiWord& string )
{ return tr("Cross-line %1").arg( string ); }

uiPhrase uiStrings::phrCannotAdd( const uiWord& string )
{ return tr("Cannot add %1").arg( string ); }

uiPhrase uiStrings::phrCannotCalculate( const uiWord& string )
{ return tr("Cannot calculate %1").arg( string ); }

uiPhrase uiStrings::phrCannotCopy( const uiWord& string )
{ return tr("Cannot copy %1").arg( string ); }

uiPhrase uiStrings::phrCannotCreate( const uiWord& string )
{ return tr("Cannot create %1").arg( string ); }

uiPhrase uiStrings::phrCannotCreateDBEntryFor(const uiWord& string)
{ return phrCannotCreate( tr("database entry for %1").arg( string ) ); }

uiPhrase uiStrings::phrCannotCreateDirectory( const char* dirnm )
{ return phrCannotCreate( tr("directory '%1'").arg( dirnm ) ); }

uiPhrase uiStrings::phrCannotCreateHor()
{ return phrCannotCreate( sHorizon() ); }

uiPhrase uiStrings::phrCannotCreateTempFile()
{ return phrCannotCreate(
	    tr("temporary file at %1").arg(File::getTempPath()) ); }

uiPhrase uiStrings::phrCannotEdit( const uiWord& string )
{ return tr("Cannot edit %1").arg( string ); }

uiPhrase uiStrings::phrCannotExtract( const uiWord& string )
{ return tr("Cannot extract %1").arg( string ); }

uiPhrase uiStrings::phrCannotFind( const uiWord& string )
{ return tr("Cannot find %1").arg( string ); }
uiPhrase uiStrings::phrCannotFind( const char* str )
{ return phrCannotFind( toUiString(str).quote(true) ); }
uiPhrase uiStrings::phrCannotFindDBEntry( const uiString& what )
{ return phrCannotFind( toUiString("%1 (%2)").arg(sDBEntry()).arg(what) ); }
uiPhrase uiStrings::phrCannotFindDBEntry( const DBKey& dbky )
{ return phrCannotFind( toUiString("%1 <%2>").arg(sDBEntry()).arg(dbky) ); }

uiPhrase uiStrings::phrCannotImport( const uiWord& string )
{ return tr("Cannot import %1").arg( string ); }

uiPhrase uiStrings::phrCannotLoad( const uiWord& string )
{ return tr("Cannot load %1").arg( string ); }
uiPhrase uiStrings::phrCannotLoad( const char* nm )
{ return phrCannotLoad( toUiString(nm).quote(true) ); }

uiPhrase uiStrings::phrCannotOpen( const uiWord& string )
{ return tr("Cannot open %1").arg( string ); }
uiPhrase uiStrings::phrCannotOpen( const char* fnm, bool forread )
{
    return forread ? phrCannotOpen( toUiString(fnm).quote(true) )
		   : phrCannotCreate( toUiString(fnm).quote(true) );
}
uiPhrase uiStrings::phrCannotOpenForRead( const char* fnm )
{ return phrCannotOpen( fnm, true ); }
uiPhrase uiStrings::phrCannotOpenForWrite( const char* fnm )
{ return phrCannotOpen( fnm, false ); }

uiPhrase uiStrings::phrCannotParse( const char* expr )
{ return tr("Cannot parse '%1'").arg( expr ); }

uiPhrase uiStrings::phrCannotRead( const char* fnm )
{ return phrCannotRead( toUiString(fnm).quote(true) ); }
uiPhrase uiStrings::phrCannotRead( const uiWord& string )
{ return tr("Cannot read %1").arg( string ); }

uiPhrase uiStrings::phrCannotRemove( const uiWord& string )
{ return tr("Cannot remove %1").arg( string ); }
uiPhrase uiStrings::phrCannotRemove( const char* fnm )
{ return phrCannotRemove( toUiString(fnm).quote(true) ); }

uiPhrase uiStrings::phrCannotSave( const uiWord& string )
{ return tr("Cannot save %1").arg( string ); }
uiPhrase uiStrings::phrCannotSave( const char* fnm )
{ return phrCannotSave( toUiString(fnm).quote(true) ); }

uiPhrase uiStrings::phrCannotStart( const uiWord& string )
{ return tr("Cannot start %1").arg( string ); }
uiPhrase uiStrings::phrCannotStart( const char* fnm )
{ return phrCannotStart( toUiString(fnm).quote(true) ); }

uiPhrase uiStrings::phrCannotUnZip( const uiWord& string )
{ return tr("Cannot unzip %1").arg( string ); }

uiPhrase uiStrings::phrCannotWrite( const uiWord& string )
{ return tr("Cannot write %1").arg( string ); }
uiPhrase uiStrings::phrCannotWrite( const char* fnm )
{ return phrCannotWrite( toUiString(fnm).quote(true) ); }
uiPhrase uiStrings::phrCannotWriteDBEntry( const uiWord& string )
{ return phrCannotWrite( string ); }

uiPhrase uiStrings::phrCannotZip( const uiWord& string )
{ return tr("Cannot zip %1").arg( string ); }

uiPhrase uiStrings::phrCheck( const uiWord& string )
{ return tr("You may want to check %1").arg( string ); }

uiPhrase uiStrings::phrClose( const uiWord& string )
{ return mJoinStr.arg( sClose() ).arg( string ); }

uiPhrase uiStrings::phrCopy( const uiWord& string )
{ return mJoinStr.arg( sCopy() ).arg( string ); }

uiPhrase uiStrings::phrCreate( const uiWord& string )
{ return mJoinStr.arg( sCreate() ).arg( string ); }

uiPhrase uiStrings::phrCreateNew( const uiWord& string )
{ return mJoinStr.arg( sCreateNew() ).arg( string ); }

uiPhrase uiStrings::phrCrossPlot( const uiWord& string )
{ return mJoinStr.arg( sCrossPlot() ).arg( string ); }

uiPhrase uiStrings::phrData( const uiWord& string )
{ return mJoinStr.arg( sData() ).arg( string ); }

uiPhrase uiStrings::phrDelete( const uiWord& string )
{ return mJoinStr.arg( sDelete() ).arg( string ); }

uiPhrase uiStrings::phrDiagnostic( const char* msg )
{ return toUiString("'%1'").arg(msg); }

uiPhrase uiStrings::phrDiskSpace()
{ return tr("This may be a disk space problem"); }

uiPhrase uiStrings::phrDoesNotExist(const uiWord& string )
{ return tr("%1 does not exist").arg( string ); }

uiPhrase uiStrings::phrErrCalculating( const uiWord& subj )
{ return tr("Error calculating %1").arg( subj ); }

uiPhrase uiStrings::phrEdit( const char* nm )
{ return mJoinStr.arg( sEdit() ).arg( nm ); }
uiPhrase uiStrings::phrEdit( const uiWord& string )
{ return mJoinStr.arg( sEdit() ).arg( string ); }

uiPhrase uiStrings::phrEnter( const uiWord& string )
{ return mJoinStr.arg( sEnter() ).arg( string ); }

uiPhrase uiStrings::phrErrDuringIO( bool read, const uiString& subj )
{
	return (read ? tr("Error during %1 read")
		     : tr("Error during %1 write")).arg( subj );
}

uiPhrase uiStrings::phrErrDuringIO( bool read, const char* nm )
{
    if ( !nm || !*nm )
	return read ? tr("Error during data read")
		    : tr("Error during data write");
    else
	return (read ? tr("Error during read of '%1'")
		     : tr("Error during write of '%1'")).arg( nm );
}

uiPhrase uiStrings::phrExistsContinue( const uiWord& string, bool overwrite )
{
    return tr("%1 exists. %2?").arg( string )
	.arg( overwrite ? sOverwrite() : sContinue() );
}

uiPhrase uiStrings::phrExitOD()
{ return tr("Exit OpendTect"); }

uiPhrase uiStrings::phrExport( const uiWord& string )
{ return mJoinStr.arg( sExport() ).arg( string ); }

uiPhrase uiStrings::phrExtract( const uiWord& string )
{ return mJoinStr.arg( sExtract() ).arg( string ); }

uiPhrase uiStrings::phrFileDoesNotExist( const char* fnm )
{ return phrDoesNotExist( toUiString( BufferString("'",fnm,"'") ) ); }

uiPhrase uiStrings::phrGenerating( const uiWord& string )
{ return mJoinStr.arg( sGenerating() ).arg( string ); }

uiPhrase uiStrings::phrHandled( const uiWord& string )
{ return tr("%1 handled").arg( string ); }

uiPhrase uiStrings::phrHandling( const uiWord& string )
{ return tr("Handling %1").arg( string ); }

uiPhrase uiStrings::phrImport( const uiWord& string )
{ return mJoinStr.arg( sImport() ).arg( string ); }

uiPhrase uiStrings::phrInline( const uiWord& string )
{ return tr("In-line %1").arg( string ); }

uiPhrase uiStrings::phrInput( const uiWord& string )
{ return mJoinStr.arg( sInput() ).arg( string ); }

uiPhrase uiStrings::phrPosNotFound( const TrcKey& tk )
{
    return tr("Position not found: %1/%2")
	.arg( tk.is2D() ? tk.geomID().name()
			: BufferString(toString(tk.inl())) )
	.arg( tk.trcNr() );
}

uiPhrase uiStrings::phrParamMissing( const char* pnm )
{ return tr("Parameter '%1' missing").arg( pnm ); }

uiPhrase uiStrings::phrInsert( const uiWord& string )
{ return tr("Insert %1").arg( string ); }

uiPhrase uiStrings::phrInvalid( const uiWord& string )
{ return mJoinStr.arg( sInvalid() ).arg( string ); }

uiPhrase uiStrings::phrInternalErr( const char* string )
{ return tr("Internal Error (please contact support@dgbes.com):\n%1")
	 .arg( string ); }

uiPhrase uiStrings::phrLoad( const uiWord& string )
{ return mJoinStr.arg( sLoad() ).arg( string ); }

uiPhrase uiStrings::phrLoading( const uiWord& string )
{ return tr("Loading %1").arg( string ); }

uiPhrase uiStrings::phrManage( const uiWord& string )
{ return mJoinStr.arg( sManage() ).arg( string ); }

uiPhrase uiStrings::phrMerge( const char* nm )
{ return mJoinStr.arg( sMerge() ).arg( nm ); }
uiPhrase uiStrings::phrMerge( const uiWord& string )
{ return mJoinStr.arg( sMerge() ).arg( string ); }

uiPhrase uiStrings::phrModify( const uiWord& string )
{ return mJoinStr.arg( sModify() ).arg( string ); }

uiPhrase uiStrings::phrNotImplInThisVersion( const char* fromver )
{ return tr("Not implemented in this version of OpendTect."
	  "\nPlease use version %1 or higher").arg( fromver ); }

uiPhrase uiStrings::phrIsNotSaved( const uiWord& obj )
{
    return tr("%1 not saved").arg( obj );
}

uiPhrase uiStrings::phrIsNotSavedSaveNow( const uiWord& obj )
{
    return phrIsNotSaved( obj ).appendPhrase( tr("Save now?") );
}

uiPhrase uiStrings::phrOpen( const uiWord& string )
{ return mJoinStr.arg( sOpen() ).arg( string ); }

uiPhrase uiStrings::phrOutput( const uiWord& string )
{ return mJoinStr.arg( sOutput() ).arg( string ); }

uiWord uiStrings::phrOutputFileExistsOverwrite()
{ return phrExistsContinue( sOutputFile(), true ); }

uiPhrase uiStrings::phrPlsCheckThe( const uiWord& subj )
{ return tr("You may want to check the %1").arg( subj ); }

uiPhrase uiStrings::phrPlsContactSupport( bool firstdoc )
{
    if ( !firstdoc )
	return tr("Please contact OpendTect support at support@dgbes.com.");

    uiPhrase ret( tr("Please consult the documentation at opendtect.org") );
    ret.appendPhrase( tr("If that fails you may want to contact "
			    "OpendTect support at support@dgbes.com") );
    return ret;
}

uiPhrase uiStrings::phrPlsSelectAtLeastOne( const uiWord& string )
{ return tr("You should select at least one %1").arg( string ); }

uiPhrase uiStrings::phrPlsSpecifyAtLeastOne( const uiWord& string )
{ return tr("You should specify at least one %1").arg( string ); }

uiPhrase uiStrings::phrRead( const uiWord& string )
{ return tr("%1 read").arg( string ); }

uiPhrase uiStrings::phrReading( const uiWord& string )
{ return tr("Reading %1").arg( string ); }

uiPhrase uiStrings::phrRemove( const char* nm )
{ return toUiString( "%1 '%2'" ).arg( sRemove() ).arg( nm ); }

uiPhrase uiStrings::phrRemove( const uiWord& string )
{ return mJoinStr.arg( sRemove() ).arg( string ); }

uiPhrase uiStrings::phrRemoveSelected( const uiWord& string )
{ return mJoinStr.arg( sRemoveSelected() ).arg( string ); }

uiPhrase uiStrings::phrRename( const uiWord& string )
{ return mJoinStr.arg( sRename() ).arg( string ); }

uiPhrase uiStrings::phrRestart( const uiWord& string )
{ return mJoinStr.arg( sRestart() ).arg( string ); }

uiPhrase uiStrings::phrSave( const char* nm )
{ return mJoinStr.arg( sSave() ).arg( nm ); }
uiPhrase uiStrings::phrSave( const uiWord& string )
{ return mJoinStr.arg( sSave() ).arg( string ); }

uiPhrase uiStrings::phrSaveAs( const char* nm )
{ return phrSaveAs( toUiString(nm) ); }
uiPhrase uiStrings::phrSaveAs( const uiWord& string )
{ return tr("Save %1 as" ).arg( string ); }

uiPhrase uiStrings::phrSelect( const uiWord& string )
{ return mJoinStr.arg( sSelect() ).arg( string ); }

uiPhrase uiStrings::phrSelectObjectWrongType( const uiWord& string )
{ return mJoinStr.arg( tr("Selected object is not a ") ).arg( string );}

uiPhrase uiStrings::phrSelectPos( const uiWord& string )
{ return mJoinStr.arg( sSelectPos() ).arg( string ); }

uiPhrase uiStrings::phrSetAs( const uiWord& string )
{ return mJoinStr.arg( sSetAs() ).arg( string ); }

uiPhrase uiStrings::phrShowIn( const uiWord& string )
{ return mJoinStr.arg( sShowIn() ).arg( string ); }

uiPhrase uiStrings::phrSorting( const uiWord& string )
{ return tr("Sorting %1").arg( string ); }

uiPhrase uiStrings::phrSpecify( const uiWord& string )
{ return mJoinStr.arg( sSpecify() ).arg( string ); }

uiPhrase uiStrings::phrStart( const uiWord& string )
{ return mJoinStr.arg( sStart() ).arg( string ); }

uiPhrase uiStrings::phrStarting( const uiWord& string )
{ return tr("Starting %1").arg( string ); }

uiPhrase uiStrings::phrStop( const uiWord& string )
{ return mJoinStr.arg( sStop() ).arg( string ); }

uiPhrase uiStrings::phrStorageDir( const uiWord& string )
{ return mJoinStr.arg( sStorageDir() ).arg( string ); }

uiPhrase uiStrings::phrSuccessfullyExported( const uiWord& string )
{ return tr("Successfully exported %1").arg( string );}

uiPhrase uiStrings::phrThreeDots( const uiWord& string, bool immediate )
{ return immediate ? string : toUiString( "%1 ..." ).arg( string ); }

uiPhrase uiStrings::phrTODONotImpl( const char* clssnm )
{ return toUiString( "[%1] TODO: Not Implemented" ).arg( clssnm ); }


uiPhrase uiStrings::phrUnexpected( const uiWord& obj, const char* what )
{
    uiPhrase ret = tr("Unexpected %1%2").arg( obj );
    if ( what && *what )
	ret.arg( BufferString(": ",what) );
    else
	ret.arg( "" );
    return ret;
}

uiPhrase uiStrings::phrWrite( const uiWord& string )
{ return tr("Write %1").arg( string ); }

uiPhrase uiStrings::phrWriting( const uiWord& string )
{ return tr("Writing %1").arg( string ); }

uiPhrase uiStrings::phrWritten( const uiWord& string )
{ return tr("%1 written").arg( string ); }

uiPhrase uiStrings::phrXcoordinate( const uiWord& string )
{ return mJoinStr.arg( sXcoordinate() ).arg( string ); }

uiPhrase uiStrings::phrYcoordinate( const uiWord& string )
{ return mJoinStr.arg( sYcoordinate() ).arg( string ); }

uiPhrase uiStrings::phrZIn( const uiWord& string )
{ return tr("Z in %1").arg( string ); }

uiPhrase uiStrings::phrZRange( const uiWord& string )
{ return mJoinStr.arg( sZRange() ).arg( string ); }


//--- phrases without 'real' args

uiPhrase uiStrings::phrCannotAllocateMemory( od_int64 szneeded )
{
    uiPhrase insuffstr = tr("Insufficient memory available");
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

uiPhrase uiStrings::phrCannotFindAttrName()
{ return phrCannotFind( tr("attribute name") ); }

uiPhrase uiStrings::phrCannotFindObjInDB()
{ return phrCannotFind( tr("object in data base") ); }

uiPhrase uiStrings::phrCannotOpenInpFile( int num )
{ return phrCannotOpen( tr("input file",0,num) ); }

uiPhrase uiStrings::phrCannotOpenOutpFile( int num )
{ return phrCannotOpen(tr("output file",0,num) ); }

uiPhrase uiStrings::phrCannotReadHor()
{ return phrCannotRead( sHorizon().toLower() ); }

uiPhrase uiStrings::phrCannotReadInp()
{ return phrCannotRead( sInput().toLower() ); }

uiPhrase uiStrings::phrCannotWriteSettings()
{ return phrCannotWrite(sSettings());}

uiPhrase uiStrings::phrCheckPermissions()
{ return tr("You may want to check the access permissions"); }

uiPhrase uiStrings::phrCheckUnits()
{ return tr("You may want to check the units of measure"); }

uiPhrase uiStrings::phrDBIDNotValid()
{ return tr("Database ID is not valid"); }

uiPhrase uiStrings::phrEnterValidName()
{ return uiStrings::phrEnter(tr("a valid name")); }

uiPhrase uiStrings::phrSaveBodyFail()
{ return tr("Save body failed"); }

uiPhrase uiStrings::phrSelOutpFile()
{ return uiStrings::phrSelect(tr("output file")); }

uiPhrase uiStrings::phrSpecifyOutput()
{ return uiStrings::phrSpecify( uiStrings::sOutput() ); }


//--- Words

uiWord uiStrings::sDistUnitString( bool isfeet, bool abbr )
{
    return isfeet ? (abbr ? toUiString("ft") : sFeet(false).toLower())
		  : (abbr ? toUiString("m") : sMeter(false).toLower());
}

uiWord uiStrings::sTimeUnitString( bool ismilli, bool abbr )
{
    return abbr ?  toUiString( (ismilli ? "ms" : "s") )
     : ((ismilli ? sMSec(false,mPlural)
		 : sSec(false,mPlural)).toLower());
}

uiWord uiStrings::sIsoMapType( bool istime )
{
    return istime ? sIsochron() : sIsochore();
}

uiWord uiStrings::sSeisObjName( bool is2d, bool is3d, bool isprestack,
			        bool both_2d_3d_in_context,
			        bool both_pre_post_in_context )
{
    if ( is2d && is3d )
	return sSeismicData();

    if ( is2d )
    {
	if ( isprestack )
	{
	    if ( both_2d_3d_in_context )
	    {
		return tr("Prestack 2D Data");
	    }

	    return sPreStackData();
	}

	if ( both_2d_3d_in_context )
	{
	    if ( both_pre_post_in_context )
	    {
		return tr("Poststack 2D Data");
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
		return tr("Prestack 3D Data");

	    return sPreStackData();
	}

	return sCube();
    }

    return sData();
}


uiWord uiStrings::sSeisGeomTypeName( bool is2d, bool isps )
{
    if ( is2d )
	return isps ? tr("Line 2D Pre-Stack") : s2DLine();
    else
	return isps ? tr("Pre-Stack Volume") : tr("3D Volume");
}


uiWord uiStrings::sMemSizeString( od_int64 memsz )
{
    NrBytesToStringCreator cr;
    return toUiString( cr.getString(memsz) );
}


// (possibly) multi-word compound words

uiWord uiStrings::sAdvanced( const uiWord& subj )
{
    return subj.isEmpty() ? tr("Advanced") : tr("Advanced %1").arg( subj );
}

uiWord uiStrings::sProceed( const uiWord& withwhat )
{
    uiString procstr = tr("Proceed");
    if ( withwhat.isEmpty() )
	return toUiString( "%1 >>" ).arg( procstr );
    else
	return toUiString( "%1 [%2] >>" ).arg( procstr ).arg( withwhat );
}
