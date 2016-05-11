<?php

header('Content-Type: text/plain');
use google\appengine\api\taskqueue\PushTask;

date_default_timezone_set( 'UTC' );


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

$task = new PushTask( '/processdlstats.php', $filearray );
$task->add();

echo "cloud.opendtect.org"."\n";
echo "download.opendtect.org"."\n";

?>

