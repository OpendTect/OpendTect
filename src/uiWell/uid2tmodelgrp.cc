/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nanne Hemstra
 Date:		August 2006
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uid2tmodelgrp.cc,v 1.5 2008-12-24 14:11:26 cvsbert Exp $";

#include "uid2tmodelgrp.h"

#include "uifileinput.h"
#include "uigeninput.h"
#include "uilabel.h"

#include "ctxtioobj.h"
#include "survinfo.h"

uiD2TModelGroup::uiD2TModelGroup( uiParent* p, bool withunit, const char* lbl )
    : uiGroup(p,"Depth to Time Model")
    , unitfld_(0)
{
    BufferString filefldlbl( lbl && *lbl ? lbl : "Depth to Time model file" );
    filefld_ = new uiFileInput( this, filefldlbl,
				uiFileInput::Setup().withexamine(true) );
    filefld_->setDefaultSelectionDir(
			IOObjContext::getDataDirName(IOObjContext::WllInf) );

    tvdfld_ = new uiGenInput( this, "Depth in D2T model file is",
			      BoolInpSpec(true,"TVDSS","MD") );
    tvdfld_->setValue( false );
    tvdfld_->attach( alignedBelow, filefld_ );
    uiLabel* uilbl = new uiLabel( this,
	    	"(TVDSS won't work with horizontal sections)" );
    uilbl->attach( rightOf, tvdfld_ );

    if ( withunit )
    {
	unitfld_ = new uiGenInput( this, "Depth in",
		BoolInpSpec(!SI().depthsInFeetByDefault(),"Meter","Feet") );
	unitfld_->attach( alignedBelow, tvdfld_ );
    }

    twtfld_ = new uiGenInput( this, "Time is",
		BoolInpSpec(true,"One-way","Two-way traveltime") );
    twtfld_->setValue( false );
    twtfld_->attach( alignedBelow, unitfld_ ? unitfld_ : tvdfld_ );

    setHAlignObj( filefld_ );
}


const char* uiD2TModelGroup::fileName() const
{ return filefld_->fileName(); }

bool uiD2TModelGroup::isTVD() const
{ return tvdfld_->getBoolValue(); }

bool uiD2TModelGroup::isTWT() const
{ return !twtfld_->getBoolValue(); }

bool uiD2TModelGroup::zInFeet() const
{ return unitfld_ ? !unitfld_->getBoolValue() : SI().depthsInFeetByDefault(); }
