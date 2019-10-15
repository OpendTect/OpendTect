/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A. Huck
 * DATE     : Jun 2019
-*/

#include "testprog.h"

#include "applicationdata.h"
#include "arrayndimpl.h"
#include "datainterp.h"
#include "hostdata.h"
#include "netreqconnection.h"
#include "netreqpacket.h"
#include "netserver.h"
#include "netsocket.h"
#include "od_ostream.h"
#include "statrand.h"



static BufferString packetString( const char* start,
				  const Network::RequestPacket* packet )
{
    if ( !packet )
	return BufferString::empty();

    BufferString ret( start );
    ret.add( " Request " ).add( packet->requestID() )
       .add( " SubID " ).add( packet->subID() );

    return ret;
}



mClass(Network) Tester : public CallBacker
{
public:
    ~Tester()
    {
	detachAllNotifiers();
	CallBack::removeFromThreadCalls( this );
    }

    void runEventLoopTest(CallBacker*)
    {
	const bool mUnusedVar res = runTest( false );
	ApplicationData::exit( res ? 0 : 1 );
    }


    bool testStringSearch()
    {
	PtrMan<Network::RequestConnection> conn =
	    new Network::RequestConnection( hostname_, port_, false );
	mRunStandardTestWithError( conn->isOK(),
		BufferString( "Connection is OK"), toString(conn->errMsg()) );

	RefMan<Network::RequestPacket> packet = new Network::RequestPacket;
	packet->setIsNewRequest();

	OD::JSON::Object request;
	request.set( "action", "search" );
	request.set( "value", "morpheus" );
	packet->setPayload( request );
	mRunStandardTestWithError( conn->sendPacket( *packet ),
				   packetString( "Sending", packet ),
				   toString(conn->errMsg()) );

	RefMan<Network::RequestPacket> receivedpacket =
				conn->pickupPacket( packet->requestID(), 2000 );
	mRunStandardTestWithError( receivedpacket.ptr() != nullptr,
				   packetString( "Receiving", receivedpacket ),
				   toString(conn->errMsg()) );
	OD::JSON::Object response;
	const uiRetVal uirv = receivedpacket->getPayload( response );
	mRunStandardTestWithError( uirv.isOK(),
				   "Reading response", "Failed" );
	BufferString res( response.getStringValue("result") );
	mRunStandardTest(
		res == "Follow the white rabbit. \xf0\x9f\x90\xb0",
		       BufferString("Testing answer: ",res) )
	return true;
    }

    bool testSingleArrayPacket( const ArrayND<float>& arr3 )
    {
	PtrMan<Network::RequestConnection> conn =
	    new Network::RequestConnection( hostname_, port_, false );
	mRunStandardTestWithError( conn->isOK(),
		BufferString( "Connection is OK"), toString(conn->errMsg()) );

	RefMan<Network::RequestPacket> packet = new Network::RequestPacket;
	packet->setIsNewRequest();
	packet->setPayload( arr3 );
	mRunStandardTestWithError( conn->sendPacket( *packet ),
				   packetString( "Sending", packet ),
				   toString(conn->errMsg()) );

	RefMan<Network::RequestPacket> receivedpacket =
				conn->pickupPacket( packet->requestID(), 2000 );
	mRunStandardTestWithError( receivedpacket.ptr() != nullptr,
				   packetString( "Receiving", receivedpacket ),
				   toString(conn->errMsg()) );

	uiRetVal uirv;
	PtrMan<ArrayND<float> > arr3ret =
				receivedpacket->getPayload<float>( uirv );
	mRunStandardTest( mIsEqual(arr3.getData()[0]* 10.f,
				   arr3ret->getData()[0],1e-3f),
				   "[Single] Array3D operation")

	return true;
    }

    bool testMultiArrayPacket( const ArrayND<double>& arr1,
			       const ArrayND<od_uint16>& arr2,
			       const ArrayND<float>& arr3 )
    {
	PtrMan<Network::RequestConnection> conn =
	    new Network::RequestConnection( hostname_, port_, false );
	mRunStandardTestWithError( conn->isOK(),
		BufferString( "Connection is OK"), toString(conn->errMsg()) );

	ObjectSet<ArrayNDInfo> infos;
	infos.add( (ArrayNDInfo*)&arr1.info() );
	infos.add( (ArrayNDInfo*)&arr2.info() );
	infos.add( (ArrayNDInfo*)&arr3.info() );
	TypeSet<OD::DataRepType> types;
	types += OD::F64;
	types += OD::UI16;
	types += OD::F32;

	RefMan<Network::RequestPacket> packet = new Network::RequestPacket;
	packet->setIsNewRequest();
	PtrMan<Network::PacketFiller> filler = packet->setPayload(infos,types);
	if ( !filler )
	    return false;
	filler->put( arr1, true );
	filler->put( arr2, true );
	filler->put( arr3, true );
	mRunStandardTestWithError( conn->sendPacket( *packet ),
				   packetString( "Sending", packet ),
				   toString(conn->errMsg()) );

	RefMan<Network::RequestPacket> receivedpacket =
				conn->pickupPacket( packet->requestID(), 2000 );
	mRunStandardTestWithError( receivedpacket.ptr() != nullptr,
				   packetString( "Receiving", receivedpacket ),
				   toString(conn->errMsg()) );

	ObjectSet<ArrayNDInfo> retinfos;
	TypeSet<OD::DataRepType> rettypes;
	PtrMan<Network::PacketInterpreter> interpreter =
			    receivedpacket->getPayload( retinfos, rettypes );
	ObjectSet<ArrayND<float> > arrs;
	for ( int idx=0; idx<retinfos.size(); idx++ )
	{
	    ArrayND<float>* arrret =
			(ArrayND<float>*) getArrayND( *retinfos.get(idx),
						      rettypes[idx] );
	    arrs.add( arrret );
	    if ( !arrret || !arrret->isOK() )
	    {
		deepErase( retinfos );
		deepErase( arrs );
		return false;
	    }

	    interpreter->getArr( *arrret, true );
	}
	deepErase( retinfos );

	mRunStandardTest( mIsEqual(arr1.getData()[0]* 3.f,
				   arrs[0]->getData()[0],1e-3f),
				   "[Multi] Array1D operation")
	mRunStandardTest( mIsEqual(arr2.getData()[0]* 4,
				   arrs[1]->getData()[0],1e-3f),
				   "[Multi] Array2D operation")
	mRunStandardTest( mIsEqual(arr3.getData()[0]* 10.f,
				   arrs[2]->getData()[0],1e-3f),
				   "[Multi] Array3D operation")
	deepErase( arrs );

	return true;
    }

    bool runTest( bool sendkill )
    {
	od_cout() << "Connecting to: " << port_ << "@" << hostname_ << od_endl;

	if ( !testStringSearch() )
	    return false;

	Array1DImpl<double> arr1( 15 );
	Array2DImpl<od_uint16> arr2( 4, 3 );
	Array3DImpl<float> arr3( 3, 6, 9 );
	for ( int idx=0; idx<arr1.totalSize(); idx++ )
	    arr1.getData()[idx] = Stats::randGen().get();
	for ( int idx=0; idx<arr2.totalSize(); idx++ )
	    arr2.getData()[idx] = mCast(od_uint16,Stats::randGen().get()*1000.);
	for ( int idx=0; idx<arr3.totalSize(); idx++ )
	    arr3.getData()[idx] = mCast(float,Stats::randGen().get());

	/* Work done by the python server:
	   Remove the last sample of each ArrayND dimension
	   Multiply the values by 3, 4, 10 respectively
	   Cast to float
	   Thus the returned arrays have different dimensions, values and types
	*/

	return testSingleArrayPacket(arr3) &&
	       testMultiArrayPacket(arr1,arr2,arr3);
    }

    BufferString		hostname_ = GetLocalHostName();
    unsigned short		port_ = 65432;
};


int mTestMainFnName(int argc, char** argv)
{
    mInitTestProg();
    ApplicationData app;
    const char* portkey = Network::Server::sKeyPort();

    clParser().setKeyHasValue( "host" );
    clParser().setKeyHasValue( portkey );

    PtrMan<Tester> runner = new Tester;
    clParser().getKeyedInfo( "host", runner->hostname_ );
    clParser().getKeyedInfo( portkey, runner->port_ );

    CallBack::addToMainThread( mCB(runner,Tester,runEventLoopTest) );
    const int retval = app.exec();
    runner = 0;

    return retval;
}
