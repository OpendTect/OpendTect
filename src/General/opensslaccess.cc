/*+
________________________________________________________________________

 Copyright:	(C) 1995-2024 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "opensslaccess.h"

#include "file.h"
#include "filepath.h"
#include "oddirs.h"
#include "oscommand.h"
#include "plugins.h"
#include "ptrman.h"
#include "separstr.h"
#include "uistringset.h"


Crypto::PassPhrase::PassPhrase( const char* str, Algorithm typ )
    : BufferString(str)
    , algo_(typ)
{
}


Crypto::PassPhrase::PassPhrase( const OD::String& str, Algorithm typ )
    : BufferString(str)
    , algo_(typ)
{
}


namespace OD
{

using encryptToFileFn = bool(*)(const OD::String&,const char*,uiRetVal&,
			const Crypto::PassPhrase&,const char*,bool,int,bool);
using decryptFromFileFn = bool(*)(const char*,BufferString&,uiRetVal&,
			const Crypto::PassPhrase&,const char*,bool,int);

class OpenSSLFnSet
{
public:

OpenSSLFnSet( const SharedLibAccess& sla )
{
    if ( !sla.isOK() )
	return;

    encrypttofilefn_ = (encryptToFileFn) sla.getFunction( "EncryptToFile" );
    decryptfromfilefn_ = (decryptFromFileFn) sla.getFunction("DecryptFromFile");
}

~OpenSSLFnSet()
{
}

encryptToFileFn encrypttofilefn_ = nullptr;
decryptFromFileFn decryptfromfilefn_ = nullptr;

};

} // namespace OD


const OD::OpenSSLAccess& OD::SSLA()
{
    static PtrMan<OpenSSLAccess> theinst = new OpenSSLAccess();
    return *theinst.ptr();
}


// OD::OpenSSLAccess

OD::OpenSSLAccess::OpenSSLAccess()
{
    loadPlugin();
}


OD::OpenSSLAccess::~OpenSSLAccess()
{
    delete functions_;
}


bool OD::OpenSSLAccess::isOK( uiRetVal* uirv ) const
{
    const bool res = sla_ && sla_->isOK();
    if ( uirv )
    {
	if ( !sla_ )
	    *uirv = tr("ODOpenSSL library is not available");
	else if ( !sla_->isOK() )
	    *uirv = toUiString( sla_->errMsg() );
    }

    return res;
}


void OD::OpenSSLAccess::loadPlugin()
{
    BufferString libnm; libnm.setMinBufSize( 32 );
    SharedLibAccess::getLibName( "ODOpenSSL", libnm.getCStr(), libnm.bufSize());
    const FilePath libfp( GetLibPlfDir(), libnm );
    if ( !libfp.exists() )
	return;

    const PluginManager::Data* pdata = PIM().findData( libfp.fileName() );
    if ( !pdata || !pdata->isloaded_ )
    {
	if ( !PIM().load(libfp.fullPath(),PluginManager::Data::AppDir,
			 PI_AUTO_INIT_EARLY) )
	    return;

	pdata = PIM().findData( libfp.fileName() );
    }

    sla_ = pdata ? pdata->sla_ : nullptr;
    if ( !sla_ || !sla_->isOK() )
	return;

    delete functions_;
    functions_ = new OpenSSLFnSet( *sla_ );
}


bool OD::OpenSSLAccess::encryptToFile( const OD::String& txt, const char* fnm,
				uiRetVal& uirv,
				const Crypto::PassPhrase& passphrase,
				int nriters,
				bool base64, const char* cipher,
				bool withsalt ) const
{
    if ( !isOK(&uirv) || !functions_ || !functions_->encrypttofilefn_ )
	return false;

    return (*functions_->encrypttofilefn_)( txt, fnm, uirv, passphrase,
					    cipher, base64, nriters, withsalt);
}


bool OD::OpenSSLAccess::decryptFromFile( const char* fnm, BufferString& res,
				uiRetVal& uirv,
				const Crypto::PassPhrase& passphrase,
				int nriters,
				bool base64, const char* cipher ) const
{
    if ( !isOK(&uirv) || !functions_ || !functions_->decryptfromfilefn_ )
	return false;

    return (*functions_->decryptfromfilefn_)( fnm, res, uirv, passphrase,
					      cipher, base64, nriters );
}


namespace System
{

static bool findLibraryPath( const char* libnm )
{
    if ( !__islinux__ )
	return false;

    const FilePath rhelsslfp( "/lib64", libnm );
    const FilePath debsslfp( "/lib/x86_64-linux-gnu", libnm );
    return rhelsslfp.exists() || debsslfp.exists();
}

} // namespace System


bool OD::OpenSSLAccess::loadOpenSSL()
{
    static int res = -1;
    if ( res >= 0 )
	return res == 1;

    if ( __iswin__ )
    {
	res = 1;
	return res == 1;
    }

#ifdef __OpenSSL_Crypto_LIBRARY__
# ifdef __OpenSSL_Crypto_PATH__
    const bool cryptook = loadOpenSSL( __OpenSSL_Crypto_LIBRARY__,
				       __OpenSSL_Crypto_PATH__, true );
# else
    const bool cryptook = loadOpenSSL( __OpenSSL_Crypto_LIBRARY__, nullptr,
					 true);
# endif
    if ( cryptook )
    {
# ifdef __OpenSSL_SSL_LIBRARY__
#  ifdef __OpenSSL_SSL_PATH__
	res = loadOpenSSL( __OpenSSL_SSL_LIBRARY__,
			   __OpenSSL_SSL_PATH__, false ) ? 1 : 0;
#  else
	res = loadOpenSSL( __OpenSSL_SSL_LIBRARY__, nullptr, false ) ? 1 : 0;
#  endif
# else
	res = 0;
# endif
    }
    else
	res = 0;
#else
    res = 0;
#endif

    return res == 1;
}


bool OD::OpenSSLAccess::loadOpenSSL( const char* libnm, const char* path,
				     bool iscrypto )
{
    static PtrMan<RuntimeLibLoader> libcryptosha, libsslsha;
    static int rescrypto = -1;
    static int resssl = -1;
    static bool isdevbuild = isDeveloperBuild();
    int& res = iscrypto ? rescrypto : resssl;
    PtrMan<RuntimeLibLoader>& libsha = iscrypto ? libcryptosha : libsslsha;
    if ( res < 0 )
    {
	if ( __islinux__ && System::findLibraryPath(libnm) )
	    res = 1;
	else
	{
	    FilePath libfp( libnm );
	    BufferString subdir;
	    if ( !__iswin__ )
	    {
		if ( isdevbuild )
		    libfp.insert( path );
		else if ( __islinux__ )
		    subdir.set( "OpenSSL" );
	    }

	    const BufferString libfnm = libfp.fullPath();
	    libsha = new RuntimeLibLoader( libfnm.buf(), subdir.buf() );
	    res = libsha && libsha->isOK() ? 2 : 0;
	}
    }

    return res > 0;
}
