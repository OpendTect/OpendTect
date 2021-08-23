/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          Feb 2006
________________________________________________________________________

-*/

#include "uitblimpexpdatasel.h"

#include "uibuttongroup.h"
#include "uicombobox.h"
#include "uicompoundparsel.h"
#include "uicoordsystem.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "uilineedit.h"
#include "uimsg.h"
#include "uiselsimple.h"
#include "uiseparator.h"
#include "uispinbox.h"
#include "uitoolbutton.h"
#include "uiunitsel.h"

#include "ioman.h"
#include "iopar.h"
#include "tabledef.h"
#include "tableascio.h"
#include "unitofmeasure.h"

#define mChoiceBoxWidth 12


class uiTableTargetInfoEd : public uiGroup
{ mODTextTranslationClass(uiTableTargetInfoEd)
public:

uiTableTargetInfoEd( uiParent* p, Table::TargetInfo& tinf, bool ishdr,
		     int nrlns )
    : uiGroup(p,tinf.name())
    , tinf_(tinf)
    , ishdr_(ishdr)
    , nrhdrlns_(nrlns)
    , formfld_(nullptr)
    , specfld_(nullptr)
    , unitfld_(nullptr)
    , crsfld_(nullptr)
{
    if ( tinf_.nrForms() < 1 )
	return;

    const CallBack boxcb( mCB(this,uiTableTargetInfoEd,boxChg) );
    if ( tinf_.nrForms() > 1 )
    {
	formfld_ = new uiComboBox( this, "Form choice" );
	formfld_->setPrefWidthInChar( 16 );
	formfld_->selectionChanged.notify( boxcb );
    }

    if ( ishdr_ && choicegrp_ )
    {
	specfld_ = new uiComboBox( choicegrp_, "provide/read" );
	specfld_->addItem( tr("provide") );
	specfld_->addItem( tr("keyword") );
	specfld_->addItem( tr("fixed") );
	specfld_->setPrefWidthInChar( mChoiceBoxWidth );
	specfld_->selectionChanged.notify( boxcb );
	specfld_->setCurrentItem( tinf_.selection_.isKeyworded(0) ? 1
				: (tinf_.selection_.isInFile(0) ? 2 : 0) );
    }

    const OD::String& nmstr = tinf_.name();
    if ( !nmstr.isEmpty() )
    {
	uiString  lbltxt = tinf_.isOptional() ? tr("[%1]").arg(tinf_.name()) :
			   tr("%1").arg(tinf_.name());
	uiLabel* lbl = new uiLabel( this, lbltxt );
	if ( formfld_ )
	    lbl->attach( rightOf, formfld_ );
	rightmostleftfld_ = lbl;
    }
    else
	rightmostleftfld_ = formfld_;

    for ( int iform=0; iform<tinf_.nrForms(); iform++ )
    {
	rightmostfld_ = rightmostleftfld_;
	const Table::TargetInfo::Form& form = tinf_.form( iform );
	if ( formfld_ )
	{
	    BufferString formnm = form.name();
	    if ( tinf_.isOptional() )
		formnm.embed( '[', ']' );
	    formfld_->addItem( mToUiStringTodo(formnm) );
	}

	mkColFlds( iform );
    }

    const int formnr = tinf_.selection_.form_;
    if ( formnr>=0 && formnr<tinf_.nrForms() && formfld_ )
	formfld_->setCurrentItem( formnr );

    Mnemonic::StdType proptyp = tinf_.propertyType();
    if ( proptyp != Mnemonic::Other )
    {
	unitfld_ = new uiUnitSel( this,
				uiUnitSel::Setup(proptyp,uiStrings::sUnit()) );
	unitfld_->attach( rightTo, rightmostfld_ );
	if ( tinf_.selection_.unit_ )
	    unitfld_->setUnit( tinf_.selection_.unit_->name() );
	else
	    unitfld_->setUnit();
    }

    if ( tinf_.selection_.coordsys_
	    && tinf_.selection_.coordsys_->isProjection() )
    {
	crsfld_ = new Coords::uiCoordSystemSel( this, true, true,
						tinf_.selection_.coordsys_ );
	if ( !colboxes_.isEmpty() && !colboxes_[0]->isEmpty() )
	    crsfld_->attach( alignedBelow, (*colboxes_[0])[0] );
	else
	    crsfld_->attach( alignedBelow, rightmostfld_ );
    }

    postFinalise().notify( boxcb );
}


void doAttach( uiTableTargetInfoEd* prev )
{
    if ( !prev ) return;

    attach( alignedBelow, prev );
    if ( specfld_ )
	specfld_->attach( alignedBelow, prev->specfld_ );
}

void mkColFlds( int iform )
{
    colboxes_ += new ObjectSet<uiSpinBox>;
    if ( ishdr_ )
    {
	rowboxes_ += new ObjectSet<uiSpinBox>;
	kwinps_ += new ObjectSet<uiLineEdit>;
	inps_ += new ObjectSet<uiGenInput>;
    }

    const int nrflds = tinf_.form(iform).specs_.size();
    for ( int ifld=0; ifld<nrflds; ifld++ )
    {
	addBoxes( iform, ifld );
	if ( ishdr_ )
	    addInp( iform, ifld );
    }
}

void addBoxes( int iform, int ifld )
{
    uiSpinBox* rowspinbox = nullptr;
    uiLineEdit* kwinp = nullptr;

    if ( ishdr_ )
    {
	rowspinbox = new uiSpinBox( this );
	rowspinbox->setInterval( 1, nrhdrlns_ > 0 ? nrhdrlns_ : 999, 1 );
	rowspinbox->setPrefix( tr("row:") );
	*rowboxes_[iform] += rowspinbox;
	ObjectSet<uiLineEdit>& kwinps = *kwinps_[iform];
	kwinp = new uiLineEdit( this, "keyword" );
	kwinps += kwinp;
	kwinp->setHSzPol( uiObject::Small );
    }

    ObjectSet<uiSpinBox>& colboxes = *colboxes_[iform];
    BufferString heading; heading += "Col :"; heading += ifld+1;
    uiSpinBox* colspinbox = new uiSpinBox( this, 0, heading.buf() );
    const int firstcol = tinf_.isOptional() ? 0 : 1;
    colspinbox->setInterval( firstcol, 999, 1 );
    colspinbox->setPrefix( tr("col:") );
    colboxes += colspinbox;

    if ( !rowspinbox )
	colspinbox->attach( rightOf, rightmostfld_ );
    else
    {
	rowspinbox->attach( rightOf, rightmostfld_ );
	kwinp->attach( rightOf, rightmostfld_ );
	colspinbox->attach( rightOf, rowspinbox );
	colspinbox->attach( ensureRightOf, kwinp );
    }
    rightmostfld_ = colspinbox;

    if ( ifld == 0 && iform == 0 )
	setHAlignObj( rowspinbox ? rowspinbox : colspinbox );

    const bool iskw = tinf_.selection_.isKeyworded(ifld);
    const bool isrc = tinf_.selection_.isInFile(ifld) && !iskw;

    if ( !isrc && rowspinbox )
	{ rowspinbox->setValue( defrow_ ); defrow_++; }

    if ( !iskw && !isrc )
    {
	colspinbox->setValue( tinf_.isOptional() ? 0 : defcol_ + ifld );
	if ( !ishdr_ && iform==tinf_.nrForms()-1
	     && ifld==tinf_.form(iform).specs_.size()-1 ) defcol_ += ifld + 1;
    }
    else
    {
	const RowCol& rc = tinf_.selection_.elems_[ifld].pos_;
	if ( rowspinbox )
	{
	    if ( isrc )
		rowspinbox->setValue( rc.row()+1 ); // Users tend to start at 1
	    else
		kwinp->setText( tinf_.selection_.elems_[ifld].keyword_ );
	}
	colspinbox->setValue( rc.col() + 1 );
    }
}

void addInp( int iform, int ifld )
{
    ObjectSet<uiGenInput>& colinps = *inps_[iform];
    uiGenInput* inp = new uiGenInput( this, uiString::emptyString(),
			  *tinf_.form(iform).specs_[ifld] );
    const char* val = tinf_.selection_.getVal(ifld);
    if ( val && *val )
	inp->setText( val );

    colinps += inp;

    if ( ifld )
	inp->attach( rightOf, colinps[ifld-1] );
    else
	inp->attach( rightOf, rightmostleftfld_ );
}


void boxChg( CallBacker* )
{
    const int selformidx = formfld_ ? formfld_->currentItem() : 0;
    const bool isspec = specfld_ ? specfld_->currentItem()==0 : ishdr_;
    const bool iskw = specfld_ && specfld_->currentItem() == 1;

    for ( int iform=0; iform<tinf_.nrForms(); iform++ )
    {
	const bool isselform = iform == selformidx;

	ObjectSet<uiSpinBox>& colboxes = *colboxes_[iform];
	ObjectSet<uiSpinBox>* rowboxes = iform < rowboxes_.size()
				       ? rowboxes_[iform] : nullptr;
	ObjectSet<uiLineEdit>* kwinps = iform < kwinps_.size()
				       ? kwinps_[iform] : nullptr;
	for ( int ifld=0; ifld<colboxes.size(); ifld++ )
	{
	    colboxes[ifld]->display( isselform && !isspec );
	    if ( rowboxes )
		(*rowboxes)[ifld]->display( isselform && !iskw && !isspec );
	    if ( kwinps )
		(*kwinps)[ifld]->display( isselform && iskw );
	}
	if ( ishdr_ )
	{
	    ObjectSet<uiGenInput>& colinps = *inps_[iform];
	    for ( int ifld=0; ifld<colinps.size(); ifld++ )
		colinps[ifld]->display( isselform && isspec );
	}
    }

    if ( crsfld_ )
	crsfld_->display( !selformidx );	// When Position is XY.
}

bool commit()
{
    const int formnr = formfld_ ? formfld_->currentItem() : 0;
    const bool doread = !ishdr_ || (specfld_ && specfld_->currentItem() > 0);
    const bool iskw = specfld_ && specfld_->currentItem() == 1;

    if ( !tinf_.isOptional() )
    {
	if ( iskw )
	{
	    ObjectSet<uiLineEdit>& kwinps = *kwinps_[formnr];
	    for ( int idx=0; idx<kwinps.size(); idx++ )
	    {
		if ( !*kwinps[idx]->text() )
		{
		    errmsg_ = tr("Missing keyword for %1")
			    .arg(tinf_.form(formnr).name());
		    return false;
		}
	    }
	}
	else if ( !doread )
	{
	    ObjectSet<uiGenInput>& colinps = *inps_[formnr];
	    for ( int idx=0; idx<colinps.size(); idx++ )
	    {
		if ( colinps[idx]->isUndef() )
		{
		    errmsg_ = tr("Value missing for %1")
			    .arg(tinf_.form(formnr).name());
		    return false;
		}
	    }
	}
    }

    tinf_.selection_.form_ = formnr;
    tinf_.selection_.elems_.erase();

    if ( !doread )
    {
	ObjectSet<uiGenInput>& colinps = *inps_[formnr];
	for ( int idx=0; idx<colinps.size(); idx++ )
	    tinf_.selection_.elems_ +=
		Table::TargetInfo::Selection::Elem( colinps[idx]->text() );
    }
    else
    {
	ObjectSet<uiSpinBox>& colboxes = *colboxes_[formnr];
	ObjectSet<uiSpinBox>* rowboxes = rowboxes_.size() > formnr
				       ? rowboxes_[formnr] : nullptr;
	ObjectSet<uiLineEdit>* kwinps = kwinps_.size() > formnr
				       ? kwinps_[formnr] : nullptr;
	for ( int idx=0; idx<colboxes.size(); idx++ )
	{
	    RowCol rc( 0, colboxes[idx]->getIntValue() );
	    BufferString kw;

	    if ( !iskw && rowboxes )
		rc.row() = (*rowboxes)[idx]->getIntValue();
	    if ( iskw && kwinps )
		kw = (*kwinps)[idx]->text();
	    if ( mIsUdf(rc.row()) || (!iskw && mIsUdf(rc.col())) )
	    {
		errmsg_ = tr("Missing position in the file for %1")
			.arg(tinf_.form(formnr).name());
		return false;
	    }
	    else if ( iskw && kw.isEmpty() )
	    {
		errmsg_ = tr("Missing header keyword for %1")
			.arg(tinf_.form(formnr).name());
		return false;
	    }
	    rc.row()--; rc.col()--; // Users tend to start at 1
	    tinf_.selection_.elems_ +=
				    Table::TargetInfo::Selection::Elem(rc,kw);
	}
    }

    tinf_.selection_.unit_ = unitfld_ ? unitfld_->getUnit() : nullptr;
    if ( crsfld_ )
    {
	if ( crsfld_->isDisplayed() )
	    tinf_.selection_.coordsys_ = crsfld_->getCoordSystem();
	else
	    tinf_.selection_.coordsys_ = SI().getCoordSystem();
    }

    return true;
}

