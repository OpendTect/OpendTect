/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Prajjaval Singh
 * DATE     : December 2018
-*/

#include "giswriter.h"

mImplClassFactory(GISWriter, factory);

GISWriter::GISWriter()
{}


void GISWriter::setProperties(const Property& properties)
{
    properties_ = properties;
    ispropset_ = true;
}
