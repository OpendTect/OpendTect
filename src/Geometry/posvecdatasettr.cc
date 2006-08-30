/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Jul 2005
-*/

static const char* rcsID = "$Id: posvecdatasettr.cc,v 1.2 2006-08-30 16:03:27 cvsbert Exp $";

#include "posvecdatasetfact.h"
#include "posvecdataset.h"
#include "ioobj.h"

mDefSimpleTranslatorioContext(PosVecDataSet,Feat)


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