    Table::TargetInfo&			tinf_;
    bool				ishdr_;
    int					nrhdrlns_;
    uiString				errmsg_;

    uiComboBox*				formfld_;
    uiComboBox*				specfld_;
    uiUnitSel*				unitfld_;
    Coords::uiCoordSystemSel*		crsfld_;
    ObjectSet< ObjectSet<uiSpinBox> >	colboxes_;
    ObjectSet< ObjectSet<uiSpinBox> >	rowboxes_;
    ObjectSet< ObjectSet<uiGenInput> >	inps_;
    ObjectSet< ObjectSet<uiLineEdit> >	kwinps_;
    uiObject*				rightmostfld_;
    uiObject*				rightmostleftfld_;

    static uiGroup*			choicegrp_;
    static int				defrow_;
    static int				defcol_;
    static void				initDefs()
					{ defrow_ = defcol_ = 1; }

};

int uiTableTargetInfoEd::defrow_ = 1;
int uiTableTargetInfoEd::defcol_ = 1;
uiGroup* uiTableTargetInfoEd::choicegrp_ = nullptr;


class uiTableFormatDescFldsEd : public uiDialog
{ mODTextTranslationClass(uiTableFormatDescFldsEd)
public:

				uiTableFormatDescFldsEd(uiTableImpDataSel*,
							const HelpKey&);

