/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Nov 2010
________________________________________________________________________

-*/
static const char* mUnusedVar rcsID = "$Id: inituiwellattrib.cc,v 1.4 2012-05-02 11:54:05 cvskris Exp $";

#include "moddepmgr.h"
#include "uistratlayermodel.h"

mDefModInitFn(uiWellAttrib)
{
    mIfNotFirstTime( return );

    uiStratLayerModel::initClass();
}
