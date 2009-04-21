/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bruno
 Date:		Mar 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: welltiecshot.cc,v 1.1 2009-04-21 13:55:59 cvsbruno Exp $";

#include "welltiecshot.h"

#include "idxable.h"
#include "welldata.h"
#include "welllogset.h"
#include "welllog.h"
#include "welld2tmodel.h"
#include "welltiesetup.h"


WellTieCSCorr::WellTieCSCorr(Well::Data& d, const WellTieSetup& s )
	: log_(new Well::Log(*d.logs().getLog(s.vellognm_)))
	, cs_(d.checkShotModel())
{
    if ( !cs_ )
	return;
    if ( !cs_->size() )
	return;
    
    TypeSet<float> newcsvals; 

    setCSToLogScale( newcsvals, s.factors_.velFactor() );
    fitCS( newcsvals );
    BufferString corr = "Corrected ";
    corr += log_->name();
    log_->setName( s.corrvellognm_ );
    d.logs().add( log_ );
}


void WellTieCSCorr::setCSToLogScale( TypeSet<float>& cstolog, double velfactor )
{
    TypeSet<float> dpt;
    dpt += cs_->dah(0);
    cstolog += cs_->value(0);
    for ( int idx=1; idx< cs_->size(); idx++ )
    {
	dpt[idx] = cs_->dah(idx);
	cstolog += (cs_->value(idx) - cs_->value(idx-1))*1000;
    }

    cstolog[0] = cs_->value(0)/(velfactor*dpt[0]);
    for ( int idx=1; idx<cs_->size(); idx++)
	cstolog[idx] = cstolog[idx]/(2*( dpt[idx]-dpt[idx-1] )*velfactor);
        //To Remove
cstolog[0] += 150;
cstolog[1] += 50;
cstolog[2] -= 50;
cstolog[3] += 50;
cstolog[4] -= 50;

}


void WellTieCSCorr::fitCS( const TypeSet<float>& csvals ) 
{
    TypeSet<float> logvaldah, coeffs, logshifts, csshifts;
   
    /* 
    for ( int idx=0; idx<log_.size(); idx++ )
    {
	if ( idx>1 && (mIsUdf(log_.value(idx))) )
	    logvaldah += log_.valArr()[idx-1];
	else
	    logvaldah +=  log_.value(idx); 
	ctrlspls += idx;
    }

    IdxAble::callibrateArray<float*>( logvaldah.arr(), logvaldah.size(),
		    csvals.arr(), ctrlspls, csvals.size(), false,
		    logshifts.arr() );

    for ( int idx=0; idx<log_.size(); idx++ )
    {
	if ( idx>1 && (mIsUdf(log_.value(idx)) || mIsUdf(logshifts[idx])) )
	    log_.valArr()[idx] = log_.valArr()[idx-1];
	else
	    log_.valArr()[idx]  = logshifts[idx]; 
    }
*/

    for ( int idx=0; idx<cs_->size(); idx++)
    {
	logvaldah += log_->getValue(cs_->dah(idx)); 
	csshifts += csvals[idx]-logvaldah[idx];
    }

    for ( int idx=0; idx<cs_->size()-1; idx++)
	coeffs +=  (csshifts[idx+1]-csshifts[idx])
	    	  /(cs_->dah(idx+1)-cs_->dah(idx));

    for ( int logidx =0; logidx<log_->size()-1; logidx++)
    {
	for (int csidx=0; csidx<cs_->size(); csidx++)
	{
	    if ( (cs_->dah(csidx) <= log_->dah(logidx)) 
		  && (cs_->dah(csidx+1) > log_->dah(logidx)) )
		logshifts += coeffs[csidx]*log_->dah(logidx) 
		   	   + csshifts[csidx]-coeffs[csidx]*cs_->dah(csidx);  
	}
	if ( log_->dah(logidx) <= cs_->dah(0))
	    logshifts += 0;
	if ( log_->dah(logidx) > cs_->dah(cs_->size()-1))
	    logshifts += logshifts[logidx-1];
    }
    
    for ( int idx=0; idx< log_->size(); idx++ )
    {
	if ( idx>1 && (mIsUdf(log_->value(idx)) || mIsUdf(logshifts[idx])) )
	    log_->valArr()[idx] = log_->valArr()[idx-1];
	else
	    log_->valArr()[idx]  = log_->value(idx) + logshifts[idx]; 
    }
}






/*
uiWellTieCShotImpDlg::uiWellTieCShotImpDlg( uiParent* p, WellTieSetup& twtss)
	    : uiDialog(p,uiDialog::Setup("Import new Check Shot",
	    				 "Specify a check shot file",
					 mTODOHelpID))
	    , twtss_(twtss)			
	    , checkshot_(0)				
{
    filefld_ = new uiFileInput( this, "Check Shot file",
    uiFileInput::Setup().withexamine(true) );
    filefld_->setDefaultSelectionDir(
    IOObjContext::getDataDirName(IOObjContext::WllInf) );

    tvdfld_ = new uiGenInput( this, "Depth in CS file is",
    BoolInpSpec(true,"TVDSS","MD") );
    tvdfld_->setValue( false );
    tvdfld_->attach( alignedBelow, filefld_ );

    unitfld_ = new uiGenInput( this, "Depth in",
    BoolInpSpec(!SI().depthsInFeetByDefault(),"Meter","Feet") );
    unitfld_->attach( alignedBelow, tvdfld_ );

    twtfld_ = new uiGenInput( this, "Time is",
    BoolInpSpec(true,"One-way","Two-way traveltime") );
    twtfld_->setValue( false );
    twtfld_->attach( alignedBelow, unitfld_ ? unitfld_ : tvdfld_ );
} 

uiWellTieCShotImpDlg::~uiWellTieCShotImpDlg()
{
}


bool uiWellTieCShotImpDlg::acceptOK( CallBacker* )
{
    bool istvd, istwt, isft;
    fnm_ = filefld_->fileName();
    if ( File_isEmpty(fnm_.buf()) )
	{ uiMSG().error( "Invalid input file" ); return false; }
    istvd = tvdfld_->getBoolValue();
    isft = unitfld_ ? !unitfld_->getBoolValue() 
		    : SI().depthsInFeetByDefault(); 
    istwt = !istvd;

    Well::Data* wd = Well::MGR().get( twtss_.wellid_ );
    if ( !wd ) return false;
    Well::Data tmpwd( *wd );
    Well::AscImporter ascimp( tmpwd );
    BufferString errmsg = ascimp.getD2T( fnm_, istvd, istwt, isft );
    if ( !errmsg.isEmpty() )
	uiMSG().error( "Please select a valid file" );

    checkshot_ = tmpwd.d2TModel(); 

    return true;
}

*/