    Table::FormatDesc&		desc()		{ return fd_; }

protected:

    uiTableImpDataSel&		ds_;
    Table::FormatDesc&		fd_;
    int				nrhdrlines_;

    uiGroup*			hdrinpgrp_;
    uiGroup*			bodyinpgrp_;
    uiGroup*			elemgrp_;
    ObjectSet<uiTableTargetInfoEd> hdrelems_;
    ObjectSet<uiTableTargetInfoEd> bodyelems_;
    uiTableTargetInfoEd*	lastelm_;
    uiGenInput*			eobfld_;

    void			mkElemFlds(bool);
    void			initSaveButton(CallBacker*);
    void			openFmt(CallBacker*);
    void			saveFmt(CallBacker*);

    bool			commit();
    bool			acceptOK(CallBacker*);
};




uiTableFormatDescFldsEd::uiTableFormatDescFldsEd( uiTableImpDataSel* ds,
						  const HelpKey& helpkey )
	: uiDialog(ds,uiDialog::Setup(tr("Format Definition"),
				      tr("Specify Necessary Information"),
				      helpkey)
		      .savebutton(true).savebutispush(true)
		      .savetext(tr("Save format")))
	, ds_(*ds)
	, fd_(ds->desc())
	, hdrinpgrp_(0)
	, bodyinpgrp_(0)
	, nrhdrlines_(ds->nrHdrLines())
{
    hdrelems_.allowNull( true ); bodyelems_.allowNull( true );

    elemgrp_ = new uiGroup( this, "Elem group" );
    uiTableTargetInfoEd::choicegrp_ = 0;

    lastelm_ = 0;
    mkElemFlds( true );
    mkElemFlds( false );
    if ( !lastelm_ ) return;

    eobfld_ = new uiGenInput( lastelm_->attachObj()->parent(),
			      tr("Stop reading at"),
			      StringInpSpec(fd_.eobtoken_) );
    eobfld_->setWithCheck( true );
    eobfld_->setChecked( fd_.haveEOBToken() );
    eobfld_->attach( alignedBelow, lastelm_ );

    if ( hdrinpgrp_ && bodyinpgrp_ )
	bodyinpgrp_->attach( alignedBelow, hdrinpgrp_ );

    postFinalise().notify( mCB(this,uiTableFormatDescFldsEd,initSaveButton) );
}


