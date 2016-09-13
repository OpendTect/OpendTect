<?php
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer
 Date:		Jun 2016
________________________________________________________________________

-*/

//Directory where dlsites data should reside
$DLSITES_DATA_ROOT_DIR = '.';

//Subdirectory where processing scripts looks for files to process
$DLSITES_UNPROCESSED_DIR = 'unprocessed';

//Subdirectory where processing script stores unprocessed input file
$DLSITES_PROCESSED_DIR = 'processed';

//Subdirectory where processing scripts saves processed data
$DLSITES_ARCHIVE_DIR = 'archive';

//Subdirectory where processing scripts saves processed compressed data
$DLSITES_COMPRESSED_ARCHIVE_DIR = 'compressed_archive';

//Site that should be reported as download site
$DLSITES_DOWNLOAD_SITE = 'download.opendtect.org';

//Statistics database host
$DLSITES_DB_HOST = 'mysql-enschede';

//Statistics database
$DLSITES_DB = 'dlsites_db';

//Table prefix
$DLSITES_TABLE_PREFIX = 'dlsites_';

//Statistics database user
$DLSITES_DB_USER = 'dlsiteuser';

//Statistics database password
$DLSITES_DB_PW = 'SYceaRECCuQHcfaJ';

//IP-geography database key
$DLSITES_IP_API_KEY = '6657b916b375e9beb92a17a5f9ec0b232715bec8';

//AWS SDK location
$DLSITES_AWS_PATH = '/dsk/d46/apps/AWS-SDK';

//AWS archive access ID
$DLSITES_AWS_ACCESS_ID = 'AKIAJ44YWDPIBJHMXZ7A';

//AWS archive access key
$DLSITES_AWS_ACCESS_KEY = 'uz9VCYJ9uqUFQoz7JeZe2cKOUbgF0hMPX5gCuxNq';

//AWS archive path
$DLSITES_AWS_ARCHIVE_PATH = 's3://dlsites/test_archive';

//Override defaults if local file exists
if(file_exists('dlsitessettings.local.php'))
{
    include_once( 'dlsitessettings.local.php' );
}

