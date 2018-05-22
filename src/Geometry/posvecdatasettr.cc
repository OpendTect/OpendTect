/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Jul 2005
-*/


#include "posvecdatasettr.h"
#include "posvecdataset.h"
#include "ioobj.h"


defineTranslatorGroup(PosVecDataSet,"Positioned Vector Data");
defineTranslator(od,PosVecDataSet,mdTectKey);
mDefSimpleTranslatorioContext(PosVecDataSet,Feat)
mDefSimpleTranslatorSelector(PosVecDataSet)
uiString PosVecDataSetTranslatorGroup::sTypeName( int num )
{ return tr("Positioned Vector Data", 0, num ); }


bool odPosVecDataSetTranslator::read( const IOObj& ioobj, PosVecDataSet& vds )
{
    return vds.getFrom( ioobj.mainFileName(), errmsg_ );
}


bool odPosVecDataSetTranslator::write( const IOObj& ioobj,
					const PosVecDataSet& vds )
{
    return vds.putTo( ioobj.mainFileName(), errmsg_, false );
}
