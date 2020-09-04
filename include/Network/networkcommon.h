#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A. Huck
 Date:		November 2019
 RCS:		$Id$
________________________________________________________________________

-*/

#include "networkmod.h"

#include "bufstring.h"
#include "callback.h"
#include "uistring.h"

mFDQtclass(QString)
mFDQtclass(QHostAddress)

class uiRetVal;


namespace Network
{

class Socket;

enum SpecAddr { Any, IPv4, IPv6, Broadcast, LocalIPv4, LocalIPv6, Local, None };


mGlobal(Network) PortNr_Type getUsablePort(uiRetVal&,PortNr_Type firstport =0,
					   int maxportstotry=100);
				//!<Returns 0 if none found

mGlobal(Network) PortNr_Type getUsablePort(PortNr_Type firstport=0);
mGlobal(Network) bool isPortFree(PortNr_Type port,uiString* errmsg=nullptr);
mGlobal(Network) PortNr_Type getNextCandidatePort();
mGlobal(Network) bool isLocal(SpecAddr);



/*\brief
    Network authority, containing all necessary information to connect
    to a local or remote host if that one has a listening socket
    authority	= [ userinfo "@" ] host [ ":" port ]

    See RFC 3986 for more details:
    https://www.rfc-editor.org/rfc/rfc3986.html#section-3.2

*/

mExpClass(Network) Authority
{ mODTextTranslationClass(Authority);
public:
			Authority(const char* host=nullptr,
					PortNr_Type=0,bool resolveipv6=false);
			Authority(const Authority&);
			~Authority();

    Authority&		operator=(const Authority&);
    bool		operator==(const Authority&) const;

    bool		isLocal() const { return !getServerName().isEmpty(); }
    static Authority	getLocal(const char*);
    BufferString	getServerName() const;

    BufferString	toString(bool external=false) const;
    BufferString	getUserInfo() const		{ return userinfo_; }
    BufferString	getHost(bool external=false) const;
    PortNr_Type		getPort() const			{ return port_; }
    bool		addressIsValid() const;
    bool		hasAssignedPort() const		{ return port_ > 0; }
    bool		isUsable() const;
    bool		portIsFree(uiString* errmsg =nullptr) const;

    void		fromString(const char*,bool resolveipv6=false);
    void		localFromString(const char*);
    void		setUserInfo( const char* inf )	{ userinfo_ = inf; }
    void		setHost(const char*,bool resolveipv6=false);
    void		setPort( PortNr_Type port )   { port_ = port; }
    void		setFreePort(uiRetVal&);


private:
			explicit Authority(const BufferString& servernm);

    void		setHostAddress(const char*,bool resolveipv6=false);

    BufferString	userinfo_;
    PortNr_Type		port_;

    void		setServerName(const char*);

    bool		hostisaddress_;
    mQtclass(QHostAddress)&	qhostaddr_;
    mQtclass(QString)&	qhost_;

    friend class Socket;

};


} // namespace Network

