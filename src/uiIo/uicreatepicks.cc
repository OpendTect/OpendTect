/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        R. K. Singh
 Date:          Aug 2007
________________________________________________________________________

-*/

static const char* rcsID = "$Id: uicreatepicks.cc,v 1.8 2008-01-09 13:54:34 cvsbert Exp $";

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


uiCreatePicks::uiCreatePicks( uiParent* p )
    : uiDialog(p,uiDialog::Setup("Pick Set Creation",
				 "Create new PickSet",
				 "105.0.0"))
{
    nmfld_ = new uiGenInput( this, "Name for new PickSet" );
    colsel_ = new uiColorInput( this, getRandStdDrawColor(), "Color" );
    colsel_->attach( alignedBelow, nmfld_ );
}


const char* uiCreatePicks::getName() const
{
    return nmfld_->text();
}


const Color& uiCreatePicks::getPickColor()
{
    return colsel_->color();
}


#define mErrRet(s) { uiMSG().error(s); return false; } 
#define mCheckName() \
{ \
    BufferString res = getName(); \
    char* ptr = res.buf(); mTrimBlanks(ptr); \
    if ( !*ptr ) mErrRet( "Please enter a name" ) \
}

bool uiCreatePicks::acceptOK( CallBacker* )
{
    mCheckName();
    return true;
}


uiGenRandPicks::uiGenRandPicks( uiParent* p, const BufferStringSet& hornms )
	: uiCreatePicks(p)
	, geomfld_(0)
	, hornms_(hornms)
{
    nrfld_ = new uiGenInput( this, "Number of picks to generate",
					 IntInpSpec(sLastNrPicks) );
    nrfld_->attach( alignedBelow, colsel_);

    if ( hornms_.size() )
    {
	horselfld_ = new uiLabeledComboBox( this, "Horizon selection" );
	horselfld_->box()->addItem( "Select" );
	horselfld_->box()->addItems( hornms_ );
	horselfld_->box()->selectionChanged.notify(mCB(this,
		    				       uiGenRandPicks,hor1Sel));
	horsel2fld_ = new uiComboBox( this, "" );
	horsel2fld_->addItem( "Select" );
	horsel2fld_->addItems( hornms_ );
	horsel2fld_->selectionChanged.notify( mCB(this,
		    				 uiGenRandPicks,hor2Sel) );
    }
}


void uiGenRandPicks::hor1Sel( CallBacker* cb )
{    
    horSel( horselfld_->box(), horsel2fld_ );
}


void uiGenRandPicks::hor2Sel( CallBacker* cb )
{
    horSel( horsel2fld_, horselfld_->box() );
}


void uiGenRandPicks::horSel( uiComboBox* sel, uiComboBox* tosel )
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


uiGenRandPicks3D::uiGenRandPicks3D( uiParent* p, const BufferStringSet& hornms )
    : uiGenRandPicks(p,hornms)
    , horsubselfld_(0)
{
    if ( hornms.size() )
    {
	geomfld_ = new uiGenInput( this, "Geometry",
				     StringListInpSpec(sGeoms3D) );
	geomfld_->attach( alignedBelow, nrfld_ );
	geomfld_->valuechanged.notify( mCB(this,uiGenRandPicks,geomSel) );
    }

    volsubselfld_ = new uiBinIDSubSel( this, uiBinIDSubSel::Setup()
	    			.withz(true)
				.withstep(false)
				.showsurvinfo(true)
				.alltxt("From entire survey") );
    volsubselfld_->attach( alignedBelow,
	    			geomfld_ ? geomfld_ : nrfld_ );
    if ( geomfld_ )
    {
	horselfld_->attach( alignedBelow, geomfld_ );
	horsel2fld_->attach( rightOf, horselfld_ );
	horsubselfld_ = new uiBinIDSubSel( this, uiBinIDSubSel::Setup()
				.withz(false)
				.withstep(false)
				.showsurvinfo(true)
				.alltxt("From entire horizon") );
	horsubselfld_->attach( alignedBelow, horselfld_ );
    }

    finaliseStart.notify( mCB(this,uiGenRandPicks,geomSel) );
}


void uiGenRandPicks3D::geomSel( CallBacker* cb )
{
    if ( !geomfld_ ) return;

    const int geomtyp = geomfld_->getIntValue();
    const bool vol = geomtyp == 0;
    horsubselfld_->display( !vol );
    volsubselfld_->display( vol );
    horselfld_->display( !vol );
    horsel2fld_->display( geomtyp == 2 );
}


void uiGenRandPicks3D::mkRandPars()
{
    randpars_.nr_ = nrfld_->getIntValue();
    randpars_.needhor_ = geomfld_ && geomfld_->getIntValue();

    const HorSampling& hs = 
       (randpars_.needhor_ ? horsubselfld_ : volsubselfld_)->data().cs_.hrg;
    randpars_.hs_ = hs;

    if ( randpars_.needhor_ )
    {
	randpars_.horidx_ = hornms_.indexOf( horselfld_->box()->text() );
	randpars_.horidx2_ = -1;
	if ( geomfld_->getIntValue() == 2 )
	    randpars_.horidx2_ = hornms_.indexOf( horsel2fld_->text() );
    }
    else
	randpars_.zrg_ = volsubselfld_->data().cs_.zrg;
}


