#include <QCoreApplication>
#include "remcommhandler.h"

int main( int argc, char** argv )
{
    QCoreApplication app( argc, argv );
    RemCommHandler* handler = new RemCommHandler( 5050 );
    handler->listen();
    app.exec();
    delete handler;
}
