<?php
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer
 Date:		Jun 2016
________________________________________________________________________

-*/

include_once( 'dlsitesdb.php' );
include_once( 'countries.php' );

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

$mysqli = connect_dlsitesdb();

$tables = array();

if ($result = $mysqli->query("SHOW TABLES LIKE '$DLSITES_TABLE_PREFIX$DLSITES_PERIOD_TABLE_PREFIX%'")) {
    $fields = $result->fetch_fields();
    while($obj = (array) $result->fetch_object()){
	array_push( $tables, $obj[$fields[0]->name] );
    }

    $result->close(); 
}

$stats = array();

foreach ( $tables AS $table )
{
    $nrsystems = 0;
    $nrrows = 0;
    $nrsystemskey = "NRSYSTEMS";
    $nrrowskey = "NRROWS";
    
    $query = "SELECT COUNT(DISTINCT `id`) AS $nrsystemskey, COUNT(*) AS $nrrowskey FROM `$table`";
    if ($result = $mysqli->query($query)) {
	while($obj = (array) $result->fetch_object()){
	    $nrsystems = $obj[$nrsystemskey];
	    $nrrows = $obj[$nrrowskey];
	}

	$result->close(); 
    }
    
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

    $stats[$table] = $tablestats;
}

$totaltablename = $DLSITES_TABLE_PREFIX.$DLSITES_PERIOD_TABLE_PREFIX."total";

// Get country list, based on total stats
$totalcountrycount = $stats[$totaltablename]['countries'];
arsort( $totalcountrycount );
$allcountries = array_keys( $totalcountrycount );

$platforms = array( "lux32", "lux64", "mac", "win32", "win64" );

echo '<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN" ';
echo '"http://www.w3.org/TR/html4/loose.dtd">'."\n";
echo "<html>\n";
echo " <head>\n";
echo '  <meta http-equiv="Content-Type" content="text/html; charset=utf-8">'."\n";
echo '  <title>OpendTect platform & usage statistics</title>'."\n";
echo '  <link rel="stylesheet" href="dlsitesstats.css" type="text/css">'."\n";
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
{
    $countrycode = ucfirst( $country );
    $countryname = $countrycode;
    if ( array_key_exists( $countrycode, $countrynames ) )
	$countryname = $countrynames[$countrycode];

    echo '    <th title="'.$countryname.'">'.$countrycode."</th>\n";
}

echo "   </tr>\n";


foreach ( $stats as $periodkey => $periodstats )
{
    echo "   <tr>\n";
    echo "    <td>".substr( $periodkey,
                            strlen( $DLSITES_TABLE_PREFIX.$DLSITES_PERIOD_TABLE_PREFIX ) )."</td>\n";
    echo "    <td>".$periodstats['nrsystems']."</td>\n";

    $nrrows = $periodstats['nrrows'];

    foreach ( $platforms as $platform )
    {
	echo "    <td>";
	if ( array_key_exists( $platform, $periodstats['platforms'] ) )
	    echo number_format($periodstats['platforms'][$platform]/$nrrows*100,1);
	echo "</td>\n";
    }

    echo "    <td>".number_format($periodstats['memavg'])."</td>\n";
    echo "    <td>".number_format($periodstats['cpuavg'],1)."</td>\n";

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

if ($result = $mysqli->query("SELECT max(`time`) AS maxtime FROM `$totaltablename`") ){ ;
    $fields = $result->fetch_fields();
    if($obj = (array) $result->fetch_object()){
       echo "  <p>Last entry in table was from ".$obj["maxtime"].".</p>\n";
    }
 
    $result->close(); 
}
 
echo '  <p>Country information is provided by <a href="http://DB-IP.com">http://DB-IP.com</a></p>'."\n";
echo " </body>\n";
echo "</html>\n";
