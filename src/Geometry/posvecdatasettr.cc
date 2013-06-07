/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Jul 2005
-*/

static const char* rcsID = "$Id$";

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
