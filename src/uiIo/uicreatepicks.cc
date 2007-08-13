/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        R. K. Singh
 Date:          Aug 2007
________________________________________________________________________

-*/

static const char* rcsID = "$Id: uicreatepicks.cc,v 1.1 2007-08-13 04:35:04 cvsraman Exp $";

#include "uicreatepicks.h"

#include "uibinidsubsel.h"
#include "uibutton.h"
#include "uicolor.h"
#include "uicombobox.h"
#include "uigeninput.h"
#include "uiioobjsel.h"
#include "uilistbox.h"
#include "uimsg.h"

#include "color.h"
#include "ctxtioobj.h"
#include "cubesampling.h"
#include "datainpspec.h"
#include "iodir.h"
#include "ioman.h"
#include "ioobj.h"
#include "multiid.h"
#include "pickset.h"
#include "picksettr.h"
#include "randcolor.h"

static int sLastNrPicks = 500;
static const char* sGeoms[] = { "Cube", "On Horizon", "Between Horizons", 0 };

uiCreatePicks::uiCreatePicks( uiParent* p, const BufferStringSet& hornms )
	: uiDialog(p,uiDialog::Setup("Pick Set Creation",
				     "Create new PickSet",
				     "105.0.0"))
	, randvolfld_(0)
	, hornms_(hornms)
{
    nmfld_ = new uiGenInput( this, "Name for new PickSet" );
    colsel_ = new uiColorInput( this, getRandStdDrawColor(), "Color" );
    colsel_->attach( alignedBelow, nmfld_ );

    randfld_ = new uiGenInput( this, "Generate random picks",
	    		      BoolInpSpec(false) );
    randfld_->attach( alignedBelow, colsel_ );
    randfld_->valuechanged.notify( mCB(this,uiCreatePicks,randSel) );

    randgrp_ = new uiGroup( this, "Random pick pars" );
    randgrp_->attach( alignedBelow, randfld_ );
    randnrfld_ = new uiGenInput( randgrp_, "Number of picks to generate",
					 IntInpSpec(sLastNrPicks) );
    randgrp_->setHAlignObj( randnrfld_ );

    if ( hornms_.size() )
    {
	randvolfld_ = new uiGenInput( randgrp_, "Geometry",
				     StringListInpSpec(sGeoms) );
	randvolfld_->attach( alignedBelow, randnrfld_ );
	randvolfld_->valuechanged.notify( mCB(this,uiCreatePicks,randHorSel) );
    }

    if ( randvolfld_ )
    {
	randhorselfld_ = new uiLabeledComboBox( randgrp_, "Horizon selection" );
	randhorselfld_->box()->addItem( "Select" );
	randhorselfld_->box()->addItems( hornms_ );
	randhorselfld_->box()->selectionChanged.notify(mCB(this,
		    					uiCreatePicks,hor1Sel));
	randhorselfld_->attach( alignedBelow, randvolfld_ );
	randhorsel2fld_ = new uiComboBox( randgrp_, "" );
	randhorsel2fld_->addItem( "Select" );
	randhorsel2fld_->addItems( hornms_ );
	randhorsel2fld_->selectionChanged.notify( mCB(this,
		    				 uiCreatePicks,hor2Sel) );
	randhorsel2fld_->attach( rightOf, randhorselfld_ );
    }
}


void uiCreatePicks::randSel( CallBacker* cb )
{
    randgrp_->display( genRand() );
    randHorSel( cb );
}


void uiCreatePicks::hor1Sel( CallBacker* cb )
{    
    horSel( randhorselfld_->box(), randhorsel2fld_ );
}


void uiCreatePicks::hor2Sel( CallBacker* cb )
{
    horSel( randhorsel2fld_, randhorselfld_->box() );
}


void uiCreatePicks::horSel( uiComboBox* sel, uiComboBox* tosel )
{
    const char* nm = sel->text();
    const char* curnm = tosel->text();
    const int idx = hornms_.indexOf( nm );
    BufferStringSet hornms( hornms_ );
    BufferString* bs = 0;
    if ( idx >= 0 ) bs = hornms.remove( idx );

    tosel->empty();
    tosel->addItem( "Select" );
    tosel->addItems( hornms );
    tosel->setCurrentItem( curnm );
    if ( bs ) delete bs;
}


const char* uiCreatePicks::getName() const
{
    return nmfld_->text();
}


