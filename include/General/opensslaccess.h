#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2024 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "generalmod.h"

#include "bufstring.h"
#include "uistring.h"

class SharedLibAccess;
class uiRetVal;

namespace Crypto
{

//!\brief A string with a hash algorithm method

mExpClass(General) PassPhrase : public BufferString
{
public:
			PassPhrase(const char*,Algorithm);
			PassPhrase(const OD::String&,Algorithm);

    Algorithm		getAlgo() const		{ return algo_; }

private:

    Algorithm		algo_;
};

} // namespace Crypto

namespace OD
{

class OpenSSLFnSet;

/*!\brief Provides access to the core ODOpenSSL API
	  Will load the ODOpenSSL plugin at runtime without
	  introducing dependencies
	  The plugin remains fully managed by the plugin manager PIM()	 */

mExpClass(General) OpenSSLAccess
{ mODTextTranslationClass(OpenSSLAccess);
public:
			OpenSSLAccess();
			~OpenSSLAccess();

    bool		isOK(uiRetVal* =nullptr) const;

    bool		encryptToFile(const OD::String&,const char* fnm,
				uiRetVal&,const Crypto::PassPhrase&,
				int nriters=sDefNrIters(),
				bool base64=sDefBase64(),
				const char* cipher=sKeyDefCipher(),
				bool withsalt=sDefUseSalt()) const;

    bool		decryptFromFile(const char* fnm,BufferString&,uiRetVal&,
				    const Crypto::PassPhrase&,
				    int nriters=sDefNrIters(),
				    bool base64=sDefBase64(),
				    const char* cipher=sKeyDefCipher()) const;

    static const char*	sKeyDefCipher()		{ return "aes-256-cbc";}
    static bool		sDefBase64()		{ return false; }
    static int		sDefNrIters()		{ return 10000; }
    static bool		sDefUseSalt()		{ return true; }

    static bool		loadOpenSSL();
			/* Loads the openssl crypto and ssl libraries,
			   but not ODOpenSSL */
private:

    void			loadPlugin();

    const SharedLibAccess* sla_ = nullptr;
    OpenSSLFnSet*		functions_ = nullptr;

    static bool		loadOpenSSL(const char* libnm,bool iscrypto);
    static bool		loadOpenSSL(const char* libnm,const char* path,
				    bool iscrypto);

};

mGlobal(General) const OpenSSLAccess& SSLA();

} // namespace OD
