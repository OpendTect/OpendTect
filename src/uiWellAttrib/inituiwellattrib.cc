/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Nov 2010
________________________________________________________________________

-*/
static const char* rcsID = "$Id: inituiwellattrib.cc,v 1.2 2011-08-23 06:54:12 cvsbert Exp $";

#include "inituiwellattrib.h"
#include "uistratlayermodel.h"

void uiWellAttrib::initStdClasses()
{
    mIfNotFirstTime( return );

    uiStratLayerModel::initClass();
}
