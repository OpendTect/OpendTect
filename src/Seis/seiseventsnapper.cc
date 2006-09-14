/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nanne Hemstra
 Date:		September 2006
 RCS:		$Id: seiseventsnapper.cc,v 1.1 2006-09-14 20:10:39 cvsnanne Exp $
________________________________________________________________________

-*/

#include "seiseventsnapper.h"

#include "ioobj.h"
#include "seisread.h"
#include "seisreq.h"
#include "seistrcsel.h"


SeisEventSnapper::SeisEventSnapper( const IOObj& ioobj, BinIDValueSet& bvs )
    : Executor("Snapping to nearest event")
    , positions_(bvs)
{
    req_ = new SeisRequester( &ioobj );
    req_->prepareWork();

    SeisSelData* sd = new SeisSelData;
    sd->all_ = false;
    sd->type_ = Seis::Table;
    sd->table_ = bvs;
    req_->reader()->setSelData( sd );
}


SeisEventSnapper::~SeisEventSnapper()
{
    delete req_;
}


int SeisEventSnapper::nextStep()
{
    const int res = req_->next();
    if ( res == 1 )
    {
	// get trc, get z val from binidvalueset, snap to nearest event, put new z into binidvalueset
    }

    return 1;
}
