/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiseisfmtscale.h"
#include "uicompoundparsel.h"
#include "uidialog.h"
#include "uiscaler.h"
#include "datachar.h"
#include "ioman.h"
#include "ioobj.h"
#include "iopar.h"
#include "scaler.h"
#include "seistrc.h"
#include "uigeninput.h"
#include "od_helpids.h"


class uiSeisFmtScaleData
{ mODTextTranslationClass(uiSeisFmtScaleData);
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
{ mODTextTranslationClass(uiSeisFmtScaleDlg);
public:

uiSeisFmtScaleDlg( uiParent* p, Seis::GeomType gt, uiSeisFmtScaleData& d,
		   bool fixedfmtscl, bool withext )
    : uiDialog(p,uiDialog::Setup(tr("Format / Scaling"),mNoDlgTitle,
				 mODHelpKey(mSeisFmtScaleDlgHelpID) ))
    , optimfld_(0)
    , trcgrowfld_(0)
    , data_(d)
    , gt_(gt)
{
    stortypfld_ = new uiGenInput( this, tr("Storage"),
		 StringListInpSpec(DataCharacteristics::UserTypeDef()) );
    stortypfld_->setValue( data_.stor_ );
    if ( fixedfmtscl )
	stortypfld_->setSensitive( false );

    scalefld_ = new uiScaler( this, uiStrings::sEmptyString(), true );
    if ( data_.sclr_ )
	scalefld_->setInput( *data_.sclr_ );
    scalefld_->attach( alignedBelow, stortypfld_ );
    if ( fixedfmtscl )
	scalefld_->setSensitive( false );

    if ( withext )
    {
	trcgrowfld_ = new uiGenInput(this, tr("Adjust Z range to survey range"),
				     BoolInpSpec(false));
	trcgrowfld_->attach( alignedBelow, scalefld_ );
    }

    if ( Seis::is3D(gt) && !Seis::isPS(gt_) )
    {
	optimfld_ = new uiGenInput(this, tr("Optimize for Z-slice viewing"),
				   BoolInpSpec(true));
	optimfld_->setValue( data_.optim_ );
	if ( trcgrowfld_ )
	    optimfld_->attach( alignedBelow, trcgrowfld_ );
	else
	    optimfld_->attach( alignedBelow, scalefld_ );
    }
}

bool acceptOK( CallBacker* ) override
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
{ mODTextTranslationClass(uiSeisFmtScaleComp);
public:

uiSeisFmtScaleComp( uiSeisFmtScale* p, Seis::GeomType gt, bool ffs, bool we )
    : uiCompoundParSel(p,tr("Format / Scaling"),uiStrings::sSpecify())
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

BufferString getSummary() const override
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
    if ( !forexp )
	compfld_ = new uiSeisFmtScaleComp( this, gt, issteer_, withext );
    else
	scalefld_ = new uiScaler( this, uiStrings::sEmptyString(), true );

    setHAlignObj( compfld_ ? (uiGroup*)compfld_ : (uiGroup*)scalefld_ );
    postFinalize().notify( mCB(this,uiSeisFmtScale,updSteer) );
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
    BufferString res = ioobj.pars().find( sKey::Type() );
    if ( !res.isEmpty() )
    {
	const BufferString firstchar( res[0] );
	setSteering( firstchar.isEqual("S") );
    }
    else
	setSteering( false );

    if ( !compfld_ )
	return;

    DataCharacteristics::UserType usrtyp;
    if ( DataCharacteristics::getUserTypeFromPar(ioobj.pars(),usrtyp) )
	compfld_->data_.stor_ = int(usrtyp);

    res = ioobj.pars().find( "Optimized direction" );
    if ( !res.isEmpty() )
    {
	const BufferString dirfirstchar( res[0] );
	compfld_->data_.optim_ = dirfirstchar.isEqual( "H" );
    }
    else
	compfld_->data_.optim_ = false;

    compfld_->updateSummary();
}


void uiSeisFmtScale::updateIOObj( IOObj* ioobj, bool commit ) const
{
    if ( !ioobj ) return;

    if ( !scalefld_ )
	fillFmtPars( ioobj->pars() );

    if ( commit )
    {
	IOM().to( ioobj->key() );
	IOM().commitChanges( *ioobj );
    }
}


void uiSeisFmtScale::fillFmtPars( IOPar& iop ) const
{
    const int tp = getFormat();
    iop.set( sKey::DataStorage(), DataCharacteristics::UserTypeNames()[tp] );
    iop.update( sKeyOptDir(), horOptim() ? "Horizontal" : "" );
}


void uiSeisFmtScale::fillOtherPars( IOPar& iop ) const
{
    scalefld_->fillPar( iop );
    iop.setYN( SeisTrc::sKeyExtTrcToSI(), extendTrcToSI() );
}
