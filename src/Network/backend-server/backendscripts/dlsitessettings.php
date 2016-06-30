<?php
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer
 Date:		Jun 2016
________________________________________________________________________

The settings here are defaults only. Best practice is to copy this file
to dlsitessettings.local.php and put the changes there.

-*/


//Subdirectory where processing scripts looks for files to process
$DLSITES_OUTPUT_DIR = 'unprocessed';

//Subdirectory where processing script stores processed file
$DLSITES_ARCHIVE_DIR = 'archive';

//Site that should be reported as download site
$DLSITES_DOWNLOAD_SITE = 'download.opendtect.org';

//Statistics database host
$DLSITES_DB_HOST = '';

//Statistics database
$DLSITES_DB = '';

//Statistics database user
$DLSITES_DB_USER = '';

//Statistics database password
$DLSITES_DB_PW = '';

//Table prefix
$DLSITES_TABLE_PREFIX = 'dlsites_';

//Override defaults if local file exists
$localsettingsfile = realpath(dirname(__FILE__))."/dlsitessettings.local.php";
if(file_exists( $localsettingsfile ))
{
    include_once( $localsettingsfile );
}