void uiTableFormatDescFldsEd::initSaveButton( CallBacker* cb )
{
    uiButton* but = button( uiDialog::SAVE );
    but->setToolTip ( tr("Save Format") );
    if ( but )
	but->activated.notify( mCB(this,uiTableFormatDescFldsEd,saveFmt) );
}


void uiTableFormatDescFldsEd::mkElemFlds( bool ishdr )
{
    ObjectSet<Table::TargetInfo>& infos = ishdr ? fd_.headerinfos_
						: fd_.bodyinfos_;
    if ( infos.size() < 1 ) return;

    ObjectSet<uiTableTargetInfoEd>& elms = ishdr ? hdrelems_ : bodyelems_;

    uiTableTargetInfoEd::initDefs();
    uiGroup* grp;
    if ( !ishdr )
	grp = bodyinpgrp_ = new uiGroup( elemgrp_, "Body input group" );
    else
    {
	grp = hdrinpgrp_ = new uiGroup( elemgrp_, "Header input group" );
	if ( nrhdrlines_ != 0 )
	{
	    uiTableTargetInfoEd::choicegrp_ = new uiGroup( this,
						"provide/read choice group" );
	    elemgrp_->attach( rightOf, uiTableTargetInfoEd::choicegrp_ );
	}
    }

    uiTableTargetInfoEd* prev = 0;
    for ( int idx=0; idx<infos.size(); idx++ )
    {
	if ( infos[idx]->isHidden() )
	    { elms += 0; continue; }

	uiTableTargetInfoEd* elem = new uiTableTargetInfoEd( grp, *infos[idx],
						     ishdr, nrhdrlines_ );
	elms += elem;
	if ( prev )
	    elem->doAttach( prev );
	else
	    grp->setHAlignObj( elem );

	lastelm_ = prev = elem;
    }
}


