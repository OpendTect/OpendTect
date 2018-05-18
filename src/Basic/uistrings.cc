/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Prajjaval
 Date:		2015
________________________________________________________________________

-*/

#include "uistrings.h"
#include "dbkey.h"

#define mJoinStr toUiString("%1 %2")

uiPhrase uiStrings::phrAdd( const uiWord& string )
{ return tr("Add %1").arg( string ); }

uiPhrase uiStrings::phrASCII( const uiWord& string )
{ return tr("ASCII %1").arg( string ); }

uiPhrase uiStrings::phrBatchProgramFailedStart()
{ return tr("Batch Program failed to start"); }

uiPhrase uiStrings::phrInterpretDataAlreadyLoadedAskForRename()
{ return tr("Interpretation data is already loaded. Enter a different name"); }

uiPhrase uiStrings::phrCalculate( const uiWord& string )
{ return tr("Calculate %1").arg( string ); }

uiPhrase uiStrings::phrCalculateFrom( const uiWord& string )
{ return tr("Calculate From %1").arg( string ); }

uiPhrase uiStrings::phrCrossline( const uiWord& string )
{ return tr("Cross-line %1").arg( string ); }

uiPhrase uiStrings::phrDoesNotExist(const uiWord& string )
{ return tr("%1 does not exist").arg( string ); }

uiPhrase uiStrings::phrCannotAdd( const uiWord& string )
{ return mJoinStr.arg( sCannotAdd() ).arg( string ); }

uiPhrase uiStrings::phrCannotAllocateMemory()
{ return tr("Cannot allocate enough memory"); }

uiPhrase uiStrings::phrCannotCalculate( const uiWord& string )
{ return tr("Cannot calculate %1").arg( string ); }

uiPhrase uiStrings::phrCannotCopy( const uiWord& string )
{ return mJoinStr.arg( sCannotCopy() ).arg( string ); }

uiPhrase uiStrings::phrCannotCreate( const uiWord& string )
{ return tr("Cannot create %1").arg( string ); }

uiPhrase uiStrings::phrCannotCreateDBEntryFor(const uiWord& string)
{ return phrCannotCreate( tr("database entry for %1").arg( string ) ); }

uiPhrase uiStrings::phrCannotCreateDirectory( const char* dirnm )
{ return phrCannotCreate( tr("directory '%1'").arg( dirnm ) ); }

uiPhrase uiStrings::phrCannotCreateHor()
{ return phrCannotCreate( sHorizon() ); }

uiPhrase uiStrings::phrCannotEdit( const uiWord& string )
{ return mJoinStr.arg( sCannotEdit() ).arg( string ); }

uiPhrase uiStrings::phrCannotExtract( const uiWord& string )
{ return mJoinStr.arg( sCannotExtract() ).arg( string ); }

uiPhrase uiStrings::phrCannotFind( const uiWord& string )
{ return mJoinStr.arg( sCannotFind() ).arg( string ); }

uiPhrase uiStrings::phrCannotFindDBEntry( const uiWord& string )
{ return phrCannotFind( tr("database entry for %1").arg( string ) ); }

uiPhrase uiStrings::phrCannotImport( const uiWord& string )
{ return mJoinStr.arg( sCannotImport() ).arg( string ); }

uiPhrase uiStrings::phrCannotLoad( const uiWord& string )
{ return mJoinStr.arg( sCannotLoad() ).arg( string ); }

uiPhrase uiStrings::phrCannotOpen( const uiWord& string )
{ return mJoinStr.arg( sCannotOpen() ).arg( string ); }

uiPhrase uiStrings::phrCannotOpen( const char* fnm )
{ return phrCannotOpen( toUiString( BufferString("'",fnm,"'") ) ); }

uiPhrase uiStrings::phrCannotParse( const uiWord& string )
{ return mJoinStr.arg( sCannotParse() ).arg( string ); }

uiPhrase uiStrings::phrCannotRead( const uiWord& string )
{ return tr("Cannot read %1").arg( string ); }

uiPhrase uiStrings::phrCannotRemove( const uiWord& string )
{ return mJoinStr.arg( sCannotRemove() ).arg( string ); }

uiPhrase uiStrings::phrCannotSave( const uiWord& string )
{ return mJoinStr.arg( sCannotSave() ).arg( string ); }

