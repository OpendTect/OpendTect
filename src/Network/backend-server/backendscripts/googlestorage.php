<?php
/*
 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 AUTHOR   :     K. Tingdahl
 DATE     :     Nov 2016

 Utilities to access files to google storage in php.

*/

require_once __DIR__ . '/google-api-php-client-2.1.0/vendor/autoload.php';

function uploadGoogleStorageFile( $credentialsFile, $bucket, $filename, $contents )
{
    try {
	$client = new Google_Client();
	$client->setApplicationName($_SERVER['PHP_SELF']);
	$client->setScopes( "https://www.googleapis.com/auth/devstorage.read_write" );
	$client->setAuthConfig( $credentialsFile );

	$storage = new Google_Service_Storage($client);

	$obj = new Google_Service_Storage_StorageObject();
	$obj->setName($filename);
	$storage->objects->insert(
	    $bucket,
	    $obj,
	    ['name' => $filename, 'data' => $contents, 'uploadType' => 'media']
	);
    }
    catch ( Google_Service_Exception $e )
    {
	print_r( $e->getErrors() );
	return false;
    }
    catch ( Exception $e )
    {
	return false;
    }

    return true;
}
