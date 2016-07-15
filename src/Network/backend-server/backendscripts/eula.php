<?php
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Kristofer
 Date:          Jun 2016
________________________________________________________________________

Displays the EULA in HTML.

-*/

$eulatext = file_get_contents( dirname(__FILE__)."/../../../../doc/eula.txt" );
$eulaarray = explode( "\n", $eulatext );

$margin = 'margin-left: 5em; margin-right: 5em;';

$DefaultStyle = "$margin ".
		"text-align: justify; ".
		"font-size: xx-small; ".
		"margin-bottom: 0em; ".
		"margin-top: 0em;";

$Level1Style = "$margin ".
		"font-size: xx-small; ".
		"font-weight: bold";

$Level2Style = "$margin ".
		"text-align: justify; ".
		"font-size: xx-small; ".
		"margin-bottom: 0em; ".
		"margin-top: 0em; ".
		"padding-left: 1em; ".
		"text-indent: -1em;";

$Level3Style = "$margin ".
		"text-align: justify; ".
		"font-size: xx-small; ".
		"margin-bottom: 0em; ".
		"margin-top: 0em; ".
		"padding-left: 2em; ".
		"text-indent: -1em;";

$TitleStyle = "text-align: center; font-weight: bold; $margin";

$SubTitleStyle = "text-align: center; font-size: x-small; $margin";


echo '<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01//EN"'.
     '"http://www.w3.org/TR/html4/strict.dtd">'.
    '<html>'.
    '<head>'.
    '<meta http-equiv="Content-Type" content="text/html; charset=utf-8">';
    '<title>'.
    '</title>'.
    '</head>'.
    '<body>'."\n";


$linenr = 0;

foreach ( $eulaarray AS $line )
{
    $linearray = explode( " ", $line );
    $style = $DefaultStyle;

    if ( $linenr==0 )
    {
	$style = $TitleStyle;
	echo '<title>'.$line.'</title></head><body>'."\n\n\n\n";
    }

    else if ( $linenr==1 )
	$style = $SubTitleStyle;

    else if ( preg_match( "/[0-9]+\./m", $linearray[0] ) == 1 )
	$style = $Level1Style;
    else if ( preg_match( "/[A-Z]+\.+/m", $linearray[0] ) == 1 )
	$style = $Level2Style;
    else if ( preg_match( "/[a-z]+\.+/m", $linearray[0] ) == 1 )
	$style = $Level3Style;

    echo '<p style="'.$style.'">'.htmlspecialchars($line)."</p>\n";

    $linenr++;
}

echo "\n\n\n\n\n</body></html>\n";
