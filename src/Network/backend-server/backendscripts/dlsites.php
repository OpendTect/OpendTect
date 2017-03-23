<?php
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer
 Date:		Jun 2016
________________________________________________________________________

-*/

include( 'dlsitessettings.php' );

header('Content-Type: text/plain');

//Get IP address
if ( array_key_exists( 'REMOTE_ADDR', $_SERVER ) )
    $address = $_SERVER['REMOTE_ADDR'];

//Record date
$datestring = date("Y-m-d\TH:i:s\Z");

//Setup array to be written
$filearray = $_GET;
$filearray['date'] = $datestring;
$filearray['address'] = $address;

if ( $DLSITES_DATA_ROOT_DIR=='' )
{ $DLSITES_DATA_ROOT_DIR = $_SERVER['DOCUMENT_ROOT']; }

$outputdir = $DLSITES_DATA_ROOT_DIR."/".$DLSITES_UNPROCESSED_DIR."/";

//Create directory if need be
if ( !file_exists( $outputdir ) )
{
    mkdir( $outputdir );
}

$filenamebase = "dlstats-".$datestring."-".getmypid();
$filename = $filenamebase.".txt";
$tmpfilename = $filenamebase.".tmp";
$outputfile = $outputdir.$tmpfilename;

$outputurl = $DLSITES_DOWNLOAD_SITE;

/*Write to .tmp file first. As the processing scripts only reads .txt, it will hence not 
  read a half-done file.
  If the file already exists, rename to tmp, append it, and then rename it back.
*/

if ( file_exists( $outputdir.$filename ) )
    rename( $outputdir.$filename, $outputfile );

$iplookup = file_get_contents( "http://api.db-ip.com/v2/$DLSITES_IP_API_KEY/$address" );
if ( $iplookup!==false )
{
    $iplookuparr = (array) json_decode( $iplookup );
    if ( array_key_exists( 'countryCode', $iplookuparr ) )
    {
	$filearray['country'] = $iplookuparr['countryCode'];

	$country = strtolower( $iplookuparr['countryCode'] );
	if ( in_array( $country, $DLSITES_CLOUDFRONT_COUNTRIES ) )
	   $outputurl = $DLSITES_CLOUDFRONT_URL;
    }
}

if ( file_put_contents( $outputfile, json_encode( $filearray )."\n", FILE_APPEND | LOCK_EX ) !== false )
{
    rename( $outputfile, $outputdir.$filename );
}

echo $outputurl."\n";
