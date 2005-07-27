/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Jul 2005
-*/

static const char* rcsID = "$Id: posvecdatasettr.cc,v 1.1 2005-07-27 09:23:35 cvsbert Exp $";

#include "posvecdatasetfact.h"
#include "posvecdataset.h"
#include "ioobj.h"


const IOObjContext& PosVecDataSetTranslatorGroup::ioContext()
{
    static IOObjContext* ctxt = 0;

    if ( !ctxt )
    {
	ctxt = new IOObjContext( &theInst() );
	ctxt->crlink = false;
	ctxt->newonlevel = 1;
	ctxt->needparent = false;
	ctxt->maychdir = false;
	ctxt->stdseltype = IOObjContext::Feat;
    }

    return *ctxt;
}


int PosVecDataSetTranslatorGroup::selector( const char* key )
{
    return defaultSelector( theInst().userName(), key );
}


bool odPosVecDataSetTranslator::read( const IOObj& ioobj, PosVecDataSet& vds )
{
    return vds.getFrom( ioobj.fullUserExpr(true), errmsg_ );
}


bool odPosVecDataSetTranslator::write( const IOObj& ioobj,
					const PosVecDataSet& vds )
{
    return vds.putTo( ioobj.fullUserExpr(false), errmsg_, false );
}
