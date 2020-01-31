<?php
/*
 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 AUTHOR   :	K. Tingdahl
 DATE     :	June 2012

 crashreport.php - receives text and file with 'report' and 'dumpfile' as the key.
 If only text is received, it is sent as email. If file is also uploaded along with the 
 text, the sent email contains text as part of mail and file as attachment.
*/

use PHPMailer\PHPMailer\PHPMailer;
use PHPMailer\PHPMailer\Exception;

require 'PHPMailer/src/Exception.php';
require 'PHPMailer/src/PHPMailer.php';
require 'PHPMailer/src/SMTP.php';

require_once('googlestorage.php');

$localsettingsfile = realpath(dirname(__FILE__))."/settings.local.php";
if(file_exists($localsettingsfile))
{
    include_once( $localsettingsfile );
}

$credentialsFile = "/customers/2/1/5/opendtect.org/httpd.private/crashreport-uploader-credentials.json";
$bucket = "opendtect-crashreports";
$recipient = 'crashreports@dgbes.com';
$fromaddress = 'noreply@dgbes.com';
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
   
    if ( file_exists( $credentialsFile ) ) 
	uploadGoogleStorageFile( $credentialsFile, $bucket, "incoming/".$crashid.".json", $filecontent, true );

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
	
    $mail = new PHPMailer(true);
    try {
        $mail->SMTPDebug = 0;
        $mail->isSMTP();
        $mail->Host = "mailout.one.com";
        $mail->SMTPAuth = true;
        $mail->Username = "noreply@dgbes.com";
        $mail->Password = $SMTPMAIL_PASSWORD;
        $mail->SMTPSecure = "";
        $mail->Port = 25;
        $mail->Timeout = 60;

        $mail->setFrom('noreply@dgbes.com', 'Crash Reporter');
        $mail->AddAddress($recipient, "Support");

        $mail->Subject = $subject;
        $mail->Body = $message;
	$mail->addCustomHeader('Reply-To: '. $recipient);
	if ( array_key_exists($dumpvarname,$_FILES) )
	{
	    if ( !is_dir($dumpfolder) )
		mkdir( $dumpfolder );

	    $filename = basename( $_FILES[$dumpvarname]['name'] );
	    $target = $dumpfolder.'/'.$filename;
	    if ( move_uploaded_file($_FILES[$dumpvarname]['tmp_name'],$target) )
		echo "The file ".$filename. " has been uploaded";
	    else
		echo "Sorry, there was a problem uploading your file.";
	    
	    $mail->AddAttachment( $target , $crashid . "_" . $filename );
	}
	
        $mail->send();
	echo "Report submitted with ID ".$crashid.".\n\n";
        echo "Thank you for helping us improve OpendTect!\n";
	} catch (Exception $e) {
        echo 'Report could not be sent to support.';
        echo 'Mailer Error: ' . $mail->ErrorInfo;
	}

}

if ( $dohtml )
{
    echo "</p> </body> </html>";
}
?>
