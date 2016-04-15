<?php
/*
 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 AUTHOR   :     K. Tingdahl
 DATE     :     June 2012

 crashreport.php - receives text and file with 'report' and 'dumpfile' as the key.
 If only text is received, it is sent as email. If file is also uploaded along with the 
 text, the sent email contains text as part of mail and file as attachment.
*/

use google\appengine\api\mail\Message;

$recipient = 'crashreports@dgbes.com';
$fromaddress = 'crashreports@opendtect-001.appspotmail.com';
$reportvarname = 'report';
$textkey = 'return_text';
$dumpvarname = 'dumpfile';

/* No settings below. */

$report = '';
$dohtml = true;

if ( array_key_exists($textkey,$_REQUEST) )
{
    $dohtml = false;
}

if ( $dohtml )
{
    echo '<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01//EN"'.
     '"http://www.w3.org/TR/html4/strict.dtd">'.
    '<html>'.
    '<head>'.
    '<meta http-equiv="Content-Type" content="text/html; charset=utf-8">'.
    '<title>'.
    '</title>'.
    '</head>'.
    '<body>'.
    '<p>';
}
else
{
    header('Content-Type: text/plain');
}

if ( array_key_exists($reportvarname,$_REQUEST) )
        $report = $_REQUEST[$reportvarname];

if ( $report == '' )
{
    echo "Crash report not received";
}
else
{
    date_default_timezone_set( 'UTC' );

    //Try to read submitted file
    $filecontent = '';
    $filename = '';

    if ( array_key_exists( $dumpvarname, $_FILES ) &&
         array_key_exists( 'name', $_FILES[$dumpvarname] ) )
    {
	$filename = basename( $_FILES[$dumpvarname]['name'] );
	$filecontent = file_get_contents( $_FILES[$dumpvarname]['tmp_name'] );

	if ( $filecontent!==false )
	    echo "The file ".$filename. " has been uploaded. ";
	else
	    echo "Sorry, there was a problem uploading your file. ";
    }

    //Create a crash ID
    $memcache = new Memcached;
    $uniquenr = $memcache->increment('uniquenr');
    $crashid = date('YmdHis').$uniquenr;

    //Compose e-mail
    $messagebody = "Remote IP:\t".$_SERVER['REMOTE_ADDR']."\n\r".
                //"Remote Host:\t".$_SERVER['REMOTE_HOST']."\n\r".
                "Time:\t".date("F j, Y, g:i a T")."\n\r".
                "ID:\t".$crashid."\n\r".
                "Report: \n\r\n\r".$report;

    $subject = "OpendTect crash report ".date( "F j, Y, g:i a" );
  
    //Send e-mail 
    try {
	$message = new Message();
	$message->setSender( $fromaddress );
	$message->addTo( $recipient );
	$message->setSubject( $subject );
	$message->setTextBody( $messagebody );

	if ( $filecontent!='' )
	    $message->addAttachment($crashid.'_'.$filename, $filecontent );

	$message->send();
	echo "Report submitted with ID ".$crashid.".\n\n";
	echo "Thank you for helping us improve OpendTect!\n";
    }

    catch (InvalidArgumentException $e)
    {   
	echo "Could not send crash-report. Please forward message below to support@dgbes.com:\n\n";
        echo $e;
    }
}

if ( $dohtml )
{
    echo "</p> </body> </html>";
}
?>
