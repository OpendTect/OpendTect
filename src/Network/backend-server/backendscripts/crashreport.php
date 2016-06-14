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

require_once('class.phpmailer.php');

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
    $message = "Remote IP:\t".$_SERVER['REMOTE_ADDR']."\n\r".
                "Remote Host:\t".$_SERVER['REMOTE_HOST']."\n\r".
                "Time:\t".date("F j, Y, g:i a T")."\n\r".
                "ID:\t".$crashid."\n\r".
                "Report: \n\r\n\r".$report;

    $subject = "OpendTect crash report ".date( "F j, Y, g:i a" );

    $mail = new PHPMailer( true );
    $mail->IsSendmail();

    try
    {
        $mail->AddAddress( $recipient, 'Crash Report' );

        $mail->SetFrom( $fromaddress, 'Crash Reporter' );
        $mail->Subject = $subject;

        $mail->WordWrap = 100;
        $mail->Body = $message;
        $mail->IsHTML( false );

        $target = "";

        if ( array_key_exists($dumpvarname,$_FILES) )
        {
            if ( !is_dir($dumpfolder) )
                mkdir( $dumpfolder );

            $fileprefix = date("F_j_Y_g_i_a_");
            $filename = basename( $_FILES[$dumpvarname]['name'] );
            $target = $dumpfolder.'/'.$fileprefix.getmypid().$filename;

            if ( move_uploaded_file($_FILES[$dumpvarname]['tmp_name'], $target) )
                echo "The file ".$filename. " has been uploaded. ";
            else
                echo "Sorry, there was a problem uploading your file.";

            $mail->AddAttachment( $target,  $fileprefix.$filename );
        }

        $mail->Send();
        echo "Report submitted with ID ".$crashid.".\n\n";
        echo "Thank you for helping us improve OpendTect!\n";

        if ( file_exists($target) )
            unlink ( $target );
    }

    catch ( phpmailerException $e )
    {
        echo $e->errorMessage();
    }
    catch ( Exception $e )
    {
        echo $e->getMessage();
    }

}

if ( $dohtml )
{
    echo "</p> </body> </html>";
}
?>

