/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/


#include "applicationdata.h"
#include "moddepmgr.h"
#include "networkcommon.h"
#include "netsocket.h"
#include "systeminfo.h"
#include "testprog.h"
#include "timer.h"
#include "winutils.h"


#include <QHostAddress>

static const char* localhoststr = Network::Socket::sKeyLocalHost();


class TestClass : public CallBacker
{
public:
    TestClass()
        : timer_( "starter" )
    {
        mAttachCB( timer_.tick, TestClass::timerTick );
        timer_.start( 0, true );
    }

    ~TestClass()
    {
        detachAllNotifiers();
        CallBack::removeFromThreadCalls( this );
    }

private:

    void timerTick( CallBacker* )
    {
	retval_ = testNetAuthority() && testSystemInfo() ? 0 : 1;
	CallBack::addToMainThread( mCB(this,TestClass,closeTesterCB) );
    }

    void closeTesterCB( CallBacker* )
    {
	ApplicationData::exit( retval_ );
    }

    const char* getDomainName() const
    {
	static BufferString domainnm( System::localDomainName() );
	if ( domainnm.isEmpty() )
	    domainnm.set( "enschede.dgbes.com" );
	return domainnm.buf();
    }

    bool testSystemInfo()
    {
	//Dummy test in a sense, as we cannot check the result
	mRunStandardTest( System::macAddressHash(), "macAddressHash" );

	const BufferString localaddress = System::localAddress();
	mRunStandardTest( !localaddress.isEmpty(), "Local address" );

	BufferString hostaddress = System::hostAddress( localhoststr, true );
	mRunStandardTest( hostaddress=="127.0.0.1", "localhost address ipv4" );

	hostaddress = System::hostAddress( localhoststr, false );
	mRunStandardTest( hostaddress=="::1", "localhost address ipv6" );

	const BufferString dgb29hostname( "dgb29.", getDomainName() );
	hostaddress = System::hostAddress( dgb29hostname );
	mRunStandardTest( hostaddress=="192.168.0.29", "dgb29.domain ipv4" );

	hostaddress = System::hostAddress( dgb29hostname, false );
	mRunStandardTest( !hostaddress.isEmpty(), "dgb29.domain ipv6" );

	if ( __iswin__ )
	    return true;

	hostaddress = System::hostAddress( "dgb29", true );
	mRunStandardTest( hostaddress=="192.168.0.29", "dgb29 ipv4" );

	hostaddress = System::hostAddress( "dgb29", false );
	mRunStandardTest( !hostaddress.isEmpty(), "dgb29 ipv6" );

	return true;
    }

