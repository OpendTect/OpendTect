<?php

/*+
  (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
  AUTHOR   : K. Tingdahl
  DATE     : May 2016

-*/

include_once( 'dlsitessettings.php' );

/* Get a system id for the machine with the given profile where:

a) mac-address hash and platform are identical
   If same mac address comes with a different platform, they should be 
   treated as two machines
b) The number of on the record is lower or equal to current cpu and memory.
   Hence, if the machine is upgraded with a new cpu and more memory, treat
   it as the same machine (and upgrade the new record to the new cpu & mem.
   If the machine on the record has MORE memory than the one we are searching
   for, treat it as an different machine and add a new record.

If $updatedb is false, no updates will be done to the database.

Returns 64 bit ID wich is unique.
*/


function resolve_system_id( $mysqli, $machash, $platform, $cpu, $memory, $updatedb )
{
    global $DLSITES_TABLE_PREFIX;
    $tablename = $DLSITES_TABLE_PREFIX."system_ids";
    $query = "CREATE TABLE IF NOT EXISTS `$tablename` ("
	."`id` BIGINT UNSIGNED UNIQUE KEY, "
	."`machash` BIGINT UNSIGNED, "
	."`platform` VARCHAR( 6 ) NOT NULL, "
	."`memory` BIGINT UNSIGNED NOT NULL, "
	."`cpu` INT( 8 ) UNSIGNED NOT NULL, "
	." PRIMARY KEY (`id` ), "
	." INDEX `largeindex` (`machash`,`platform`,`cpu`,`memory`)"
	.") ENGINE = INNODB";

    if ( $mysqli->query( $query ) === false )
    {
        echo "Cannot create table $tablename\n";
        return $machash;
    }

    //Check if machash exists already
    $query = "SELECT `id`, `platform`, `memory`, `cpu` FROM `$tablename` ".
	     "WHERE `machash` = '$machash' ".
	     "AND `platform` = '$platform' ".
	     "AND `cpu` <= '$cpu' ".
	     "AND `memory` <= '$memory' ";

    $retval = $machash;
    if ($result = $mysqli->query($query))
    {
        if ( $result->num_rows>0 )
        {
	    $candidates = array();
	    while($obj = (array) $result->fetch_object())
		array_push( $candidates, $obj );

	    $arraykey = -1;
	    if ( $result->num_rows==1 )
	    {
		$arraykey = 0;
	    }
	    else
	    {
		//Of all candidate machines, found one that the resembles the most
		//and update that one.
		$bestscore = 10000000; //Very large
		$arraykey = 0;
		foreach ( $candidates AS $key => $candidate )
		{
		    $score = abs($cpu-$candidate['cpu'])*1000 + abs($memory-$candidate['memory']);
		    if ( $score<$bestscore )
		    {
			$bestscore = $score;
			$arraykey = $key;
		    }
		}
	    }

	    $result->close();

	    if ( $updatedb )
	    {
		if ( $candidates[$arraykey]['cpu'] != $cpu ||
		     $candidates[$arraykey]['memory']!= $memory )
		{
		    $query = "UPDATE `$tablename` SET `cpu` = $cpu, `memory` = $memory WHERE `id` = ".$candidates[$arraykey]['id'];
		    if ( !$mysqli->query($query) )
		    {
			echo "Cannot update record ".$candidates[$arraykey]."\n";
		    }
		}
	    }

	    $retval = $candidates[$arraykey]['id'];
	}
	else if ( $updatedb )
	{
	    $result->close(); 
	    $id = $machash;
	    

	    /* Store the new entity. Find an id by using the machash, and if that is
	       taken (i.e. the insert will fail), increment or decrements until
	       one is found. */

	    /* If id is large, decrement, otherwise increment */
	    $nrtries = 100000;
	    $operator = $id>$nrtries ? " - " : " + ";

	    for ( $idx=0; $idx<$nrtries; $idx++ )
	    {
		//As php cannot handle adding large numbers, let sql handle id+idx or id - idx
		$query = "INSERT INTO `$tablename` (`id`,`machash`,`platform`,`memory`,`cpu` ) "
			."VALUES ( $id $operator $idx, '$machash', '$platform', $memory, $cpu )"; 

		$result = $mysqli->query($query);

		if ( $result )
		    break;
	    }

	    if ( !$result )
	    {
		echo "Cannot insert system into $tablename\n";
	    }
	    else
	    {
		$retval = $id;
	    }
	}
    }

    return $retval;
}


?>