#define mDoCommit(elms) \
    for ( int idx=0; idx<elms.size(); idx++ ) \
    { \
	if ( elms[idx] && !elms[idx]->commit() ) \
	{ \
	    uiMSG().error( elms[idx]->errmsg_ ); \
	    return false; \
	} \
    }

bool uiTableFormatDescFldsEd::commit()
{
    mDoCommit(hdrelems_)
    mDoCommit(bodyelems_)

    if ( eobfld_->isChecked() )
	fd_.eobtoken_ = eobfld_->text();
    else
	fd_.eobtoken_.setEmpty();

    return true;
}

bool uiTableFormatDescFldsEd::acceptOK( CallBacker* )
{
    return commit();
}


void uiTableFormatDescFldsEd::saveFmt( CallBacker* )
{
    if ( !commit() )
	return;

    BufferStringSet nms;
    Table::FFR().getFormats( fd_.name(), nms );
    nms.sort();

    uiGetObjectName::Setup listsetup( tr("Save format"), nms );
    listsetup.inptxt( tr("Name for format") )
	     .deflt( ds_.fmtname_ )
	     .dlgtitle( tr("Enter a name for the format") );
    uiGetObjectName dlg( this, listsetup );
    const char* strs[] = { "All Surveys",
			   "This Survey only",
			   "My user ID only", 0 };
    uiGenInput* srcfld = new uiGenInput( &dlg, tr("Store for"),
					 StringListInpSpec(strs) );
    srcfld->attach( alignedBelow, dlg.inpFld() );
    if ( dlg.go() )
    {
	const char* fmtnm = dlg.text();
	IOPar* newiop = new IOPar;
	fd_.fillPar( *newiop );
	Repos::Source src = (Repos::Source)(srcfld->getIntValue()+3);
	Table::FFR().set( fd_.name(), fmtnm, newiop, src );
	if ( !Table::FFR().write(src) )
	    uiMSG().error( tr("Cannot write format") );
	else
	{
	    ds_.storediop_.setEmpty(); fd_.fillPar( ds_.storediop_ );
	    ds_.fmtname_ = fmtnm;
	}
    }
}


