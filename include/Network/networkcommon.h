#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A. Huck
 Date:		November 2019
________________________________________________________________________

-*/

#include "networkmod.h"

#include "bufstring.h"
#include "notify.h"
#include "uistring.h"

mFDQtclass(QString)
mFDQtclass(QHostAddress)

class CommandLineParser;
class uiRetVal;
namespace OS { class MachineCommand; }


namespace Network
{

class Socket;

enum SpecAddr { Any, IPv4, IPv6, Broadcast, LocalIPv4, LocalIPv6, None };


mGlobal(Network) PortNr_Type getUsablePort(uiRetVal&,PortNr_Type firstport =0,
					   int maxportstotry=100);
				//!<Returns 0 if none found

mGlobal(Network) PortNr_Type getUsablePort(PortNr_Type firstport=0);
mGlobal(Network) bool isPortFree(PortNr_Type port,uiString* errmsg=nullptr);
mGlobal(Network) PortNr_Type getNextCandidatePort();


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
    explicit		Authority(const BufferString& servernm);
			Authority(const char* host=nullptr,
				  PortNr_Type=0,bool resolveipv6=false);
			Authority(const Authority&);
			~Authority();

    Authority&		operator=(const Authority&);
    bool		operator==(const Authority&) const;

    bool		isOK() const;
    bool		isAddressBased() const { return hostisaddress_; }

    bool		isLocal() const { return !servernm_.isEmpty();	}
    BufferString	getServerName() const;
    SpecAddr		serverAddress() const;

    BufferString	toString() const;
    BufferString	getUserInfo() const		{ return userinfo_; }
    BufferString	getHost() const;
    enum ConnType	{ FQDN, HostName, IPv4 };
    BufferString	getConnHost(ConnType) const;
			/*<! Exclusively for network authorities from listening
			  TCP servers, returns the connection name to be used
			  for connecting to the server from other machines */
    PortNr_Type		getPort() const			{ return port_; }
    bool		addressIsValid() const;
    bool		isUsable() const;
			//<! Also checks if already in use
    bool		portIsFree(uiString* errmsg =nullptr) const;

    void		fromString(const char*,bool resolveipv6=false);
    Authority&		localFromString(const char*);
    Authority&		setUserInfo(const char*);
    Authority&		setHost(const char*,bool resolveipv6=false);
    Authority&		setPort(PortNr_Type);
    void		setFreePort(uiRetVal&);

    Authority&		setFrom(const CommandLineParser&,
				const char* defservnm=nullptr,
				const char* defhostnm=nullptr,
				PortNr_Type defport=0);

    void		addTo(OS::MachineCommand&,
			      const char* ky=nullptr) const;

    static BufferString getAppServerName(const char* nm=nullptr);

private:
    void		setHostAddress(const char*,bool resolveipv6=false);

    BufferString	userinfo_;
    PortNr_Type		port_;

    bool		hostisaddress_;
    mQtclass(QHostAddress)&	qhostaddr_;
    mQtclass(QString)&		qhost_;

    BufferString	servernm_;

    bool		hasAssignedPort() const { return port_ > 0; }


    friend class Socket;

};


} // namespace Network
