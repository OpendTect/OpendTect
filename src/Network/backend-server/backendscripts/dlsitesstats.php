<?php
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer
 Date:		Jun 2016
________________________________________________________________________

-*/

include( 'settings.php' );

function read_field_value_counts( $db, $table, $field )
{
    $retarr = array();
    $countkey = "THECOUNT";
    $query = "SELECT `$field`, count(*) AS $countkey FROM `$table` GROUP BY `$field`";
    if ($result = $db->query($query)) {
	while($obj = (array) $result->fetch_object()){
	    $retarr[$obj[$field]] = $obj[$countkey];
	}

	$result->close(); 
    }

    return $retarr;
}


function read_field_average( $db, $table, $field )
{
    $ret = -1;
    $avgkey = "THEAVG";
    $query = "SELECT AVG(`$field`) AS $avgkey FROM `$table`";
    if ($result = $db->query($query)) {
	if($obj = (array) $result->fetch_object()){
	    $ret = $obj[$avgkey];
	}

	$result->close(); 
    }

    return $ret;
}


$mysqli = new mysqli( $DLSITES_DB_HOST, $DLSITES_DB_USER, $DLSITES_DB_PW );

if ($mysqli->connect_errno) {
    printf("Connect failed: %s\n", $mysqli->connect_error);
    exit( 1 );
}

//Try to select database, and create it if missing
if ( $mysqli->select_db( $DLSITES_DB )===false ) {
    echo "Cannot select database $DLSITES_DB";
    exit( 1 );
}

$tables = array();

if ($result = $mysqli->query("SHOW TABLES")) {
    $fields = $result->fetch_fields();
    while($obj = (array) $result->fetch_object()){
	array_push( $tables, $obj[$fields[0]->name] );
    }

    $result->close(); 
}

$allcountries = array();
$stats = array();

foreach ( $tables AS $table )
{
    $nrsystems = 0;
    $nrrow = 0;
    $nrsystemskey = "NRSYSTEMS";
    $nrrowskey = "NRROWS";
    
    $query = "SELECT COUNT(DISTINCT `id`) AS $nrsystemskey, COUNT(*) AS $nrrowskey FROM `$table`";
    if ($result = $mysqli->query($query)) {
	while($obj = (array) $result->fetch_object()){
	    $nrsystems = $obj[$nrsystemskey];
	    $nrrows = $obj[$nrrowskey];
	}
    }
    
    $result->close(); 

    $platforms = read_field_value_counts( $mysqli, $table, 'platform' );
    $countries = read_field_value_counts( $mysqli, $table, 'country' );
    $cpuavg = read_field_average( $mysqli, $table, 'cpu' );
    $memavg = read_field_average( $mysqli, $table, 'memory' );
    $tablestats = array( 'platforms' => $platforms,
  			 'countries' => $countries,
			 'cpuavg' => $cpuavg,
			 'memavg' => $memavg,
			 'nrrows' => $nrrows,
			 'nrsystems' => $nrsystems );

    //Sort array in to get high values first
    arsort( $countries );

    $allcountries = array_unique( array_merge( $allcountries, array_keys( $countries ) ) );
    $stats[$table] = $tablestats;
}

$platforms = array( "lux32", "lux64", "mac", "win32", "win64" );

echo '<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN" ';
echo '"http://www.w3.org/TR/html4/loose.dtd">'."\n";
echo "<html>\n";
echo " <head>\n";
echo '  <meta http-equiv="Content-Type" content="text/html; charset=utf-8">'."\n";
echo '  <title>OpendTect platform & usage statistics</title>'."\n";
echo '  <link rel="stylesheet" href="dlsitesstats.css" type="text/css" />'."\n";
echo " </head>\n";
echo " <body>\n";

echo "  <table>\n";

//Print header
echo "   <tr>";
echo '    <th colspan="2"></th>'."\n";
echo '    <th colspan="'.count( $platforms ).'">Platforms</th>'."\n";
echo '    <th colspan="2">System stats</th>'."\n";
echo '    <th colspan="'.count( $allcountries ).'">Countries</th>'."\n";
echo "   <tr>\n";
echo "    <th>Period</th>\n";
echo "    <th>Unique systems</th>\n";
foreach ( $platforms as $platform )
    echo "    <th>$platform</th>\n";

echo "    <th>Average memory (Kb)</th><th>Average nr cpus</th>\n";

foreach ( $allcountries as $country )
    echo "    <th>".ucfirst( $country )."</th>\n";

echo "   </tr>\n";


foreach ( $stats as $periodkey => $periodstats )
{
    echo "   <tr>\n";
    echo "    <td>$periodkey</td>\n";
    echo "    <td>".$periodstats['nrsystems']."</td>\n";

    $nrrows = $periodstats['nrrows'];

    foreach ( $platforms as $platform )
    {
	echo "    <td>";
	echo number_format($periodstats['platforms'][$platform]/$nrrows*100,1);
	echo "</dh>\n";
    }

    echo "    <td>".$periodstats['memavg']."</td>\n";
    echo "    <td>".$periodstats['cpuavg']."</td>\n";

    foreach ( $allcountries as $country )
    {
	echo "    <td>";
        if ( array_key_exists( $country, $periodstats['countries'] ) )
	{
	    echo number_format($periodstats['countries'][$country]/$nrrows*100,1);
	}
	echo "</td>\n";
    }
    echo "   </tr>\n";
}

echo "  </table>\n";
echo " </body>\n";
echo "</html>\n";