uiPhrase uiStrings::phrCannotStart( const uiWord& string )
{ return mJoinStr.arg( sCannotStart() ).arg( string ); }

uiPhrase uiStrings::phrCannotUnZip( const uiWord& string )
{ return mJoinStr.arg( sCannotUnZip() ).arg( string ); }

uiPhrase uiStrings::phrCannotWrite( const uiWord& string )
{ return mJoinStr.arg( sCannotWrite() ).arg( string ); }

uiPhrase uiStrings::phrCannotWriteDBEntry( const uiWord& string )
{ return phrCannotWrite( tr("database entry for %1").arg( string ) ); }

uiPhrase uiStrings::phrCannotZip( const uiWord& string )
{ return mJoinStr.arg( sCannotZip() ).arg( string ); }

uiPhrase uiStrings::phrCheck( const uiWord& string )
{ return tr("Please check %1").arg( string ); }

uiPhrase uiStrings::phrClose( const uiWord& string )
{ return mJoinStr.arg( sClose() ).arg( string ); }

uiPhrase uiStrings::phrColonString( const uiWord& string )
{ return tr(": %1").arg( string ); }

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

uiPhrase uiStrings::phrDiskSpace()
{ return tr("This may be a disk space problem"); }

uiPhrase uiStrings::phrErrCalculating( const uiWord& subj )
{ return tr("Error calculating %1").arg( subj ); }

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

uiPhrase uiStrings::phrInsert( const uiWord& string )
{ return tr("Insert %1").arg( string ); }

uiPhrase uiStrings::phrInvalid( const uiWord& string )
{ return mJoinStr.arg( sInvalid() ).arg( string ); }

uiPhrase uiStrings::phrInternalErr( const char* string )
{ return tr("Internal Error (pease contact support@dgbes.com):\n%1")
	 .arg( string ); }

uiPhrase uiStrings::phrJoinStrings( const char* a, const char* b )
{ return mJoinStr.arg( a ).arg( b ); }

uiPhrase uiStrings::phrJoinStrings( const char* a, const char* b,
				    const char* c )
{ return toUiString("%1 %2 %3").arg( a ).arg( b ).arg( c ); }

uiPhrase uiStrings::phrLoad( const uiWord& string )
{ return mJoinStr.arg( sLoad() ).arg( string ); }

uiPhrase uiStrings::phrLoading( const uiWord& string )
{ return tr("Loading %1").arg( string ); }

uiPhrase uiStrings::phrManage( const uiWord& string )
{ return mJoinStr.arg( sManage() ).arg( string ); }

uiPhrase uiStrings::phrMerge( const uiWord& string )
{ return mJoinStr.arg( sMerge() ).arg( string ); }

uiPhrase uiStrings::phrModify( const uiWord& string )
{ return mJoinStr.arg( sModify() ).arg( string ); }

uiPhrase uiStrings::phrNotImplInThisVersion( const char* fromver )
{ return tr("Not implemented in this version of OpendTect."
	  "\nPlease use version %1 or higher").arg( fromver ); }

uiPhrase uiStrings::phrOpen( const uiWord& string )
{ return mJoinStr.arg( sOpen() ).arg( string ); }

uiPhrase uiStrings::phrOutput( const uiWord& string )
{ return mJoinStr.arg( sOutput() ).arg( string ); }

uiPhrase uiStrings::phrInterpretationDataExist( uiWord type, const char* nm)
{
    return tr("A %1 with name '%2' already exists").arg( type ).arg( nm );
}

uiWord uiStrings::phrOutputFileExistsOverwrite()
{ return phrExistsContinue( sOutputFile(), true ); }

uiPhrase uiStrings::phrPlsCheckThe( const uiWord& subj )
{ return tr("Please chack the %1").arg( subj ); }

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
{ return tr("Please select at least one %1").arg( string ); }

uiPhrase uiStrings::phrPlsSpecifyAtLeastOne( const uiWord& string )
{ return tr("Please specify at least one %1").arg( string ); }

uiPhrase uiStrings::phrRead( const uiWord& string )
{ return tr("%1 read").arg( string ); }

uiPhrase uiStrings::phrReading( const uiWord& string )
{ return tr("Reading %1").arg( string ); }

