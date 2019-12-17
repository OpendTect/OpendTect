<?php

/*+
  (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
  AUTHOR   : K. Tingdahl
  DATE     : May 2016
  FUNCTION : Redirections for documentation

Based on version number and module, the browser is redirected to the 
correct URL.

Syntax:

docsites.php?version=<version>&module=<module>

For example:

docsites.php?version=6.0.1&module=od_userdoc

All other arguments (but 'version' and 'module') is passed on to the redirect
url.

-*/

$versionkey = 'version';
$modulekey = 'module';
$docurl = 'http://doc.opendtect.org';

if ( array_key_exists( $versionkey, $_GET ) &&
     array_key_exists( $modulekey, $_GET ) )
{
    $version = $_GET[$versionkey];
    $module = $_GET[$modulekey];
    $version = str_replace('.', '', $version);

    //Temporary fix before website is updated.
    if ( $module=='workflows' )
	$module = 'HTML_WF';

    if ( is_numeric( $version ) )
    {
	if ( $version>=660 )
	{
	    $docurl = $docurl.'/6.6.0/doc/'.$module.'/Default.htm';
	}
	else if ( $version>=640 )
	{
	    $docurl = $docurl.'/6.4.0/doc/'.$module.'/Default.htm';
	}
	else if ( $version>=620 )
	{
	    $docurl = $docurl.'/6.2.0/doc/'.$module.'/Default.htm';
	}
	else if ( $version>=600 )
	{
	    $docurl = $docurl.'/6.0.0/doc/'.$module.'/Default.htm';
	}
	else if ( $version>=500 )
	{
	    $docurl = $docurl.'/5.0.0/doc/'.$module.'/Default.htm';
	}
    }
}

$first = true;

foreach ( $_GET as $key => $value ) 
{
    if ( $key==$versionkey )
	continue;

    if ( $key==$modulekey )
	continue;

    if ( $first )
    {
	$docurl .= "?";
	$first = false;
    }
    else
	$docurl .= "&";

    $docurl .= "$key=$value";
}

header( "Location: $docurl" ) ;

?>

