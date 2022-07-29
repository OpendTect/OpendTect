
/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A. Huck
 * DATE     : Jun 2020
 * FUNCTION :
-*/


#include "testprog.h"

#include "iopar.h"
#include "timefun.h"
#include <iostream>
#include "odjson.h"
#include <QHash>

int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();

    // Main idea is to test things that are so basic they don't
    // really fit anywhere else.

    IOPar par;
    par.read( "/tmp/Demo_Machine_Learning_plugin_2020.sess", "Session setup" );
    std::cerr << "Read " << par.size() << std::endl;
    IOPar copy;
    Time::Counter ctr; ctr.start();
    for ( int idx=par.size()-1; idx>=0; idx-- )
	copy.set( par.getKey(idx), par.getValue(idx) );

    std::cerr << "Time Taken to create copy: " << ctr.elapsed() << std::endl;
    QHash<QString,QString> qhash;
    ctr.restart();

    for ( int idx=par.size()-1; idx>=0; idx-- )
	qhash[ par.getKey(idx).str() ] = par.getValue(idx).str();

    std::cerr << "Time Taken to create QHash: " << ctr.elapsed() << std::endl;
    std::cerr << "QHash size: " << qhash.size() << std::endl;

    ctr.restart();
    for ( int idx=0; idx<par.size(); idx++ )
	FixedString val = copy.find( par.getKey(idx) );

    std::cerr << "Time Taken to read copy: " << ctr.elapsed() << std::endl;
    ctr.restart();
    for ( int idx=0; idx<par.size(); idx++ )
	QString val = qhash[ par.getKey(idx).str() ];

    std::cerr << "Time Taken to read qhash: " << ctr.elapsed() << std::endl;

    OD::JSON::Object jsonobj;
    par.fillJSON( jsonobj,false );
    od_ostream strm( "/tmp/jsontest.json" );
    jsonobj.write( strm );

    IOPar fromjson;
    fromjson.useJSON( jsonobj );
    fromjson.write( "/tmp/fromjson.par", "par" );

    return 0;
}
