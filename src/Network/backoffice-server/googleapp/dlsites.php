<?php

header('Content-Type: text/plain');
use google\appengine\api\taskqueue\PushTask;

date_default_timezone_set( 'UTC' );


$platform = "";
$id = "";
$address = "";

/*First, attempt to see if address is set in _GET.
  This may be the case if the request is forwarded to us,
  and the address is already captured. Otherwise, capture
  the address from _SEVER */

if ( array_key_exists( 'REMOTE_ADDR', $_GET ) )
    $address = $_GET['REMOTE_ADDR'];
else if ( array_key_exists( 'REMOTE_ADDR', $_SERVER ) )
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

echo "download.opendtect.org"."\n";

?>

