<?php

header('Content-Type: text/plain');
use google\appengine\api\modules\ModulesService;


$platform = "";
$id = "";
$address = "";

if ( array_key_exists( 'REMOTE_ADDR', $_SERVER ) )
    $address = $_SERVER['REMOTE_ADDR'];

if ( array_key_exists( 'p', $_GET ) )
    $platform = $_GET['p'];

if ( array_key_exists( 'i', $_GET ) )
    $id = $_GET['i'];

$filearray = $_GET;

$datestring = date("Y-m-d\TH:i:s\Z");

$filearray['date'] = $datestring;
$filearray['address'] = $address;

$memcache = new Memcached;
$uniquenr = $memcache->increment('uniquenr');

$outputdir = "gs://opendtect-001.appspot.com/dlsites/tmp/";
$filenamebase = "dlstats-".$datestring."-".$uniquenr;
$filename = $filenamebase.".txt";

$outputfile = $outputdir.$filename;

file_put_contents( $outputfile, json_encode( $filearray ) );

echo "cloud.opendtect.org"."\n";
echo "download.opendtect.org"."\n";

?>

