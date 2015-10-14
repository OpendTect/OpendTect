<?php
include_once( "dtect/arcanist/dgbarcanist/linter/TabTools.php");

$myFile = $argv[1];
$fh = fopen($myFile, 'r');
$theData = fread($fh, 50000);
fclose($fh);
echo tab_expand( $theData );
?>
