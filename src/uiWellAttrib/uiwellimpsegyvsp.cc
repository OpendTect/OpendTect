/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Jan 2009
_______________________________________________________________________

-*/
static const char* rcsID = "$Id: uiwellimpsegyvsp.cc,v 1.1 2009-01-08 15:47:48 cvsbert Exp $";

#include "uiwellimpsegyvsp.h"
#include "uilabel.h"


uiWellImportSEGYVSP::uiWellImportSEGYVSP( uiParent* p )
    : uiDialog(p,uiDialog::Setup("Import Zero-offset VSP",
				 "Import Zero-offset VSP as Well Log",
				 mTODOHelpID) )
    , bparsfld_(0)
{
    new uiLabel( this, "TODO - not implemented yet" );
}


uiWellImportSEGYVSP::~uiWellImportSEGYVSP()
{
}
