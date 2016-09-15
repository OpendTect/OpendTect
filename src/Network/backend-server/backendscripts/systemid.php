<?php
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Kristofer
 Date:          Jun 2016
________________________________________________________________________

Returns the OpendTect system ID based on a fingerprint of platform,
the hashed mac address, memory and number of cors.

Syntax:
systemid.php?machash=<machash>&m=<memoryamount>&c=<nrcores>&p=<platform>

If the system fingerprint is not found previously, the fingerprint will
be stored in the database, and a new system id will be created and returned.

-*/

include_once( 'dlsitesdb.php' );
include_once( 'dlsitessystemid.php' );

header('Content-Type: text/plain');


if ( array_key_exists( 'machash', $_GET ) &&
     array_key_exists( 'p', $_GET ) &&
     array_key_exists( 'c', $_GET ) &&
     array_key_exists( 'm', $_GET ) )
{
    $mysqli = connect_dlsitesdb();
    $machash = $_GET['machash'];
    $platform = $_GET['p'];
    $nrcpu =  $_GET['c'];
    $mem =  $_GET['m'];

    echo resolve_system_id( $mysqli, $machash, $platform, $nrcpu, $mem, true );
}
else
{
    echo "Invalid entry.";
}

