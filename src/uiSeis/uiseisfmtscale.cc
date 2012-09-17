/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert Bril
 Date:          May 2002
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiseisfmtscale.cc,v 1.30 2011/11/23 11:35:56 cvsbert Exp $";

#include "uiseisfmtscale.h"
#include "uicompoundparsel.h"
#include "uidialog.h"
#include "uiscaler.h"
#include "datachar.h"
#include "ioman.h"
#include "ioobj.h"
#include "iopar.h"
#include "scaler.h"
#include "uigeninput.h"


class uiSeisFmtScaleData
{
public:

uiSeisFmtScaleData() : stor_(0), sclr_(0), optim_(false), trcgrow_(false) {}
uiSeisFmtScaleData( const uiSeisFmtScaleData& d ) : sclr_(0)
{
    stor_ = d.stor_; optim_ = d.optim_;
    trcgrow_ = d.trcgrow_;
    setScaler( d.sclr_ );
}

void setScaler( Scaler* sc )
{
    if ( sclr_ != sc )
	{ delete sclr_; sclr_ = sc ? sc->clone() : 0; }
}

Scaler* getScaler() const
{
    return sclr_ ? sclr_->clone() : 0;
}

    int		stor_;
    Scaler*	sclr_;
    bool	optim_;
    bool	trcgrow_;

};


class uiSeisFmtScaleDlg : public uiDialog
{
public:

uiSeisFmtScaleDlg( uiParent* p, Seis::GeomType gt, uiSeisFmtScaleData& d,
		   bool fixedfmtscl, bool withext )
    : uiDialog(p,uiDialog::Setup("Format / Scaling","Format and scaling",
				 "103.0.17"))
    , optimfld_(0)
    , trcgrowfld_(0)
    , data_(d)
    , gt_(gt)
{
    stortypfld_ = new uiGenInput( this, "Storage",
		 StringListInpSpec(DataCharacteristics::UserTypeNames()) );
    stortypfld_->setValue( data_.stor_ );
    if ( fixedfmtscl )
	stortypfld_->setSensitive( false );

    scalefld_ = new uiScaler( this, 0, true );
    if ( data_.sclr_ )
	scalefld_->setInput( *data_.sclr_ );
    scalefld_->attach( alignedBelow, stortypfld_ );
    if ( fixedfmtscl )
	scalefld_->setSensitive( false );

    if ( withext )
    {
	trcgrowfld_ = new uiGenInput( this, "Adjust Z range to survey range",
				      BoolInpSpec(false) );
	trcgrowfld_->attach( alignedBelow, scalefld_ );
    }

    if ( !Seis::isPS(gt_) )
    {
	optimfld_ = new uiGenInput( this, "Optimize horizontal slice access",
				   BoolInpSpec(true) );
	optimfld_->setValue( data_.optim_ );
	if ( trcgrowfld_ )
	    optimfld_->attach( alignedBelow, trcgrowfld_ );
	else
	    optimfld_->attach( alignedBelow, scalefld_ );
    }
}

bool acceptOK( CallBacker* )
{
    data_.stor_ = stortypfld_->getIntValue();
    data_.sclr_ = scalefld_->getScaler();
    data_.optim_ = optimfld_ && optimfld_->getBoolValue();
    data_.trcgrow_ = trcgrowfld_ && trcgrowfld_->getBoolValue();
    return true;
}

    uiGenInput*	stortypfld_;
    uiGenInput*	optimfld_;
    uiGenInput*	trcgrowfld_;
    uiScaler*	scalefld_;

    uiSeisFmtScaleData&	data_;
    Seis::GeomType	gt_;
};


