<?php

include_once( 'dlsitessystemid.php' );
include_once( 'dlsitesdb.php' );

$mysqli = connect_dlsitesdb();

$mysqli->query( "DROP TABLE `od_system_ids`" );

if ( resolve_system_id( $mysqli, 1, "mac", 2, 3, true ) != 1 )
{
    echo "Test 1 failed";
    exit (1);
}

if ( resolve_system_id( $mysqli, 1, "mac", 2, 4, true ) != 1 )
{
    echo "Test 2 failed";
    exit (1);
}

if ( resolve_system_id( $mysqli, 1, "mac", 2, 3, true ) != 2 )
{
    echo "Test 3 failed";
    exit (1);
}


if ( resolve_system_id( $mysqli, 1, "win64", 2, 3, true ) != 3 )
{
    echo "Test 3 failed";
    exit (1);
}
