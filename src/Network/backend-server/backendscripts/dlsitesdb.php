<?php

include_once( 'dlsitessettings.php' );


function connect_dlsitesdb()
{
    global $DLSITES_DB, $DLSITES_DB_HOST, $DLSITES_DB_USER, $DLSITES_DB_PW;

    $mysqli = new mysqli( $DLSITES_DB_HOST, $DLSITES_DB_USER, $DLSITES_DB_PW );

    if ($mysqli->connect_errno) {
	printf("Connect failed: %s\n", $mysqli->connect_error);
	exit();
    }

    //Try to select database, and create it if missing
    if ( $mysqli->select_db( $DLSITES_DB )===false ) {
	//Try to create it if it missing
	$result = $mysqli->query( "CREATE DATABASE $DLSITES_DB");
	if ( $mysqli->select_db( $DLSITES_DB )===false ) {
	    printf( "Cannot select or create database $DLSITES_DB" );
	    exit(1);
	}
    }

    return $mysqli;
}

