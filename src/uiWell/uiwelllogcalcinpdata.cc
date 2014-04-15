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
#include "uigeninput.h"
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
    : uiMathExpressionVariable(inpgrp,fieldnr,true,&p->lognms_)
    , wls_(&p->wls_)
    , convertedlog_(0)
{
    vwbut_ = new uiToolButton( varfld_, "view_log", "Display this log",
				mCB(this,uiWellLogCalcInpData,vwLog) );
    vwbut_->attach( rightOf, varfld_->box() );
    inpSel.notify( mCB(this,uiWellLogCalcInpData,showHideVwBut) );

    udfbox_ = new uiCheckBox( unfld_, "Fill empty sections" );
    udfbox_->setChecked();
    udfbox_->attach( rightOf, unfld_->inpFld() );
}


uiWellLogCalcInpData::~uiWellLogCalcInpData()
{
    delete convertedlog_;
}


void uiWellLogCalcInpData::showHideVwBut( CallBacker* )
{
    vwbut_->display( !isConst() );
}


const Well::Log* uiWellLogCalcInpData::getLog()
{
    return wls_->getLog( varfld_->box()->text() );
}


bool uiWellLogCalcInpData::getInp( uiWellLogCalc::InpData& inpdata )
{
    inpdata.isconst_ = isConst();
    if ( inpdata.isconst_ )
    {
	inpdata.constval_ = toFloat( getInput() );
	if ( mIsUdf(inpdata.constval_) )
	{
	    uiMSG().error( BufferString("Please enter a value for ",varnm_) );
	    return false;
	}
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
	return true;

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


void uiWellLogCalcInpData::setProp( const PropertyRef::StdType& type )
{
    if ( !wls_ )
	return;

    TypeSet<int> propidxs = wls_->getSuitable( type );
    varfld_->box()->setEmpty();
    for ( int idx=0; idx<propidxs.size(); idx++ )
	varfld_->box()->addItem( wls_->getLog(propidxs[idx]).name() );

    if ( unfld_ )
	unfld_->setPropType( type );
}