    bool testNetAuthority()
    {
	TypeSet<QHostAddress> qaddrs;
	qaddrs += QHostAddress( QHostAddress::Null );
	qaddrs += QHostAddress( QHostAddress::Broadcast );
	qaddrs += QHostAddress( QHostAddress::LocalHost );
	qaddrs += QHostAddress( QHostAddress::LocalHostIPv6 );
	qaddrs += QHostAddress( QHostAddress::AnyIPv6 );
	qaddrs += QHostAddress( QHostAddress::AnyIPv4 );
	qaddrs += QHostAddress( QHostAddress::Any );

	static PortNr_Type port = 12345;
	TypeSet<Network::Authority> auths;
	for ( const auto& qaddr : qaddrs )
	{
	    const BufferString addr( qaddr.toString() );
	    auths += Network::Authority( addr, port );
	}

	const BufferString localhostnm( System::localHostName() );
	const BufferString localfqhn( System::localFullHostName() );
	const BufferString localipaddr( System::localAddress() );
	BufferStringSet exphostaddrs;
	exphostaddrs
	    .add( "" ).add( "255.255.255.255" )
	    .add( localhoststr ).add( localhoststr )
	    .add( localfqhn ).add( localfqhn ).add( localfqhn );
	BoolTypeSet isoks( exphostaddrs.size(), true );
	BoolTypeSet isaddrs( exphostaddrs.size(), true );
	isoks[0] = false; isaddrs[0] = false;

	for ( int idx=0; idx<auths.size(); idx++ )
	{
	    const Network::Authority& auth = auths[idx];
	    const bool isok = auth.isOK();
	    const bool isaddr = auth.isAddressBased();
	    const BufferString& exphostnm = exphostaddrs.get( idx );
	    const BufferString hostnm = auth.getHost();
	    mRunStandardTest( hostnm == exphostnm && auth.getPort() == port &&
			      isok == isoks[idx] && isaddr == isaddrs[idx],
		BufferString( "Authority for host ", exphostnm ) );
	}

	auths.setEmpty(); //Strings equivalent of these special addresses
	auths += Network::Authority( "", port );
	auths += Network::Authority( "255.255.255.255", port );
	auths += Network::Authority( "127.0.0.1", port);
	auths += Network::Authority( "::1", port );
	auths += Network::Authority( "0.0.0.0", port );
	auths += Network::Authority( "::", port );
	// QHostAddress::Any does not have a string equivalent
	auths += Network::Authority( localipaddr, port );
	exphostaddrs.last()->set( localipaddr );

	for ( int idx=0; idx<auths.size(); idx++ )
	{
	    const Network::Authority& auth = auths[idx];
	    const bool isok = auth.isOK();
	    const bool isaddr = auth.isAddressBased();
	    const BufferString& exphostnm = exphostaddrs.get( idx );
	    const BufferString hostnm = auth.getHost();
	    mRunStandardTest( hostnm == exphostnm && auth.getPort() == port &&
			      isok == isoks[idx] && isaddr == isaddrs[idx],
		BufferString( "Authority for host ", exphostnm ) );
	}

	auths.setEmpty(); isoks.setEmpty(); isaddrs.setEmpty();
	exphostaddrs.setEmpty();
	exphostaddrs.add( localfqhn ).add( localhostnm ).add( localipaddr );
	TypeSet<Network::Authority::ConnType> conntypes;
	conntypes += Network::Authority::FQDN;
	conntypes += Network::Authority::HostName;
	conntypes += Network::Authority::IPv4Address;
	for ( int idx=0; idx<exphostaddrs.size(); idx++ )
	{
	    const QHostAddress qaddr = QHostAddress( QHostAddress::Any );
	    auths += Network::Authority( BufferString(qaddr.toString()), port );
	    isoks += true;
	    isaddrs += true;
	}

	for ( int idx=0; idx<auths.size(); idx++ )
	{
	    const Network::Authority& auth = auths[idx];
	    const bool isok = auth.isOK();
	    const bool isaddr = auth.isAddressBased();
	    const BufferString& exphostnm = exphostaddrs.get( idx );
	    const BufferString hostnm = auth.getConnHost( conntypes[idx] );
	    mRunStandardTest( hostnm == exphostnm && auth.getPort() == port &&
			      isok == isoks[idx] && isaddr == isaddrs[idx],
		BufferString( "Authority for host ", exphostnm ) );
	}

	// Now hostname based authorities

	const BufferString dgb29hostname( "dgb29.", getDomainName() );
	exphostaddrs.setEmpty();
	exphostaddrs
	    .add( localhoststr )
	    .add( localhostnm )
	    .add( dgb29hostname );
	if ( !__iswin__ )
	    exphostaddrs.add( "dgb29" );

	auths.setEmpty(); isoks.setEmpty(); isaddrs.setEmpty();
	for ( const auto* exphostaddr : exphostaddrs )
	{
	    auths += Network::Authority( exphostaddr->str(), port );
	    isoks += true;
	    isaddrs += false;
	}

	for ( int idx=0; idx<auths.size(); idx++ )
	{
	    const Network::Authority& auth = auths[idx];
	    const bool isok = auth.isOK();
	    const bool isaddr = auth.isAddressBased();
	    const BufferString& exphostnm = exphostaddrs.get( idx );
	    const BufferString hostnm = auth.getHost();
	    mRunStandardTest( hostnm == exphostnm && auth.getPort() == port &&
			      isok == isoks[idx] && isaddr == isaddrs[idx],
		BufferString( "Authority for host ", exphostnm ) );
	}

	return true;
    }

    Timer	timer_;
    int		retval_ = 0;

};


static void testWinVersion()
{
#ifdef __win__

    const bool isserverwin = IsWindowsServer();
    const BufferString winver( getWinVersion() );
    const BufferString winminorver( getWinMinorVersion() );
    const BufferString fullwinver( getFullWinVersion() );
    const BufferString winbuildnr( getWinBuildNumber() );
    const BufferString windispname( getWinDisplayName() );
    const BufferString winedition( getWinEdition() );
    const BufferString winproductnm( getWinProductName() );
    const BufferString prodname( System::productName() );

    logStream() << "Windows server edition: " \
		<< (isserverwin ? "Yes" : "No") << od_newline;
    logStream() << "Windows version: " << winver.buf() << od_newline;
    logStream() << "Windows minor version: " << winminorver.buf() << od_newline;
    logStream() << "Windows full version: " << fullwinver.buf() << od_newline;
    logStream() << "Windows build number: " << winbuildnr.buf() << od_newline;
    logStream() << "Windows display name: " << windispname.buf() << od_newline;
    logStream() << "Windows edition: " << winedition.buf() << od_newline;
    logStream() << "Windows product name: " << winproductnm.buf() << od_newline;
    logStream() << "System product name: " << prodname.buf() << od_endl;

#endif
}


int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();
    ApplicationData app;
    OD::ModDeps().ensureLoaded( "Network" );

    if ( __iswin__ )
	testWinVersion();

    TestClass tester;

    return app.exec();
}
