<?php
/*
 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 AUTHOR   :     K. Tingdahl
 DATE     :     June 2012
                $Id: crashreport.php,v 1.4 2012/06/29 18:26:51 cvsdgb Exp $

 crashreport.php - receives text and file with 'report' and 'dumpfile' as the key.
 If only text is received, it is sent as email. If file is also uploaded along with the 
 text, the sent email contains text as part of mail and file as attachment.
*/

require_once('googlestorage.php');

$recipient = 'crashreports@dgbes.com';
$fromaddress = 'crashreports@opendtect.org';
$reportvarname = 'report';
$textkey = 'return_text';
$dumpvarname = 'dumpfile';
$dumpfolder = 'dumpuploads';

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
    $crashid = date('YmdHis');

    //Upload to google cloud
    $filearray = array(
	'report' => $report,
	'remote_ip' => $_SERVER['REMOTE_ADDR'],
	'remote_host' => $_SERVER['REMOTE_HOST'],
	'timestamp' => time(),
        'crash_id' => $crashid );

    $filecontent = json_encode( $filearray );
   
    $credentialsFile = "crashreport-uploader-credentials.json";
    if ( file_exists( $credentialsFile ) ) 
	uploadGoogleStorageFile( "crashreport-uploader-credentials.json", "opendtect-crashreports", $crashid.".txt", $filecontent );

    //Send e-mail
    $message = "Remote IP:\t".$_SERVER['REMOTE_ADDR']."\n\r".
                "Remote Host:\t".$_SERVER['REMOTE_HOST']."\n\r".
                "Time:\t".date("F j, Y, g:i a T")."\n\r".
                "ID:\t".$crashid."\n\r".
                "Report: \n\r\n\r".$report;

    $subject = "OpendTect crash report ".date( "F j, Y, g:i a" );

    $header = 'MIME-Version: 1.0' . "\r\n"
            . 'From: Crash Reporter <'. $fromaddress . ">\r\n"
            . 'Reply-To: '. $recipient . "\r\n"
            . 'X-Mailer: PHP/' . phpversion();

    if ( !mail($recipient, $subject, $message, $header) )
    {
	echo "Cannot send report to support";
    }

    echo "Report submitted with ID ".$crashid.".\n\n";
    echo "Thank you for helping us improve OpendTect!\n";

}

if ( $dohtml )
{
    echo "</p> </body> </html>";
}
?>

