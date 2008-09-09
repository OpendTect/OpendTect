/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Helene Huck
 Date:          August 2007
 RCS:		$Id: uistratutildlgs.cc,v 1.8 2008-09-09 10:52:11 cvsbert Exp $
________________________________________________________________________

-*/

#include "uistratutildlgs.h"

#include "randcolor.h"
#include "stratlith.h"
#include "stratunitrepos.h"
#include "uibutton.h"
#include "uicolor.h"
#include "uidialog.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "uilistbox.h"
#include "uimsg.h"
#include "uiseparator.h"
#include "uistratmgr.h"

static const char* sNoLithoTxt      = "---None---";


uiStratUnitDlg::uiStratUnitDlg( uiParent* p, uiStratMgr* uistratmgr )
    : uiDialog(p,uiDialog::Setup("Create Stratigraphic Unit","",mTODOHelpID))
    , uistratmgr_(uistratmgr)
{
    unitnmfld_ = new uiGenInput( this, "Name", StringInpSpec() );
    unitdescfld_ = new uiGenInput( this, "Description", StringInpSpec() );
    unitdescfld_->attach( alignedBelow, unitnmfld_ );
    unitlithfld_ = new uiGenInput( this, "Lithology", StringInpSpec() );
    unitlithfld_->attach( alignedBelow, unitdescfld_ );
    CallBack cb = mCB(this,uiStratUnitDlg,selLithCB);
    uiPushButton* sellithbut = new uiPushButton( this, "&Select", cb, false );
    sellithbut->attach( rightTo, unitlithfld_ );
}


void uiStratUnitDlg::selLithCB( CallBacker* )
{
    uiLithoDlg lithdlg( this, uistratmgr_ );
    if ( lithdlg.go() )
	unitlithfld_->setText( lithdlg.getLithName() );
} 


const char* uiStratUnitDlg::getUnitName() const
{
    return unitnmfld_->text();
}


const char* uiStratUnitDlg::getUnitDesc() const
{
    return unitdescfld_->text();
}


const char* uiStratUnitDlg::getUnitLith() const
{
    const char* txt = unitlithfld_->text();
    return !strcmp( txt, sNoLithoTxt ) ? 0 : txt;
}


bool uiStratUnitDlg::acceptOK( CallBacker* )
{
    if ( !strcmp( getUnitName(), "" ) )
	{ uiMSG().error( "Please specify the unit name" ); return false; }
    return true;
}


uiLithoDlg::uiLithoDlg( uiParent* p, uiStratMgr* uistratmgr )
    : uiDialog(p,uiDialog::Setup("Select Lithology","",mTODOHelpID))
    , uistratmgr_(uistratmgr)
{
    BufferStringSet lithoset;
    lithoset.add( sNoLithoTxt );
    uistratmgr_->getLithoNames( lithoset );
    
    listlithfld_ = new uiLabeledListBox( this, lithoset, "Existing lithologies",
	   				 false, uiLabeledListBox::AboveMid );
    uiGroup* rightgrp = new uiGroup( this, "right group" );
    uiLabel* lbl = new uiLabel( rightgrp,"Create new lithology with" );
    lithnmfld_ = new uiGenInput( rightgrp, "Name", StringInpSpec() );
    lithnmfld_->attach( alignedBelow, lbl );
    CallBack cb = mCB(this,uiLithoDlg,newLithCB);
    uiPushButton* newlithbut = new uiPushButton(rightgrp,"&Add as new",cb,true);
    newlithbut->attach( alignedBelow, lithnmfld_ );
    uiSeparator* sep = new uiSeparator( this, "Sep", false );
    sep->attach( rightTo, listlithfld_ );
    sep->attach( heightSameAs, listlithfld_ );
    rightgrp->attach( rightTo, sep );
}


void uiLithoDlg::newLithCB( CallBacker* )
{
    if ( listlithfld_->box()->isPresent( lithnmfld_->text() )
	 || !strcmp( lithnmfld_->text(), "" ) ) return;
    uistratmgr_->createNewLith( lithnmfld_->text() );

    listlithfld_->box()->addItem( lithnmfld_->text() );
    listlithfld_->box()->setCurrentItem( lithnmfld_->text() );
}


const char* uiLithoDlg::getLithName() const
{
    const char* txt = listlithfld_->box()->getText();
    return !strcmp( txt, sNoLithoTxt ) ? 0 : txt;
}


void uiLithoDlg::setSelectedLith( const char* lithnm )
{
    listlithfld_->box()->setCurrentItem( lithnm );
}


