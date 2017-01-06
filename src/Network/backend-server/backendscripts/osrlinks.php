<?php
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer
 Date:		Jun 2016
________________________________________________________________________

-*/

include_once( 'dlsitessettings.php' );

require "$DLSITES_AWS_PATH/aws-autoloader.php";

use Aws\S3\S3Client;

date_default_timezone_set( 'UTC' );

$s3Client = S3Client::factory(array(
    'version'     => 'latest',
    'region'      => 'eu-west-1',
    'credentials' => [
        'key'    => $DLSITES_AWS_ACCESS_ID,
        'secret' => $DLSITES_AWS_ACCESS_KEY
    ]
) );

$bucket = 'opendtect-osr';

echo '<?xml version="1.0" encoding="utf-8"?>'."\n";
echo '<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01//EN"'.
     '"http://www.w3.org/TR/html4/strict.dtd">'.
    '<html>'.
    '<head>'.
    '<meta http-equiv="Content-Type" content="text/html; charset=utf-8">'.
    '<title>'.
    '</title>'.
    '</head>'.
    '<body>'.
    '<p>These links are valid for 7 days starting at '.
	date("D M j G:i:s T Y")."<br>\n";

$prefix = "osr/";


try {
    $objects = $s3Client->getIterator('ListObjects', array(
        'Bucket' => $bucket,
	'Delimiter' => '/',
	'Prefix' => $prefix 
	    ));

    foreach ($objects as $object) {
	$cmd = $s3Client->getCommand('GetObject', [
	    'Bucket' => $bucket,
	    'Key'    => $object['Key'] ]);
	$name = str_replace($prefix,"",$object['Key']);

	if ( $name=='' )
	    continue;

	$request = $s3Client->createPresignedRequest($cmd, '+7 days');

	// Get the actual presigned-url
	$presignedUrl = (string) $request->getUri();
	echo '<a href="'.$presignedUrl.'">'.$name.'</a><br>'."\n";
    }
} catch (Exception $e) {
    echo $e->getMessage() . "\n";
}

echo '</body></html>';

