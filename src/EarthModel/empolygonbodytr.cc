/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          May 2002
________________________________________________________________________

-*/
static const char* rcsID = "$Id$";

#include "embodytr.h"
#include "empolygonbody.h"
#include "emsurfaceio.h"
#include "iopar.h"
#include "settings.h"


const IOObjContext& polygonEMBodyTranslator::getIOObjContext()
{
    static IOObjContext* res = 0;
    if ( !res )
    {
	res = new IOObjContext(EMBodyTranslatorGroup::ioContext() );
	res->deftransl = polygonEMBodyTranslator::sKeyUserName();
	res->toselect.allowtransls_ = polygonEMBodyTranslator::sKeyUserName();
    }

    return *res;
}


polygonEMBodyTranslator::polygonEMBodyTranslator( const char* unm,
						  const char* nm )
    : Translator( unm, nm )
{}


polygonEMBodyTranslator::~polygonEMBodyTranslator()
{}


const char* polygonEMBodyTranslator::errMsg() const
{
    return errmsg_.str();
}


Executor* polygonEMBodyTranslator::reader( const IOObj& ioobj,
					   EM::PolygonBody& pb )
{
    EM::dgbSurfaceReader* rd = new EM::dgbSurfaceReader( ioobj,userName() );
    if ( !rd->isOK() )
    {
	std::cout<<"\n"<<rd->message();
	delete rd;
	return 0;
    }

    rd->setOutput( pb );
    rd->setReadOnlyZ( false );

    return rd;
}


Executor* polygonEMBodyTranslator::writer( const EM::PolygonBody& pb,
					   IOObj& ioobj )
{
    bool isbinary = true;
    mSettUse(getYN,"dTect.Surface","Binary format",isbinary);

    EM::dgbSurfaceWriter* res =
	new EM::dgbSurfaceWriter( &ioobj, userName().buf(), pb, isbinary );
    res->setWriteOnlyZ( false );

    return res;
}