uiStratLevelDlg::uiStratLevelDlg( uiParent* p, uiStratMgr* uistratmgr )
    : uiDialog(p,uiDialog::Setup("Create/Edit level","",mTODOHelpID))
    , uistratmgr_( uistratmgr )
{
    lvlnmfld_ = new uiGenInput( this, "Name", StringInpSpec() );
    lvlcolfld_ = new uiColorInput( this,
			           uiColorInput::Setup(getRandStdDrawColor() ).
				   lbltxt("Color") );
    lvlcolfld_->attach( alignedBelow, lvlnmfld_ );
    lvltvstrgfld_ = new uiGenInput( this, "This level is ",
	    			    BoolInpSpec(true,"Isochron","Diachron") );
    lvltvstrgfld_->attach( alignedBelow, lvlcolfld_ );
    lvltvstrgfld_->valuechanged.notify( mCB(this,uiStratLevelDlg,isoDiaSel) );
    lvltimefld_ = new uiGenInput( this, "Level time (My)", FloatInpSpec() );
    lvltimefld_->attach( alignedBelow, lvltvstrgfld_ );
    lvltimergfld_ = new uiGenInput( this, "Level time range (My)",
	    			    FloatInpIntervalSpec() );
    lvltimergfld_->attach( alignedBelow, lvltvstrgfld_ );
    isoDiaSel(0);
}


void uiStratLevelDlg::isoDiaSel( CallBacker* )
{
    bool isiso = lvltvstrgfld_->getBoolValue();
    lvltimefld_->display( isiso );
    lvltimergfld_->display( !isiso );
}


void uiStratLevelDlg::setLvlInfo( const char* lvlnm )
{
    Interval<float> lvltrg;
    Color lvlcol;
    if ( !lvlnm || !*lvlnm || !uistratmgr_->getLvlPars(lvlnm,lvltrg,lvlcol) )
	return;

    lvlnmfld_->setText( lvlnm );
    bool isiso = mIsUdf(lvltrg.stop);
    lvltvstrgfld_->setValue( isiso );
    if ( isiso && !mIsUdf(lvltrg.start) )
	lvltimefld_->setValue( lvltrg.start );
    else if ( !isiso )
	lvltimergfld_->setValue( lvltrg );

    lvlcolfld_->setColor( lvlcol );
    oldlvlnm_ = lvlnm;
}


bool uiStratLevelDlg::acceptOK( CallBacker* )
{
    BufferString newlvlnm = lvlnmfld_->text();
    Color newlvlcol = lvlcolfld_->color();
    Interval<float> newlvlrg;
    if ( lvltvstrgfld_->getBoolValue() )
    {
	newlvlrg.start = lvltimefld_->getfValue();
	newlvlrg.stop = lvltimefld_->getfValue();
    }
    else
	newlvlrg = lvltimergfld_->getFInterval();
    
    uistratmgr_->setLvlPars( oldlvlnm_, newlvlnm, newlvlcol, newlvlrg );
    return true;
}


#define mCreateList(loc,str)\
{\
    BufferString loc##bs = "Select "; loc##bs += str; loc##bs += " level";\
    lvl##loc##listfld_ = new uiGenInput( this, loc##bs,\
	    				 StringListInpSpec( lvlnms ) );\
    lvl##loc##listfld_->setWithCheck();\
    lvl##loc##listfld_->setChecked( loc##idx>-1 );\
    if ( loc##idx>-1 )\
	lvl##loc##listfld_->setValue( loc##idx );\
}

uiStratLinkLvlUnitDlg::uiStratLinkLvlUnitDlg( uiParent* p, const char* urcode,
       					      uiStratMgr* uistratmgr )
    : uiDialog(p,uiDialog::Setup("Link levels and stratigraphic unit","",
				mTODOHelpID))
    , uistratmgr_(uistratmgr)
{
    BufferStringSet lvlnms;
    TypeSet<Color> colors; int topidx, baseidx;
    uistratmgr_->getLvlsTxtAndCol( lvlnms, colors );
    uistratmgr_->getIdxTBLvls( urcode, topidx, baseidx );
    mCreateList(top,"top")
    mCreateList(base,"base")
    lvlbaselistfld_->attach( alignedBelow, lvltoplistfld_ );
}


bool uiStratLinkLvlUnitDlg::acceptOK( CallBacker* )
{
    bool hastoplvl = lvltoplistfld_->isChecked();
    bool hasbaselvl = lvlbaselistfld_->isChecked();
    if ( hastoplvl && hasbaselvl )
    {
	BufferString msg = uistratmgr_->checkLevelsOk( lvltoplistfld_->text(),
						       lvlbaselistfld_->text());
	if ( !msg.isEmpty() ) 
	    { uiMSG().error( msg ); return false; }
    }

    return true;
}

