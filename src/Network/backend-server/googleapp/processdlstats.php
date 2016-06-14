<?php
/*
  (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
  AUTHOR   : K. Tingdahl
  DATE     : May 2016
  FUNCTION : 
	1. Reads input usage data from POST
	2. Resolves country of usage data.
	3. Writes usage data to archive
	4. Removes temporary file.
*/

use google\appengine\api\modules\ModulesService;

$datekey = 'date';
$apikeycachekey = 'apikey';
$outputpath = 'gs://opendtect-001.appspot.com/dlsites/archive/';
$apikeyfile = 'gs://opendtect-001.appspot.com/dlsites/dlsites_apikey.txt';

$failurecode = 400;
$successcode = 200;

$memcache = new Memcached;
$uniquenr = $memcache->increment('uniquenr');

if ( !array_key_exists( $datekey, $_POST ) )
{
    http_response_code($failurecode+0);
    return;
}

//Set filenames
$datestring = $_POST[$datekey];
$filenamebase = "dlstats-".$datestring."-".$uniquenr.".txt";
$outputfile = $outputpath.$filenamebase;

//Read API key
$apikey = $memcache->get( $apikeycachekey );

if ( $apikey===false )
{
    $apikey = file_get_contents( $apikeyfile );
    if ( $apikey===false )
    {
	http_response_code($failurecode+1);
	return;
    }

    $memcache->set( $apikeycachekey, $apikey );
}

$listing = $_POST;

if ( !array_key_exists( 'country', $listing ) )
{
    $ipnumber = $listing['address'];
    $iplookup = file_get_contents( "http://api.db-ip.com/addrinfo?addr=$ipnumber&api_key=$apikey" );

    if ( $iplookup!==false )
    {
	$iplookuparr = (array) json_decode( $iplookup );
	$country = '';
	if ( array_key_exists( 'country', $iplookuparr ) )
	    $country = $iplookuparr['country'];

	$listing['country'] = $country;
    }
}
    
$outputdata = json_encode( $listing )."\n";

if ( file_put_contents( $outputfile, $outputdata ) === false )
{
    http_response_code($failurecode+3);
    return;
}

http_response_code($successcode);

?>

