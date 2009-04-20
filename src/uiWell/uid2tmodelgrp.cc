/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nanne Hemstra
 Date:		August 2006
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uid2tmodelgrp.cc,v 1.8 2009-04-20 13:29:58 cvsbert Exp $";

#include "uid2tmodelgrp.h"

#include "uifileinput.h"
#include "uigeninput.h"
#include "uilabel.h"

#include "ctxtioobj.h"
#include "survinfo.h"
#include "filegen.h"



uiD2TModelGroup::uiD2TModelGroup( uiParent* p, const Setup& su )
    : uiGroup(p,"D2TModel group")
    , unitfld_(0)
    , setup_(su)
{
    filefld_ = new uiFileInput( this, setup_.filefldlbl_,
				uiFileInput::Setup().withexamine(true) );
    if ( setup_.fileoptional_ )
    {
	filefld_->setWithCheck( true ); filefld_->setChecked( true );
	filefld_->checked.notify( mCB(this,uiD2TModelGroup,fileFldChecked) );
    }
    filefld_->setDefaultSelectionDir(
			IOObjContext::getDataDirName(IOObjContext::WllInf) );

    velfld_ = new uiGenInput( this, "Temporary model velocity (m/s)",
	    		      FloatInpSpec(mi_.vel_) );
    velfld_->attach( alignedBelow, filefld_ );

    tvdfld_ = new uiGenInput( this, "Depth in D2T model file is",
			      BoolInpSpec(true,"TVDSS","MD") );
    tvdfld_->setValue( false );
    tvdfld_->attach( alignedBelow, filefld_ );
    tvdsslbl_ = new uiLabel( this,
			    "(TVDSS won't work with horizontal sections)" );
    tvdsslbl_->attach( rightOf, tvdfld_ );

    if ( setup_.withunitfld_ )
    {
	unitfld_ = new uiGenInput( this, "Depth in",
		BoolInpSpec(!SI().depthsInFeetByDefault(),"Meter","Feet") );
	unitfld_->attach( alignedBelow, tvdfld_ );
    }

    twtfld_ = new uiGenInput( this, "Time is",
		BoolInpSpec(true,"TWT","One-way traveltime") );
    twtfld_->setValue( true );
    twtfld_->attach( alignedBelow, unitfld_ ? unitfld_ : tvdfld_ );

    setHAlignObj( filefld_ );
    finaliseDone.notify( mCB(this,uiD2TModelGroup,fileFldChecked) );
}

void uiD2TModelGroup::fileFldChecked( CallBacker* )
{
    if ( !setup_.fileoptional_ ) return;
    const bool doimp = filefld_->isChecked();
    velfld_->display( !doimp );
    tvdfld_->display( doimp );
    tvdsslbl_->display( doimp );
    twtfld_->display( doimp );
    if ( unitfld_ ) unitfld_->display( doimp );
}


const char* uiD2TModelGroup::checkInput() const
{
    uiD2TModelGroup& self = *const_cast<uiD2TModelGroup*>(this);
    if ( setup_.fileoptional_ && !filefld_->isChecked() )
    {
	if ( velfld_->isUndef() )
	    return "Please enter the velocity for generating the D2T model";
	self.mi_.fname_.setEmpty();
	self.mi_.vel_ = velfld_->getfValue();
	return 0;
    }

    self.mi_.fname_ = filefld_->fileName();
    if ( File_isEmpty(self.mi_.fname_) )
	return "D2T model file is not usable";
    self.mi_.istvd_ = tvdfld_->getBoolValue();
    self.mi_.istwt_ = twtfld_->getBoolValue();
    self.mi_.zinft_ = unitfld_ ? !unitfld_->getBoolValue()
			       : SI().depthsInFeetByDefault();

    if ( unitfld_ && self.mi_.zinft_ != SI().depthsInFeetByDefault() )
    {
	SI().getPars().setYN( SurveyInfo::sKeyDpthInFt(), self.mi_.zinft_ );
	SI().savePars();
    }
    return 0;
}