class uiTableFmtDescFldsParSel : public uiCompoundParSel
{ mODTextTranslationClass(uiTableFmtDescFldsParSel);
public:

uiTableFmtDescFldsParSel( uiTableImpDataSel* p, const HelpKey& helpkey )
    : uiCompoundParSel( p, tr("Format definition"), OD::Define )
    , impsel_(*p)
    , helpkey_(helpkey)
    , descCommitted(this)
{
    butPush.notify( mCB(this,uiTableFmtDescFldsParSel,doDlg) );
}

BufferString getSummary() const
{
    BufferString ret;
    if ( !impsel_.desc().isGood() )
	ret = "<Incomplete>";
    else
    {
	bool isstor = !impsel_.fmtname_.isEmpty();
	if ( isstor )
	{
	    IOPar curiop; impsel_.desc().fillPar( curiop );
	    isstor = impsel_.storediop_ == curiop;
	}

	if ( !isstor )
	    ret = "<Defined>";
	else
	    ret = impsel_.fmtname_;
    }

    return ret;
}

void doDlg( CallBacker* )
{
    if ( !impsel_.commitHdr(true) )
	return;

    uiTableFormatDescFldsEd dlg( &impsel_, helpkey_ );
    if ( dlg.go() ) descCommitted.trigger();
}

    uiTableImpDataSel&	impsel_;
    HelpKey		helpkey_;
    Notifier<uiTableFmtDescFldsParSel> descCommitted;

};


uiTableImpDataSel::uiTableImpDataSel( uiParent* p, Table::FormatDesc& fd,
				      const HelpKey& helpkey )
	: uiGroup(p,fd.name())
	, fd_(fd)
	, descChanged(this)
{
    const char* hdrtyps[] = { "No header", "Fixed size", "Variable", 0 };
    const CallBack typchgcb = mCB(this,uiTableImpDataSel,typChg);
    const CallBack hdrchgcb = mCB(this,uiTableImpDataSel,hdrChg);
    hdrtypefld_ = new uiGenInput( this, tr("File header"),
				  StringListInpSpec(hdrtyps) );
    hdrtypefld_->valuechanged.notify( typchgcb );

    uiToolButton* button = new uiToolButton( this, "open",
				tr("Selecting existing format"),
				mCB(this,uiTableImpDataSel,openFmt) );
    button->setPrefWidthInChar( 6 );
    button->attach( rightOf, hdrtypefld_ );

    int nrlns = fd_.nrHdrLines(); if ( nrlns < 1 ) nrlns = 1;
    IntInpSpec iis( nrlns ); iis.setLimits( Interval<int>(1,mUdf(int)) );
    hdrlinesfld_ = new uiGenInput( this, tr("Header size (number of lines)"),
				   iis );
    hdrlinesfld_->attach( alignedBelow, hdrtypefld_ );
    hdrlinesfld_->valuechanged.notify( hdrchgcb );
    hdrtokfld_ = new uiGenInput( this, tr("End-of-header 'word'"),
				 StringInpSpec(fd_.eohtoken_) );
    hdrtokfld_->attach( alignedBelow, hdrtypefld_ );
    hdrtokfld_->valuechanged.notify( hdrchgcb );

    fmtdeffld_ = new uiTableFmtDescFldsParSel( this, helpkey );
    fmtdeffld_->attach( alignedBelow, hdrlinesfld_ );
    fmtdeffld_->descCommitted.notify( mCB(this,uiTableImpDataSel,descChg) );

    setHAlignObj( hdrtypefld_ );
    postFinalise().notify( typchgcb );
}


