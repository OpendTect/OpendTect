/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Helene Huck
 Date:		March 2012
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";


#include "uiwelllogcalcinpdata.h"

#include "uicombobox.h"
#include "uitoolbutton.h"
#include "uilabel.h"
#include "uimathexpression.h"
#include "uimsg.h"
#include "uirockphysform.h"
#include "uiwelllogcalc.h"
#include "uiwelllogdisplay.h"

#include "welllogset.h"
#include "welllog.h"
#include "separstr.h"
#include "mathexpression.h"
#include "unitofmeasure.h"

static const char* specvararr[] = { "MD", "DZ", 0 };
static const BufferStringSet specvars( specvararr );

uiWellLogCalcInpData::uiWellLogCalcInpData( uiWellLogCalc* p, uiGroup* inpgrp,
					    int fieldnr )
    : uiMathExpressionVariable(inpgrp,p->lognms_,fieldnr)
    , wls_(&p->wls_)
    , convertedlog_(0)
{
    inpfld_->box()->selectionChanged.notify( 
				    mCB( this,uiWellLogCalcInpData,inputSel ) );
    inpfld_->box()->selectionChanged.notify( mCB(p,uiWellLogCalc,inpSel) );
    uiToolButton* tb = new uiToolButton( this, "view_log", "Display this log",
	    			mCB(this,uiWellLogCalcInpData,vwLog) );
    tb->attach( rightOf, inpfld_ );
    unfld_->attach( ensureRightOf, tb );

    udfbox_ = new uiCheckBox( this, "Fill empty sections" );
    udfbox_->setChecked();
    udfbox_->attach( rightOf, unfld_ ? unfld_ : inpfld_ );
}


uiWellLogCalcInpData::~uiWellLogCalcInpData()
{
    delete convertedlog_;
}


void uiWellLogCalcInpData::use( const MathExpression* expr )
{
    const int nrvars = expr ? expr->nrUniqueVarNames() : 0;
    if ( idx_ >= nrvars )
	{ display( false ); return; }
    const BufferString varnm = expr->uniqueVarName( idx_ );
    if ( specvars.isPresent(varnm.buf()) )
	{ display( false ); return; }

    varnm_ = varnm;
    display( true );
    BufferString inplbl = "For '"; inplbl += varnm; inplbl += "' use";
    inpfld_->label()->setText( inplbl.buf() );
    const int nearidx = posinpnms_.nearestMatch( varnm );
    if ( nearidx >= 0 )
	inpfld_->box()->setCurrentItem( nearidx );

    inputSel(0);
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


void uiWellLogCalcInpData::inputSel( CallBacker* )
{
    if ( !unfld_ ) return;
    const Well::Log* wl = getLog();
    if ( !wl ) return;

    ObjectSet<const UnitOfMeasure> possibleunits;
    const BufferString logunitnm = wl->unitMeasLabel();
    const UnitOfMeasure* logun = UnitOfMeasure::getGuessed(logunitnm);
    if ( !logun )
	possibleunits = UoMR().all();
    else
	UoMR().getRelevant( logun->propType(), possibleunits );

    uiComboBox& cbb = *unfld_->box();
    cbb.setEmpty(); cbb.addItem( "-" );
    for ( int idx=0; idx<possibleunits.size(); idx++ )
	cbb.addItem( possibleunits[idx]->name() );

    if ( logun && cbb.isPresent(logun->name()) )
	cbb.setText( logun->name() );
    else if ( cbb.isPresent(logunitnm) )
	cbb.setText( logunitnm );
    else
	cbb.setText( "-" );
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
    if ( !wls_ ) return;
    PropertyRef property( "dummy", type );
    BufferStringSet lognms;
    TypeSet<int> propidx;                                                   
    TypeSet<int> isaltpropref;
    uiWellLogCalc::getSuitableLogs( *wls_, lognms, propidx, isaltpropref,
	    			    property, 0 );
    const_cast<BufferStringSet&>(posinpnms_) = lognms;
    inpfld_->box()->setEmpty();
    inpfld_->box()->addItems( lognms );
    inpfld_->box()->addItem( "Constant" );
}
