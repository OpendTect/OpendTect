/*+
 * AUTHOR   : A.H. Lammertink
 * DATE     : October 2003
 * FUNCTION : get special folder location
-*/

static const char* rcsID = "$Id: SearchODFile.cc,v 1.8 2010-09-29 11:50:57 cvsbert Exp $";


#include "prog.h"
#include "oddirs.h"
#include "filepath.h"
#include "file.h"

#include <iostream>

static const char* checkFile( const char* path, const char* subdir, const char* fname )
{
    if ( !path || !subdir || !fname ) return 0;

    FilePath fp( path ); fp.add( subdir );
    if ( fname && *fname )
	fp.add( fname );

    static FileNameString filenamebuf;
    strcpy( filenamebuf.buf(), fp.fullPath() );
    if ( File::exists(filenamebuf) )
	return filenamebuf;

    return 0;
}


static const char* SearchODFile( const char* fname )                                 
{
    const char* nm = checkFile( GetPersonalDir(), ".od", fname );
    if ( !nm ) nm = checkFile( GetSettingsDir(), "", fname );
    if ( !nm ) nm = checkFile( GetBaseDataDir(), "", fname );
    if ( !nm ) nm = checkFile( GetApplSetupDir(), "data", fname );
    if ( !nm ) nm = checkFile( GetSoftwareDir(0), "data", fname );
    if ( !nm ) nm = checkFile( GetApplSetupDir(), "bin", fname );
    if ( !nm ) nm = checkFile( GetSoftwareDir(0), "bin", fname );
    if ( !nm ) nm = checkFile( GetApplSetupDir(), "", fname );
    if ( !nm ) nm = checkFile( GetSoftwareDir(0), "", fname );

    return nm;
}


int main( int argc, char** argv )
{
    if ( argc < 2 )
    {
	std::cerr << "Usage: " << argv[0] << " [filename] "<< std::endl;
	return 1;
    }

    const char* result = SearchODFile( argv[1] );
    if ( !result || !*result ) return 1;

    std::cout << result << std::endl;
    return 0;
}