const Color& uiCreatePicks::getPickColor()
{
    return colsel_->color();
}


bool uiCreatePicks::genRand() const
{
    return randfld_->getBoolValue();
}


IOObj* uiCreatePicks::storObj() const
{
    const char* nm = getName();
    if ( !nm || !*nm ) return 0;

    PtrMan<CtxtIOObj> ctio = mMkCtxtIOObj(PickSet);
    IOM().to( ctio->ctxt.getSelKey() );
    const IOObj* existioobj = (*IOM().dirPtr())[nm];
    IOObj* ret = 0;
    if ( existioobj )
    {
	BufferString msg( "Overwrite existing '" );
	msg += getName(); msg += "'?";
	if ( uiMSG().askGoOn(msg) )
	    ret = existioobj->clone();
    }
    else
    {
	ctio->setName( nm );
	IOM().getEntry( *ctio );
	ret = ctio->ioobj;
	ctio->ioobj = 0;
    }

    return ret;
}


uiCreatePicks3D::uiCreatePicks3D( uiParent* p, const BufferStringSet& hornms )
    : uiCreatePicks(p,hornms)
    , randhorsubselfld_(0)
{
    randvolsubselfld_ = new uiBinIDSubSel( randgrp_, uiBinIDSubSel::Setup()
	    			.withz(true)
				.withstep(false)
				.rangeonly(true)
				.showsurvinfo(true)
				.alltxt("From entire survey") );
    randvolsubselfld_->attach( alignedBelow,
	    			randvolfld_ ? randvolfld_ : randnrfld_ );
    if ( randvolfld_ )
    {
	randhorsubselfld_ = new uiBinIDSubSel( randgrp_, uiBinIDSubSel::Setup()
				.withz(false)
				.withstep(false)
				.rangeonly(true)
				.showsurvinfo(true)
				.alltxt("From entire horizon") );
	randhorsubselfld_->attach( alignedBelow, randhorselfld_ );
    }

    finaliseStart.notify( mCB(this,uiCreatePicks,randSel) );
}


void uiCreatePicks3D::randHorSel( CallBacker* cb )
{
    if ( !randvolfld_ ) return;

    const bool dodisp = genRand();
    const int randgeomtyp = randvolfld_->getIntValue();
    const bool randvol = randgeomtyp == 0;
    randhorsubselfld_->display( dodisp && !randvol );
    randvolsubselfld_->display( dodisp && randvol );
    randhorselfld_->display( dodisp && !randvol );
    randhorsel2fld_->display( dodisp && randgeomtyp == 2 );
}


void uiCreatePicks3D::mkRandPars()
{
    randpars_.nr = randnrfld_->getIntValue();
    randpars_.iscube = !randvolfld_ || randvolfld_->getIntValue() == 0;

    const HorSampling& hs =
	(randpars_.iscube ? randvolsubselfld_ : randhorsubselfld_)
			->getInput().cs_.hrg;
    randpars_.bidrg.start = hs.start; randpars_.bidrg.stop = hs.stop; 
    randpars_.bidrg.setStepOut( hs.step );

    if ( randpars_.iscube )
	randpars_.zrg = randvolsubselfld_->getInput().cs_.zrg;
    else
    {
	randpars_.horidx = hornms_.indexOf( randhorselfld_->box()->text() );
	randpars_.horidx2 = -1;
	if ( randvolfld_->getIntValue() == 2 )
	    randpars_.horidx2 = hornms_.indexOf( randhorsel2fld_->text() );
	if ( randpars_.horidx2 == randpars_.horidx )
	    randpars_.horidx2 = -1;
    }
}


#define mErrRet(s) { uiMSG().error(s); return false; }

bool uiCreatePicks3D::acceptOK( CallBacker* )
{
    if ( genRand() )
    {
	const int choice = randvolfld_->getIntValue();
	if ( choice )
	{
	    if ( !strcmp(randhorselfld_->box()->text(),"Select") )
		mErrRet( "Please Select a valid horizon" );
	    if ( choice==2 && !strcmp(randhorsel2fld_->text(),"Select") )
		mErrRet( "Please Select a valid second horizon" );
	}

	mkRandPars();
    }

    BufferString res = getName();
    char* ptr = res.buf();
    skipLeadingBlanks(ptr); removeTrailingBlanks(ptr);
    if ( ! *ptr ) mErrRet( "Please enter a name" )

    sLastNrPicks = randpars_.nr;
    return true;
}
