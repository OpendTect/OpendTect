<?php
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer
 Date:		Jun 2016
________________________________________________________________________

-*/

include( 'settings.php' );

if ( array_key_exists( 'REMOTE_ADDR', $_SERVER ) )
    $address = $_SERVER['REMOTE_ADDR'];

$filearray = $_GET;

$datestring = date("Y-m-d\TH:i:s\Z");

$filearray['date'] = $datestring;
$filearray['address'] = $address;

$outputdir = $_SERVER['DOCUMENT_ROOT']."/".$DLSITES_OUTPUT_DIR."/";

if ( !file_exists( $outputdir ) )
{
    mkdir( $outputdir );
}

$filenamebase = "dlstats-".$datestring."-".getmypid();
$filename = $filenamebase.".txt";
$tmpfilename = $filenamebase.".tmp";

$outputfile = $outputdir.$tmpfilename;

if ( file_put_contents( $outputfile, json_encode( $filearray ), FILE_APPEND | LOCK_EX ) !== false )
{
    rename( $outputfile, $outputdir.$filename );
}

echo $DLSITES_DOWNLOAD_SITE."\n";
