<?php
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer
 Date:		Jun 2016
________________________________________________________________________

-*/

include_once( 'dlsitesdb.php' );
include_once( 'dlsitessystemid.php' );
include_once( 'dlsitessettings.php' );

require_once( 'googlestorage.php' );

date_default_timezone_set( 'UTC' );

function store_entry( $db, $tablename, $time, $id, $platform, $country, $nrcpu, $mem )
{
    global $DLSITES_TABLE_PREFIX;
    global $DLSITES_PERIOD_TABLE_PREFIX;
    global $DLSITES_IP_API_KEY;

    // last char Z needs to be removed due to changes of timestamp formatting in MySQL
    if ( substr($time, -1) === "Z" ) {
    	$time = substr($time, 0, -1);
    }

    $tablename = $DLSITES_TABLE_PREFIX.$DLSITES_PERIOD_TABLE_PREFIX.$tablename;

    $query = "CREATE TABLE IF NOT EXISTS `$tablename` ("
      	    ."`id` BIGINT UNSIGNED NOT NULL, "
      	    ."`time` DATETIME, "
      	    ."`platform` VARCHAR(6) NOT NULL, "
      	    ."`country` VARCHAR(10) DEFAULT NULL, "
      	    ."`memory` BIGINT UNSIGNED DEFAULT NULL, "
      	    ."`cpu` INT(8) DEFAULT NULL "
    	    .") ENGINE=InnoDB";

    if ( $db->query( $query ) === false )
    {
	echo "Cannot create table $tablename\n";
	return false;
    }

    $query = "INSERT INTO `$tablename` (`time`,`id`,`platform`,`country`,`memory`,`cpu` ) "
	    ."VALUES ( '$time', '$id', '$platform', '$country', $mem, $nrcpu )";

    if ( $db->query( $query )=== false )
    {
	echo "Cannot insert item into table $tablename\n";
	return false;
    }

    return true;
}

$mysqli = connect_dlsitesdb();

if ( $DLSITES_DATA_ROOT_DIR=='' )
{ $DLSITES_DATA_ROOT_DIR = $_SERVER['DOCUMENT_ROOT']; }

$inputdir = $DLSITES_DATA_ROOT_DIR."/".$DLSITES_UNPROCESSED_DIR."/";
$archivedir = $DLSITES_DATA_ROOT_DIR."/".$DLSITES_ARCHIVE_DIR."/";
$processeddir = $DLSITES_DATA_ROOT_DIR."/".$DLSITES_PROCESSED_DIR."/";


if ( !file_exists( $inputdir ) )		{ echo "$inputdir does not exist\n"; exit ( 1 ); }
if ( !file_exists( $archivedir ) )		{ echo "$archivedir does not exist\n"; exit ( 1 ); }
if ( !file_exists( $processeddir ) )		{ echo "$processeddir does not exist\n"; exit ( 1 ); }

$use_country_db = false;
$dbip = ''; //Make variable in this scope

foreach(glob($inputdir."/*.txt", GLOB_NOSORT) as $file)   
{  
    if ( !file_exists( $file ) ) //It may be temporary renamed
	continue;

    echo "Processing $file: ";

    $inputdata = file_get_contents( $file );

    if ( $inputdata===false )
    {
	echo "Cannot read $file\n";
	continue;
    }

    $inputarray = explode( "\n", $inputdata );
    $archivename = $archivedir.basename( $file );
    $processedname = $processeddir.basename( $file );
    $gsarchivename = $DLSITES_GS_ARCHIVE_PATH."/".basename( $file );

    if ( file_exists( $archivename ) )
    {
	echo "Removing $archivename";
	unlink( $archivename );
    }

    $outputarray = array();
    foreach ( $inputarray as $input )
    {
	$listing = (array) json_decode( $input );
	if ( empty( $listing ) )
	    continue;

	if ( !array_key_exists( 'address', $listing ) )
	{
	    echo "Cannot read address in $file. Skipping\n";
	    continue;
	}

	if ( !array_key_exists( 'country', $listing ) || $listing['country']=='' )
	{
	    $ipnumber = $listing['address'];
	    $iplookup = file_get_contents( "http://api.db-ip.com/v2/$DLSITES_IP_API_KEY/$ipnumber" );
	    if ( $iplookup!==false )
	    {
		$iplookuparr = (array) json_decode( $iplookup );
		if ( array_key_exists( 'countryCode', $iplookuparr ) )
		    $listing['country'] = $iplookuparr['countryCode'];
	    }
	}

	if ( !array_key_exists( 'country', $listing ) || $listing['country']=='' )
	{
	    echo "Cannot resolve country for ".$listing['address']."\n";
	    exit( 1 );
        }

	array_push( $outputarray, $listing );
    }

    $archivetext = '';
    foreach ( $outputarray as $listing )
    {
	$machash = '';
	//Read machash. If not available, try old variable name.
	if ( array_key_exists( 'machash', $listing ) )
	    $machash = $listing['machash'];
	else if ( array_key_exists( 'i', $listing ) )
	    $machash = $listing['i'];

	$platform = array_key_exists( 'p', $listing ) ? $listing['p'] : '';
	$country =  array_key_exists( 'country', $listing ) ? $listing['country'] : '';
	$nrcpu =  array_key_exists( 'c', $listing ) ? $listing['c'] : "NULL";
	$mem =  array_key_exists( 'm', $listing ) ? $listing['m'] : "NULL";

	if ( $platform!='' && $machash!='' )
	{
	    $timestring = $listing['date'];
	    $timestamp = new DateTime( $timestring );
	    $year_table_name = $timestamp->format( "Y" ); 
	    $month_table_name = $timestamp->format( "Y-m" );
	    $total_table_name = 'total';

	    $id = resolve_system_id( $mysqli, $machash, $platform, $nrcpu, $mem, true );

	    if ( !store_entry( $mysqli, $year_table_name, $timestring, $id, $platform, $country, $nrcpu, $mem ) ||
		 !store_entry( $mysqli, $month_table_name, $timestring, $id, $platform, $country, $nrcpu, $mem ) ||
		 !store_entry( $mysqli, $total_table_name, $timestring, $id, $platform, $country, $nrcpu, $mem ) )
	    {
		echo "Failure in storing entry from $file\n";
	    }
	}

	$archivetext .= json_encode( $listing )."\n";
    }

    $renameresult = rename( $file, $processedname );
    $savegsresult = uploadGoogleStorageFile( $DLSITES_GS_CREDENTIALS, $DLSITES_GS_BUCKET,
			$gsarchivename, $archivetext, true );
    
    $saveresult = file_put_contents( $archivename, $archivetext );

    if ( $renameresult===false )
	echo "Could not move $file to $processedname.\n";

    if ( $savegsresult===false )
	echo "Could not write to GS.\n";

    if ( $saveresult===false )
	echo "Could not write to $archivename.\n";

    if ( $renameresult===false || $savegsresult===false || $saveresult===false )
	exit( 1 );

    echo "Done\n";
}

