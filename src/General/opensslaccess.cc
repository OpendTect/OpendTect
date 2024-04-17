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

static bool findLibraryPath( const char* libnm, FilePath& ret )
{
    if ( __iswin__ )
	return false;

    if ( __ismac__ )
    {
	//TODO impl
	return false;
    }

    OS::MachineCommand mc( "/sbin/ldconfig" );
    mc.addFlag( "p", OS::OldStyle ).addPipe()
      .addArg( "grep" ).addArg( libnm );

    if ( !File::exists(mc.program()) )
	return false;

    BufferString ldoutstr;
    if ( !mc.execute(ldoutstr) )
	return false;

    BufferStringSet cmdoutlines, cmdoutlines_x64;
    cmdoutlines.unCat( ldoutstr.buf() );
    for ( const auto* cmdoutline : cmdoutlines )
    {
      const SeparString cmdoutsep( cmdoutline->buf(), ' ' );
      if ( cmdoutsep.size() < 4 || !cmdoutsep[1].contains("x86-64") )
	  continue;

      const char* syslibnm = cmdoutsep[0].buf();
      if ( *syslibnm == '\t' )
	  syslibnm++;

      if ( StringView(syslibnm) != libnm )
	  continue;

      cmdoutlines_x64.add( cmdoutsep[3] );
    }

    if ( cmdoutlines_x64.isEmpty() )
	return false;

    ret.set( cmdoutlines_x64.first()->buf() );
    return ret.exists();
}


static bool canUseSystemOpenSSL( const char* libfnm, bool iscrypto,
				 FilePath& syslibfnm )
{
    if ( __iswin__ )
	return false;

    const FilePath libfp( libfnm );
    const char* libnm = libfp.fileName().buf();
    if ( !findLibraryPath(libnm,syslibfnm) )
	return false;

    if ( __ismac__ )
	return syslibfnm.exists();

    const BufferString funcnm( iscrypto ? "EVP_PKEY_param_check"
					: "EVP_PKEY_paramgen" );

    OS::MachineCommand mc( "strings" );
    mc.addArg( syslibfnm.fullPath() ).addPipe()
      .addArg( "grep" )
      .addKeyedArg( "m", 1, OS::OldStyle )
      .addArg( funcnm.str() );

    BufferString cmdoutstr;
    return mc.execute( cmdoutstr ) && !cmdoutstr.isEmpty();
}

} // namespace System


bool OD::OpenSSLAccess::loadOpenSSL( const char* libnm, bool iscrypto )
{
    static PtrMan<RuntimeLibLoader> libsha;
    static int rescrypto = -1;
    static int resssl = -1;
    int& res = iscrypto ? rescrypto : resssl;
    if ( res < 0 )
    {
	FilePath syslibfnm;
	if ( System::canUseSystemOpenSSL(libnm,iscrypto,syslibfnm) )
	{
	    libsha = new RuntimeLibLoader( syslibfnm.fullPath() );
	    res = libsha && libsha->isOK() ? 1 : 0;
	}
	else
	{
	    const BufferString subdir( __linux__ ? "OpenSSL" : "" );
	    libsha = new RuntimeLibLoader( libnm, subdir.buf() );
	    res = libsha && libsha->isOK() ? 2 : 0;
	}
    }

    return res > 0;
}
