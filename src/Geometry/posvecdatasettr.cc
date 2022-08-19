/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "posvecdatasetfact.h"
#include "posvecdataset.h"
#include "ioobj.h"

mDefSimpleTranslatorioContext(PosVecDataSet,Feat)
mDefSimpleTranslatorSelector(PosVecDataSet)


bool odPosVecDataSetTranslator::read( const IOObj& ioobj, PosVecDataSet& vds )
{
    return vds.getFrom( ioobj.fullUserExpr(true), errmsg_ );
}


bool odPosVecDataSetTranslator::write( const IOObj& ioobj,
					const PosVecDataSet& vds )
{
    return vds.putTo( ioobj.fullUserExpr(false), errmsg_, false );
}
