/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        R. K. Singh
 Date:          Aug 2007
________________________________________________________________________

-*/

static const char* rcsID = "$Id: uicreatepicks.cc,v 1.3 2007-08-22 05:34:10 cvsraman Exp $";

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
#include "survinfo.h"

static int sLastNrPicks = 500;
static const char* sGeoms3D[] = { "Cube", "On Horizon", "Between Horizons", 0 };
static const char* sGeoms2D[] = { "Z Range", "Between Horizons", 0 };

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
	randhorselfld_ = new uiLabeledComboBox( randgrp_, "Horizon selection" );
	randhorselfld_->box()->addItem( "Select" );
	randhorselfld_->box()->addItems( hornms_ );
	randhorselfld_->box()->selectionChanged.notify(mCB(this,
		    					uiCreatePicks,hor1Sel));
	randhorsel2fld_ = new uiComboBox( randgrp_, "" );
	randhorsel2fld_->addItem( "Select" );
	randhorsel2fld_->addItems( hornms_ );
	randhorsel2fld_->selectionChanged.notify( mCB(this,
		    				 uiCreatePicks,hor2Sel) );
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
    if ( hornms.size() )
    {
	randvolfld_ = new uiGenInput( randgrp_, "Geometry",
				     StringListInpSpec(sGeoms3D) );
	randvolfld_->attach( alignedBelow, randnrfld_ );
	randvolfld_->valuechanged.notify( mCB(this,uiCreatePicks,randHorSel) );
    }

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
	randhorselfld_->attach( alignedBelow, randvolfld_ );
	randhorsel2fld_->attach( rightOf, randhorselfld_ );
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

    const int randgeomtyp = randvolfld_->getIntValue();
    const bool randvol = randgeomtyp == 0;
    randhorsubselfld_->display( !randvol );
    randvolsubselfld_->display( randvol );
    randhorselfld_->display( !randvol );
    randhorsel2fld_->display( randgeomtyp == 2 );
}


void uiCreatePicks3D::mkRandPars()
{
    randpars_.nr_ = randnrfld_->getIntValue();
    randpars_.iscube_ = !randvolfld_ || randvolfld_->getIntValue() == 0;

    const HorSampling& hs =
	(randpars_.iscube_ ? randvolsubselfld_ : randhorsubselfld_)
			->getInput().cs_.hrg;
    randpars_.bidrg_.start = hs.start; randpars_.bidrg_.stop = hs.stop; 
    randpars_.bidrg_.setStepOut( hs.step );

    if ( randpars_.iscube_ )
	randpars_.zrg_ = randvolsubselfld_->getInput().cs_.zrg;
    else
    {
	randpars_.horidx_ = hornms_.indexOf( randhorselfld_->box()->text() );
	randpars_.horidx2_ = -1;
	if ( randvolfld_->getIntValue() == 2 )
	    randpars_.horidx2_ = hornms_.indexOf( randhorsel2fld_->text() );
    }
}


#define mErrRet(s) { uiMSG().error(s); return false; }

bool uiCreatePicks3D::acceptOK( CallBacker* )
{
    if ( genRand() )
    {
	const int choice = randvolfld_ ? randvolfld_->getIntValue() : 0;
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

    sLastNrPicks = randpars_.nr_;
    return true;
}


uiCreatePicks2D::uiCreatePicks2D( uiParent* p, const BufferStringSet& hornms,
       				  const BufferStringSet& lsets,
				  const TypeSet<BufferStringSet>& lnms )
    : uiCreatePicks(p,hornms)
    , linenms_(lnms)
{
    linesetfld_ = new uiGenInput( randgrp_, "Line Set",
	    			  StringListInpSpec(lsets) );
    linesetfld_->attach( alignedBelow, randnrfld_ );
    linesetfld_->valuechanged.notify( mCB(this,uiCreatePicks2D,lineSetSel) );

    linenmfld_ = new uiLabeledListBox( randgrp_, lnms[0], "Select Lines", true);
    linenmfld_->attach( alignedBelow, linesetfld_ );

    if ( hornms.size() )
    {
	randvolfld_ = new uiGenInput( randgrp_, "Geometry",
				     StringListInpSpec(sGeoms2D) );
	randvolfld_->attach( alignedBelow, linenmfld_ );
	randvolfld_->valuechanged.notify( mCB(this,uiCreatePicks,randHorSel) );
	randhorselfld_->attach( alignedBelow, randvolfld_ );
	randhorsel2fld_->attach( rightOf, randhorselfld_ );
    }

    BufferString zlbl = "Z Range";
    zlbl += SI().zIsTime() ? "(milliseconds)" : SI().zInFeet() ? "(ft)"
							       : "{metres)";
    zfld_ = new uiGenInput( randgrp_, zlbl, FloatInpIntervalSpec() );
    if ( randvolfld_ ) zfld_->attach( alignedBelow, randvolfld_ );
    else zfld_->attach( alignedBelow, linenmfld_ );

    finaliseStart.notify( mCB(this,uiCreatePicks,randSel) );
}


void uiCreatePicks2D::randHorSel( CallBacker* cb )
{
    if ( !randvolfld_ ) return;

    const int randgeomtyp = randvolfld_->getIntValue();
    const bool needhor = randgeomtyp == 1;
    if ( needhor )
    {	
	uiMSG().error( "This feature is not implemented yet" );		//TODO
	randvolfld_->setValue( 0 ); return;
    }

    zfld_->display( !needhor );
    randhorselfld_->display( needhor );
    randhorsel2fld_->display( needhor );
}


void uiCreatePicks2D::lineSetSel( CallBacker* cb )
{
    const int setidx = linesetfld_->getIntValue();
    linenmfld_->box()->empty();
    if ( setidx<0 || setidx>=linenms_.size() ) return;

    linenmfld_->box()->addItems( linenms_[setidx] );
}


void uiCreatePicks2D::mkRandPars()
{
    randpars_.nr_ = randnrfld_->getIntValue();
    randpars_.iscube_ = !randvolfld_ || randvolfld_->getIntValue() == 0;

    randpars_.lsetidx_ = linesetfld_->getIntValue();
    linenmfld_->box()->getSelectedItems( randpars_.linenms_ );
    if ( randpars_.iscube_ )
    {
	randpars_.zrg_ = zfld_->getFInterval();
	if ( SI().zIsTime() ) randpars_.zrg_.scale( 0.001 );
    }
    else
    {
	randpars_.horidx_ = hornms_.indexOf( randhorselfld_->box()->text() );
	randpars_.horidx2_ = hornms_.indexOf( randhorsel2fld_->text() );
    }
}


#define mErrRet(s) { uiMSG().error(s); return false; }

bool uiCreatePicks2D::acceptOK( CallBacker* )
{
    if ( genRand() )
    {
	const int choice = randvolfld_ ? randvolfld_->getIntValue() : 0;
	if ( choice )
	{
	    if ( !strcmp(randhorselfld_->box()->text(),"Select") )
		mErrRet( "Please Select a valid horizon" );
	    if ( !strcmp(randhorsel2fld_->text(),"Select") )
		mErrRet( "Please Select a valid second horizon" );
	}

	mkRandPars();
    }

    BufferString res = getName();
    char* ptr = res.buf();
    skipLeadingBlanks(ptr); removeTrailingBlanks(ptr);
    if ( ! *ptr ) mErrRet( "Please enter a name" )

    sLastNrPicks = randpars_.nr_;
    return true;
}
