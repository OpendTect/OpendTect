/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Nov 2010
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id: inituiwellattrib.cc,v 1.5 2012-05-02 15:12:29 cvskris Exp $";

#include "moddepmgr.h"
#include "uistratlayermodel.h"

mDefModInitFn(uiWellAttrib)
{
    mIfNotFirstTime( return );

    uiStratLayerModel::initClass();
}