class uiSeisFmtScaleComp : public uiCompoundParSel
{
public:

uiSeisFmtScaleComp( uiSeisFmtScale* p, Seis::GeomType gt, bool ffs, bool we )
    : uiCompoundParSel(p,"Format / Scaling","Sp&ecify")
    , gt_(gt)
    , fixfmtscl_(ffs)
    , withext_(we)
{
    butPush.notify( mCB(this,uiSeisFmtScaleComp,doDlg) );
}

void doDlg( CallBacker* )
{
    uiSeisFmtScaleDlg dlg( this, gt_, data_, fixfmtscl_, withext_ );
    dlg.go();
}

BufferString getSummary() const
{
    const char* nms[] = { "Auto",
	  "8bit [-,+]", "8bit [0,+]",
	  "16bit [-,+]", "16bit [0,+]",
	  "32bit [-,+]", "32bit [0,+]",
	  "32bit [float]", "64bit [float]", "64bit [-,+]", 0 };
    BufferString ret( nms[data_.stor_] );
    ret += " / ";
    ret += data_.sclr_ ? data_.sclr_->toString() : "None";
    if ( data_.optim_ )
	ret += " (Hor optim)";
    return ret;
}

    uiSeisFmtScaleData	data_;
    Seis::GeomType	gt_;
    bool		fixfmtscl_;
    bool		withext_;

};


uiSeisFmtScale::uiSeisFmtScale( uiParent* p, Seis::GeomType gt, bool forexp,
       				bool withext )
	: uiGroup(p,"Seis format and scale")
	, gt_(gt)
	, issteer_(false)
	, compfld_(0)
	, scalefld_(0)
{
    if ( !forexp && !Seis::is2D(gt_) )
	compfld_ = new uiSeisFmtScaleComp( this, gt, issteer_, withext );
    else
	scalefld_ = new uiScaler( this, 0, true );

    setHAlignObj( compfld_ ? (uiGroup*)compfld_ : (uiGroup*)scalefld_ );
    postFinalise().notify( mCB(this,uiSeisFmtScale,updSteer) );
}


void uiSeisFmtScale::setSteering( bool yn )
{
    issteer_ = yn;
    updSteer( 0 );
}


void uiSeisFmtScale::updSteer( CallBacker* )
{
    if ( !issteer_ ) return;

    if ( scalefld_ )
	scalefld_->setUnscaled();
    else
    {
	compfld_->data_.setScaler( 0 );
	compfld_->data_.stor_ = (int)DataCharacteristics::SI16;
	compfld_->updateSummary();
    }
}


Scaler* uiSeisFmtScale::getScaler() const
{
    return scalefld_ ? scalefld_->getScaler() : compfld_->data_.getScaler();
}


int uiSeisFmtScale::getFormat() const
{
    return scalefld_ ? 0 : compfld_->data_.stor_;
}


bool uiSeisFmtScale::horOptim() const
{
    return scalefld_ ? false : compfld_->data_.optim_;
}


bool uiSeisFmtScale::extendTrcToSI() const
{
    return compfld_ ? compfld_->data_.trcgrow_ : false;
}


void uiSeisFmtScale::updateFrom( const IOObj& ioobj )
{
    const char* res = ioobj.pars().find( "Type" );
    setSteering( res && *res == 'S' );
    if ( !compfld_ ) return;

    res = ioobj.pars().find( sKey::DataStorage );
    if ( res )
	compfld_->data_.stor_ = (int)(*res - '0');

    res = ioobj.pars().find( "Optimized direction" );
    compfld_->data_.optim_ = res && *res == 'H';

    compfld_->updateSummary();
}


void uiSeisFmtScale::updateIOObj( IOObj* ioobj, bool commit ) const
{
    if ( !ioobj || Seis::is2D(gt_) ) return;

    if ( !scalefld_ )
    {
	const int tp = getFormat();
	ioobj->pars().set( sKey::DataStorage,
		DataCharacteristics::UserTypeNames()[tp] );
	if ( horOptim() )
	    ioobj->pars().set( "Optimized direction", "Horizontal" );
	else
	    ioobj->pars().removeWithKey( "Optimized direction" );
    }

    if ( commit )
    {
	IOM().to( ioobj->key() );
	IOM().commitChanges( *ioobj );
    }
}
