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


//Subdirectory where processing scripts saves processed data
$DLSITES_UNPROCESSED_DIR = 'unprocessed';

//Subdirectory where processing script stores unprocessed input files
$DLSITES_PROCESSED_DIR = 'processed';

//Subdirectory where processing scripts saves processed data
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

//IP-geography database key
$DLSITES_IP_API_KEY = '';

//IP-geography database key
$DLSITES_USE_LOCAL_IP_DB = false;

//AWS SDK location
$DLSITES_AWS_PATH = '';

//AWS archive access ID
$DLSITES_AWS_ACCESS_ID = '';

//AWS archive access key
$DLSITES_AWS_ACCESS_KEY = '';

//AWS archive path
$DLSITES_AWS_ARCHIVE_PATH = '';

//Override defaults if local file exists
$localsettingsfile = realpath(dirname(__FILE__))."/dlsitessettings.local.php";
if(file_exists( $localsettingsfile ))
{
    include_once( $localsettingsfile );
}
