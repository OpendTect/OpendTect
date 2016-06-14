<?php
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer
 Date:		Jun 2016
________________________________________________________________________

-*/

include( 'settings.php' );

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

$outputdir = $_SERVER['DOCUMENT_ROOT']."/".$DLSITES_OUTPUT_DIR."/";

//Create directory if need be
if ( !file_exists( $outputdir ) )
{
    mkdir( $outputdir );
}

$filenamebase = "dlstats-".$datestring."-".getmypid();
$filename = $filenamebase.".txt";
$tmpfilename = $filenamebase.".tmp";
$outputfile = $outputdir.$tmpfilename;

/*Write to .tmp file first. As the processing scripts only reads .txt, it will hence not 
  read a half-done file.
  If the file already exists, rename to tmp, append it, and then rename it back.
*/

if ( file_exists( $outputdir.$filename ) )
    rename( $outputdir.$filename, $outputfile );

if ( file_put_contents( $outputfile, json_encode( $filearray ), FILE_APPEND | LOCK_EX ) !== false )
{
    rename( $outputfile, $outputdir.$filename );
}

echo $DLSITES_DOWNLOAD_SITE."\n";