uiPhrase uiStrings::phrRemove( const uiWord& string )
{ return mJoinStr.arg( sRemove() ).arg( string ); }

uiPhrase uiStrings::phrRemoveSelected( const uiWord& string )
{ return mJoinStr.arg( sRemoveSelected() ).arg( string ); }

uiPhrase uiStrings::phrRename( const uiWord& string )
{ return mJoinStr.arg( sRename() ).arg( string ); }

uiPhrase uiStrings::phrSave( const uiWord& string )
{ return mJoinStr.arg( sSave() ).arg( string ); }

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

uiPhrase uiStrings::phrStart( const uiWord& word )
{ return tr("Start %1" ).arg( word ); }

uiPhrase uiStrings::phrStorageDir( const uiWord& string )
{ return mJoinStr.arg( sStorageDir() ).arg( string ); }

uiPhrase uiStrings::phrSuccessfullyExported( const uiWord& string )
{ return tr("Successfully exported %1").arg( string );}

uiPhrase uiStrings::phrThreeDots( const uiWord& string, bool immediate )
{ return immediate ? string : toUiString( "%1 ..." ).arg( string ); }

uiPhrase uiStrings::phrTODONotImpl( const char* clssnm )
{ return toUiString( "[%1] TODO: Not Implemented" ).arg( clssnm ); }

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


// (possibly) multi-word compound words

uiWord uiStrings::sAdvanced( const uiWord& subj )
{
    return subj.isEmpty() ? tr("Advanced") : tr("Advanced %1").arg( subj );
}

// From here all are actually phrases or most certainly illegal stuff

uiWord uiStrings::sCannotAdd()
{ return tr("Cannot add"); }

uiWord uiStrings::sCannotAllocate()
{ return tr("Cannot allocate memory"); }

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

uiWord uiStrings::sCannotOpen()
{ return tr("Cannot open"); }

uiWord uiStrings::sCannotParse()
{ return tr("Cannot parse"); }

uiWord uiStrings::sCannotRemove()
{ return tr("Cannot remove"); }

uiWord uiStrings::sCannotSave()
{ return tr("Cannot Save"); }

uiWord uiStrings::sCannotStart()
{ return tr("Cannot Start"); }

uiWord uiStrings::sCannotWrite()
{ return tr("Cannot Write"); }

uiWord uiStrings::sCannotUnZip()
{ return tr("Cannot UnZip"); }

uiWord uiStrings::sCannotZip()
{ return tr("Cannot Zip"); }

uiWord uiStrings::sCantFindAttrName()
{ return phrCannotFind( tr("attribute name") ); }

uiWord uiStrings::sCantFindODB()
{ return phrCannotFind( tr("object in data base") ); }

uiWord uiStrings::sCantFindSurf()
{ return phrCannotFind( sSurface().toLower() ); }

uiWord uiStrings::sCantOpenInpFile( int num )
{ return phrCannotOpen( tr("input file", 0, num ) ); }

uiWord uiStrings::sCantOpenOutpFile( int num )
{ return phrCannotOpen( tr("output file", 0, num ) ); }

uiWord uiStrings::sCantReadHor()
{ return phrCannotRead( sHorizon().toLower() ); }

uiWord uiStrings::sCantReadInp()
{ return phrCannotRead( sInput().toLower() ); }

uiWord uiStrings::sCantWriteSettings()
{ return phrCannotWrite(sSettings());}

uiWord uiStrings::sCheckPermissions()
{ return tr("Please check your permissions"); }

uiWord uiStrings::sCreateProbDesFunc()
{ return phrCreate( sProbDensFunc(false) ); }

uiWord uiStrings::sEnterValidName()
{ return uiStrings::phrEnter(tr("a valid name")); }

uiWord uiStrings::sInputParamsMissing()
{ return tr("Input parameters missing"); }

uiWord uiStrings::sSaveBodyFail()
{ return tr("Save body failed"); }

uiWord uiStrings::sSceneWithNr( int scnnr )
{ return mJoinStr.arg( sScene() ).arg( scnnr ); }

uiWord uiStrings::sSelOutpFile()
{ return uiStrings::phrSelect(tr("output file")); }

uiWord uiStrings::sSpecifyOut()
{ return uiStrings::phrSpecify( uiStrings::sOutput() ); }