void uiTableImpDataSel::typChg( CallBacker* )
{
    const int htyp = hdrtypefld_->getIntValue();
    hdrlinesfld_->display( htyp == 1 );
    hdrtokfld_->display( htyp == 2 );
    hdrChg( 0 );
}

void uiTableImpDataSel::hdrChg( CallBacker* )
{
    commitHdr( false );
    fmtdeffld_->updateSummary();
    descChanged.trigger();
}


void uiTableImpDataSel::descChg( CallBacker* )
{
    descChanged.trigger();
}


void uiTableImpDataSel::openFmt( CallBacker* )
{
    BufferStringSet avfmts;
    Table::FFR().getFormats( fd_.name(), avfmts );
    if ( avfmts.size() < 1 )
    {
	uiMSG().error( tr("No pre- or user-defined formats available") );
	return;
    }
    avfmts.sort();

    uiSelectFromList::Setup listsetup( tr("Retrieve data format"), avfmts );
    listsetup.dlgtitle( tr("Select a format to retrieve") );
    uiSelectFromList dlg( this, listsetup );
    if ( !dlg.go() || dlg.selection() < 0 )
	return;

    const BufferString& fmtnm = avfmts.get( dlg.selection() );
    const IOPar* iop = Table::FFR().get( fd_.name(), fmtnm );
    if ( !iop ) return; //Huh?

    fd_.clear(); fd_.usePar( *iop );
    hdrtokfld_->setText( fd_.eohtoken_ );
    const int nrlns = fd_.nrHdrLines();
    hdrtypefld_->setValue( fd_.needEOHToken() ? 2 : (nrlns == 0 ? 0 : 1) );
    hdrlinesfld_->setValue( mIsUdf(nrlns) || nrlns < 1 ? 1 : nrlns );

    typChg( 0 );
    storediop_.setEmpty(); fd_.fillPar( storediop_ );
    fmtname_ = fmtnm;
    fmtdeffld_->updateSummary();
}


int uiTableImpDataSel::nrHdrLines() const
{
    const int htyp = hdrtypefld_->getIntValue();
    return htyp == 0 ? 0 : (htyp == 2 ? -1 : hdrlinesfld_->getIntValue());
}


bool uiTableImpDataSel::commitHdr( bool witherror )
{
    const int htyp = hdrtypefld_->getIntValue();
    if ( htyp == 2 )
    {
	BufferString tok = hdrtokfld_->text();
	if ( tok.isEmpty() && witherror )
	{
	    uiMSG().error(
		tr("Please enter the string marking the end-of-header"));
	    return false;
	}
	if ( tok.contains(' ') && witherror )
	{
	    uiMSG().error(
		tr("The end-of-header 'word' cannot contain spaces") );
	    return false;
	}
	fd_.eohtoken_ = tok;
    }

    fd_.nrhdrlines_ = nrHdrLines();
    return true;
}


bool uiTableImpDataSel::commit()
{
    if ( !commitHdr(true) )
	return false;

    if ( !fd_.isGood() )
    {
	uiMSG().error( tr("The format definition is incomplete"));
	return false;
    }

    return true;
}


void uiTableImpDataSel::updateSummary()
{
    fmtdeffld_->updateSummary();
    descChg( 0 );
}
