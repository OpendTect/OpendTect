/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Helene Huck
 Date:		March 2012
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";


#include "uiwelllogcalcinpdata.h"

#include "uitoolbutton.h"
#include "uimsg.h"
#include "uicombobox.h"
#include "uiwelllogdisplay.h"
#include "uiunitsel.h"

#include "welllogset.h"
#include "welllog.h"
#include "unitofmeasure.h"


static const char* specvararr[] = { "MD", "DZ", "TVDSS", "TVD", "TVDSD", "TWT",
				    "VINT", 0 };
static const BufferStringSet specvars( specvararr );

uiWellLogCalcInpData::uiWellLogCalcInpData( uiWellLogCalc* p, uiGroup* inpgrp,
					    int fieldnr )
    : uiMathExpressionVariable(inpgrp,p->lognms_,fieldnr)
    , wls_(&p->wls_)
    , convertedlog_(0)
{
    uiToolButton* tb = new uiToolButton( this, "view_log", "Display this log",
				mCB(this,uiWellLogCalcInpData,vwLog) );
    tb->attach( rightOf, inpfld_ );
    unfld_->attach( ensureRightOf, tb );

    udfbox_ = new uiCheckBox( this, "Fill empty sections" );
    udfbox_->setChecked();
    if ( unfld_ )
	udfbox_->attach( rightOf, unfld_ );
    else
	udfbox_->attach( rightOf, inpfld_ );
}


uiWellLogCalcInpData::~uiWellLogCalcInpData()
{
    delete convertedlog_;
}


const Well::Log* uiWellLogCalcInpData::getLog()
{
    return wls_->getLog( inpfld_->box()->text() );
}


bool uiWellLogCalcInpData::getInp( uiWellLogCalc::InpData& inpdata )
{
    if ( isCst() )
    {
	inpdata.iscst_ = true;
	inpdata.cstval_ = getCstVal();
	return true;
    }

    inpdata.noudf_ = udfbox_->isChecked();
    inpdata.wl_ = getLog();
    if ( !inpdata.wl_ )
    {
	BufferString errmsg = "This well has no valid log to use as input,\n";
	errmsg += "the well log has to be of the adequate property type.\n";
	errmsg += "Use well manager to either import or create your logs";
	uiMSG().error( errmsg );
	return false;
    }

    const char* logunitnm = inpdata.wl_->unitMeasLabel();
    const UnitOfMeasure* logun = UnitOfMeasure::getGuessed( logunitnm );
    const UnitOfMeasure* convertun = getUnit();
    if ( logun == convertun )
	return inpdata.wl_;

    delete convertedlog_;
    convertedlog_ = new Well::Log( *inpdata.wl_ );
    for ( int idx=0; idx<inpdata.wl_->size(); idx++ )
    {
	const float initialval = inpdata.wl_->value( idx );
	const float valinsi = logun
		? logun->getSIValue( initialval ) : initialval;
	const float convertedval = convertun
		? convertun->getUserValueFromSI( valinsi ) : valinsi;
	convertedlog_->valArr()[idx] = convertedval;
    }

    inpdata.wl_ = convertedlog_;
    return true;
}


void uiWellLogCalcInpData::vwLog( CallBacker* )
{
    const Well::Log* wl = getLog();
    if ( !wl ) return;

    uiWellLogDisplay::Setup wldsu;
    wldsu.annotinside( true ).nrmarkerchars( 10 ).drawcurvenames( true );
    uiWellLogDispDlg* dlg = new uiWellLogDispDlg( this, wldsu, true );
    dlg->setLog( wl, true );
    dlg->show();
}


void uiWellLogCalcInpData::restrictLogChoice( const PropertyRef::StdType& type )
{
    if ( !wls_ )
	return;

    TypeSet<int> propidxs = wls_->getSuitable( type );
    inpfld_->box()->setEmpty();
    for ( int idx=0; idx<propidxs.size(); idx++ )
	inpfld_->box()->addItem( wls_->getLog(idx).name() );
    inpfld_->box()->addItem( "Constant" );
}
