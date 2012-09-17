/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Nov 2010
________________________________________________________________________

-*/
static const char* rcsID = "$Id: inituiwellattrib.cc,v 1.3 2011/08/23 14:51:33 cvsbert Exp $";

#include "moddepmgr.h"
#include "uistratlayermodel.h"

mDefModInitFn(uiWellAttrib)
{
    mIfNotFirstTime( return );

    uiStratLayerModel::initClass();
}