bool uiGenRandPicks3D::acceptOK( CallBacker* )
{
    mCheckName();
    const int choice = geomfld_ ? geomfld_->getIntValue() : 0;
    if ( choice )
    {
	if ( !strcmp(horselfld_->box()->text(),"Select") )
	    mErrRet( "Please Select a valid horizon" );
	if ( choice==2 && !strcmp(horsel2fld_->text(),"Select") )
	    mErrRet( "Please Select a valid second horizon" );
    }

    mkRandPars();
    sLastNrPicks = randpars_.nr_;
    return true;
}


uiGenRandPicks2D::uiGenRandPicks2D( uiParent* p, const BufferStringSet& hornms,
       				  const BufferStringSet& lsets,
				  const TypeSet<BufferStringSet>& lnms )
    : uiGenRandPicks(p,hornms)
    , linenms_(lnms)
{
    linesetfld_ = new uiGenInput( this, "Line Set",
	    			  StringListInpSpec(lsets) );
    linesetfld_->attach( alignedBelow, nrfld_ );
    linesetfld_->valuechanged.notify( mCB(this,uiGenRandPicks2D,lineSetSel) );

    linenmfld_ = new uiLabeledListBox( this, lnms[0], "Select Lines", true);
    linenmfld_->attach( alignedBelow, linesetfld_ );

    if ( hornms.size() )
    {
	geomfld_ = new uiGenInput( this, "Geometry",
				     StringListInpSpec(sGeoms2D) );
	geomfld_->attach( alignedBelow, linenmfld_ );
	geomfld_->valuechanged.notify( mCB(this,uiGenRandPicks,geomSel) );
	horselfld_->attach( alignedBelow, geomfld_ );
	horsel2fld_->attach( rightOf, horselfld_ );
    }

    BufferString zlbl = "Z Range";
    zlbl += SI().getZUnit();
    StepInterval<float> survzrg = SI().zRange(false);
    Interval<float> inpzrg( survzrg.start, survzrg.stop );
    inpzrg.scale( SI().zFactor() );
    zfld_ = new uiGenInput( this, zlbl, FloatInpIntervalSpec(inpzrg) );
    if ( geomfld_ ) zfld_->attach( alignedBelow, geomfld_ );
    else zfld_->attach( alignedBelow, linenmfld_ );

    finaliseStart.notify( mCB(this,uiGenRandPicks,geomSel) );
}


void uiGenRandPicks2D::geomSel( CallBacker* cb )
{
    if ( !geomfld_ ) return;

    const int geomtyp = geomfld_->getIntValue();
    const bool needhor = geomtyp == 1;
    zfld_->display( !needhor );
    horselfld_->display( needhor );
    horsel2fld_->display( needhor );
}


void uiGenRandPicks2D::lineSetSel( CallBacker* cb )
{
    const int setidx = linesetfld_->getIntValue();
    linenmfld_->box()->empty();
    if ( setidx<0 || setidx>=linenms_.size() ) return;

    linenmfld_->box()->addItems( linenms_[setidx] );
}


void uiGenRandPicks2D::mkRandPars()
{
    randpars_.nr_ = nrfld_->getIntValue();
    randpars_.needhor_ = geomfld_ && geomfld_->getIntValue();

    randpars_.lsetidx_ = linesetfld_->getIntValue();
    linenmfld_->box()->getSelectedItems( randpars_.linenms_ );
    if ( randpars_.needhor_ )
    {
	randpars_.horidx_ = hornms_.indexOf( horselfld_->box()->text() );
	randpars_.horidx2_ = hornms_.indexOf( horsel2fld_->text() );
    }
    else
    {
	randpars_.zrg_ = zfld_->getFInterval();
	randpars_.zrg_.scale( 1 / SI().zFactor() );
    }
}


bool uiGenRandPicks2D::acceptOK( CallBacker* )
{
    mCheckName(); 
    const int choice = geomfld_ ? geomfld_->getIntValue() : 0;
    if ( choice )
    {
	if ( !strcmp(horselfld_->box()->text(),"Select") )
	    mErrRet( "Please Select a valid horizon" );
	if ( !strcmp(horsel2fld_->text(),"Select") )
	    mErrRet( "Please Select a valid second horizon" );
    }
    else
    {
	Interval<float> zrg = zfld_->getFInterval();
	StepInterval<float> survzrg = SI().zRange(false);
	survzrg.scale( SI().zFactor() );
	if ( !survzrg.includes(zrg.start) || !survzrg.includes(zrg.stop) )
		mErrRet( "Please Enter a valid Z Range" );
    }

    mkRandPars();
    sLastNrPicks = randpars_.nr_;
    return true;
}
