/*+
________________________________________________________________________

 Copyright:	(C) 1995-2024 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "file.h"
#include "filepath.h"
#include "odcommonenums.h"
#include "testprog.h"

static const BufferString string( "1a2b3c" );

// SHA-3 hashes (sha3-256 and sha3-512)
static const char* sha256hash =
	"8ddd8f4801ddc4b6eafba230d282b23134b88888e12db8c6b48f0b212f2d083b";
static const char* sha512hash =
	"9c6f7579a0257d6cf6f5c4c20476194486f975f04c8aaa6d93e4024014144331"
	"a7cb0d000adab16b97e58f7832dde17220fe476504714f1dcbe32dda52f0135b";
static Crypto::Algorithm algo1 = Crypto::Algorithm::Sha3_256;
static Crypto::Algorithm algo2 = Crypto::Algorithm::Sha3_512;


static bool testStringHash()
{
    const BufferString ret1 = string.getHash( algo1 );
    const BufferString ret2 = string.getHash( algo2 );
    mRunStandardTest( ret1 == sha256hash, "sha3-256 hash for string '1a2b3c'" );
    mRunStandardTest( ret2 == sha512hash, "sha3-512 hash for string '1a2b3c'" );

    return true;
}


static bool testFileHash()
{
    const BufferString fnm = FilePath::getTempFullPath( "test_hash", "txt" );
    od_ostream strm( fnm );
    strm.add( string.str() );
    strm.close();

    mRunStandardTest( File::getFileSize( fnm.str() ) == string.size(),
		      "Created file data" );

    const BufferString ret1 = File::getHash( fnm, algo1 );
    const BufferString ret2 = File::getHash( fnm, algo2 );
    File::remove( fnm );

    mRunStandardTest( ret1 == sha256hash,
		      "sha3-256 hash for string '1a2b3c' in file" );
    mRunStandardTest( ret2 == sha512hash,
		      "sha3-512 hash for string '1a2b3c' in file" );

    return true;
}


int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();

    if ( !testStringHash() ||
	 !testFileHash() )
	return 1;

    return 0;
}
