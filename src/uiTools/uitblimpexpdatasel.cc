/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          Feb 2006
 RCS:           $Id: uitblimpexpdatasel.cc,v 1.21 2007-01-09 16:38:12 cvsbert Exp $
________________________________________________________________________

-*/

#include "uitblimpexpdatasel.h"
#include "uicombobox.h"
#include "uispinbox.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "uibutton.h"
#include "uiseparator.h"
#include "uibuttongroup.h"
#include "uidialog.h"
#include "uicompoundparsel.h"
#include "uiselsimple.h"
#include "uimsg.h"
#include "pixmap.h"
#include "ioman.h"
#include "iopar.h"
#include "tabledef.h"
#include "tableascio.h"

#define mChoiceBoxWidth 10


class uiTableTargetInfoEd : public uiGroup
{
public:

uiTableTargetInfoEd( uiParent* p, Table::TargetInfo& tinf, bool ishdr,
		     int nrlns )
    : uiGroup(p,tinf.name())
    , tinf_(tinf)
    , formfld_(0)
    , specfld_(0)
    , ishdr_(ishdr)
    , nrhdrlns_(nrlns)
{
    if ( tinf_.nrForms() < 1 )
	return;

    const CallBack boxcb( mCB(this,uiTableTargetInfoEd,boxChg) );
    if ( tinf_.nrForms() > 1 )
    {
	formfld_ = new uiComboBox( this, "Form choice" );
	formfld_->selectionChanged.notify( boxcb );
    }

    if ( ishdr_ && choicegrp_ )
    {
	specfld_ = new uiComboBox( choicegrp_, "Specify/read" );
	specfld_->addItem( "provide" ); specfld_->addItem( "read" );
	specfld_->setPrefWidthInChar( mChoiceBoxWidth );
	specfld_->selectionChanged.notify( boxcb );
	specfld_->setCurrentItem( tinf_.selection_.havePos(0) ? 1 : 0 );
    }

    BufferString lbltxt( tinf_.isOptional() ? "[" : "" );
    lbltxt += tinf_.name();
    if ( tinf_.isOptional() ) lbltxt += "]";
    uiLabel* lbl = new uiLabel( this, lbltxt );
    if ( formfld_ )
	lbl->attach( rightOf, formfld_ );
    rightmostleftfld_ = formfld_ ? (uiObject*)formfld_ : (uiObject*)lbl;

    for ( int iform=0; iform<tinf_.nrForms(); iform++ )
    {
	const Table::TargetInfo::Form& form = tinf_.form( iform );
	if ( formfld_ )
	    formfld_->addItem( form.name() );
	mkColFlds( iform );
    }

    mainObject()->finaliseDone.notify( boxcb );
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
	inps_ += new ObjectSet<uiGenInput>;
    }

    const int nrflds = tinf_.nrForms();
    for ( int ifld=0; ifld<nrflds; ifld++ )
    {
	addBox( iform, ifld );
	if ( ishdr_ )
	    addInp( iform, ifld );
    }
}

void addBox( int iform, int ifld )
{
    ObjectSet<uiSpinBox>& colboxes = *colboxes_[iform];
    uiSpinBox* colspinbox = new uiSpinBox( this );
    colspinbox->setInterval( 1, 999, 1 );
    colspinbox->setPrefix( "col:" );
    colboxes += colspinbox;

    uiSpinBox* rowspinbox = 0;
    if ( ishdr_ )
    {
	rowspinbox = new uiSpinBox( this );
	rowspinbox->setInterval( 1, nrhdrlns_ > 0 ? nrhdrlns_ : 999, 1 );
	rowspinbox->setPrefix( "row:" );
	*rowboxes_[iform] += rowspinbox;
    }

    if ( !tinf_.selection_.havePos(ifld) )
    {
	colspinbox->setValue( defcol_ );
	if ( !ishdr_ ) defcol_++;
	else if ( rowspinbox )
	    { rowspinbox->setValue( defrow_ ); defrow_++; }
    }
    else
    {
	const RowCol& rc = tinf_.selection_.elems_[ifld].pos_;
	if ( rowspinbox )
	    rowspinbox->setValue( rc.r() + 1 ); // Users tend to start at 1
	colspinbox->setValue( rc.c() + 1 );
    }

    uiSpinBox* leftbox = rowspinbox ? rowspinbox : colspinbox;

    if ( ifld != 0 )
	leftbox->attach( rightOf, colboxes[ifld-1] );
    else
    {
	leftbox->attach( rightOf, rightmostleftfld_ );
	if ( iform == 0 )
	    setHAlignObj( leftbox );
    }

    if ( rowspinbox )
	colspinbox->attach( rightOf, rowspinbox );
}

void addInp( int iform, int ifld )
{
    ObjectSet<uiGenInput>& colinps = *inps_[iform];
    uiGenInput* inp = new uiGenInput( this, "",
			  *tinf_.form(iform).specs_[ifld] );
    colinps += inp;

    if ( ifld )
	inp->attach( rightOf, colinps[ifld-1] );
    else
	inp->attach( rightOf, rightmostleftfld_ );
}


void boxChg( CallBacker* )
{
    if ( !formfld_ && !specfld_ ) return;

    const int selformidx = formfld_ ? formfld_->currentItem() : 0;
    const bool isspec = !specfld_ || specfld_->currentItem() == 0;

    for ( int iform=0; iform<tinf_.nrForms(); iform++ )
    {
	const bool isselform = iform == selformidx;

	ObjectSet<uiSpinBox>& colboxes = *colboxes_[iform];
	ObjectSet<uiSpinBox>* rowboxes = iform < rowboxes_.size()
	    			       ? rowboxes_[iform] : 0;
	for ( int ifld=0; ifld<colboxes.size(); ifld++ )
	{
	    colboxes[ifld]->display( isselform && !isspec );
	    (*rowboxes)[ifld]->display( isselform && !isspec );
	}
	if ( ishdr_ )
	{
	    ObjectSet<uiGenInput>& colinps = *inps_[iform];
	    for ( int ifld=0; ifld<colinps.size(); ifld++ )
		colinps[ifld]->display( isselform && isspec );
	}
    }
}

bool commit()
{
    const int formnr = formfld_ ? formfld_->currentItem() : 0;
    const bool doread = !ishdr_ || (specfld_ && specfld_->currentItem() == 1);
    if ( !doread && !tinf_.isOptional() )
    {
	ObjectSet<uiGenInput>& colinps = *inps_[formnr];
	for ( int idx=0; idx<colinps.size(); idx++ )
	{
	    if ( colinps[idx]->isUndef() )
	    {
		errmsg_ = "Value missing for ";
		errmsg_ += tinf_.form(formnr).name();
		return false;
	    }
	}
    }

    tinf_.selection_.form_ = formnr;
    tinf_.selection_.elems_.erase();

    if ( doread )
    {
	ObjectSet<uiSpinBox>& colboxes = *colboxes_[formnr];
	ObjectSet<uiSpinBox>* rowboxes = rowboxes_.size() > formnr
	    			       ? rowboxes_[formnr] : 0;
	for ( int idx=0; idx<colboxes.size(); idx++ )
	{
	    RowCol rc( 0, colboxes[idx]->getValue() );
	    if ( rowboxes )
		rc.r() = (*rowboxes)[idx]->getValue();
	    if ( mIsUdf(rc.r()) || mIsUdf(rc.c()) )
	    {
		errmsg_ = "Missing position in the file for ";
		errmsg_ += tinf_.form(formnr).name();
		return false;
	    }
	    rc.r()--; rc.c()--; // Users tend to start at 1
	    tinf_.selection_.elems_ += Table::TargetInfo::Selection::Elem( rc );
	}
    }
    else
    {
	ObjectSet<uiGenInput>& colinps = *inps_[formnr];
	for ( int idx=0; idx<colinps.size(); idx++ )
	    tinf_.selection_.elems_ +=
		Table::TargetInfo::Selection::Elem( colinps[idx]->text() );
    }

    return true;
}

    Table::TargetInfo&			tinf_;
    bool				ishdr_;
    int					nrhdrlns_;
    BufferString			errmsg_;

    uiComboBox*				formfld_;
    uiComboBox*				specfld_;
    ObjectSet< ObjectSet<uiSpinBox> >	colboxes_;
    ObjectSet< ObjectSet<uiSpinBox> >	rowboxes_;
    ObjectSet< ObjectSet<uiGenInput> >	inps_;
    uiObject*				rightmostleftfld_;

    static uiGroup*			choicegrp_;
    static int				defrow_;
    static int				defcol_;
    static void				initDefs()
    					{ defrow_ = defcol_ = 1; }

};

int uiTableTargetInfoEd::defrow_ = 1;
int uiTableTargetInfoEd::defcol_ = 1;
uiGroup* uiTableTargetInfoEd::choicegrp_ = 0;


class uiTableFormatDescFldsEd : public uiDialog
{
public:

    				uiTableFormatDescFldsEd(uiTableImpDataSel*);

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

    void			mkElemFlds(bool);
    void			openFmt(CallBacker*);
    void			saveFmt(CallBacker*);
    void			handleUnits(CallBacker*);

    bool			acceptOK(CallBacker*);
};




uiTableFormatDescFldsEd::uiTableFormatDescFldsEd( uiTableImpDataSel* ds )
	: uiDialog(ds,uiDialog::Setup(	"Information definition",
					"Specify necessary information",
					"0.0.0"))
	, ds_(*ds)
	, fd_(ds->desc())
	, hdrinpgrp_(0)
	, bodyinpgrp_(0)
	, nrhdrlines_(ds->nrHdrLines())
{
    elemgrp_ = new uiGroup( this, "Elem group" );
    uiTableTargetInfoEd::choicegrp_ = 0;

    mkElemFlds( true );
    mkElemFlds( false );
    if ( hdrinpgrp_ )
	bodyinpgrp_->attach( alignedBelow, hdrinpgrp_ );

    uiSeparator* sep = new uiSeparator( this, "V sep", false );
    sep->attach( stretchedRightTo, elemgrp_ );

    uiButtonGroup* utilsgrp = new uiButtonGroup( this, "" );
    uiToolButton* button = new uiToolButton( utilsgrp, "Save button",
	    			ioPixmap("savefmt.png"),
				mCB(this,uiTableFormatDescFldsEd,saveFmt) );
    button->setToolTip( "Save format" );

    button = new uiToolButton( utilsgrp, "Units button",
	    			ioPixmap("unitsofmeasure.png"),
				mCB(this,uiTableFormatDescFldsEd,handleUnits) );
    button->setToolTip( "Specify units of measure" );

    utilsgrp->attach( ensureRightOf, sep );
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

    for ( int idx=0; idx<infos.size(); idx++ )
    {
	uiTableTargetInfoEd* elem = new uiTableTargetInfoEd( grp, *infos[idx],
						     ishdr, nrhdrlines_ );
	elms += elem;
	if ( idx )
	    elem->doAttach( elms[idx-1] );
	else
	    grp->setHAlignObj( elem );
    }
}


#define mDoCommit(elms) \
    for ( int idx=0; idx<elms.size(); idx++ ) \
    { \
	if ( !elms[idx]->commit() ) \
	{ \
	    uiMSG().error( elms[idx]->errmsg_ ); \
	    return false; \
	} \
    }

bool uiTableFormatDescFldsEd::acceptOK( CallBacker* )
{
    mDoCommit(hdrelems_)
    mDoCommit(bodyelems_)
    return true;
}


void uiTableFormatDescFldsEd::saveFmt( CallBacker* )
{
    BufferStringSet nms;
    Table::FFR().getFormats( fd_.name(), nms );

    uiGetObjectName::Setup setup( "Save format", nms );
    setup.inptxt( "Name for format" )
	 .deflt( ds_.fmtname_ )
	 .dlgtitle( "Enter a name for the format" );
    uiGetObjectName dlg( this, setup );
    static const char* strs[] = { "In survey only",
				  "In all surveys",
				  "By me only" };
    uiGenInput* srcfld = new uiGenInput( &dlg, "Store for use",
	    				 StringListInpSpec(strs) );
    srcfld->attach( alignedBelow, dlg.inpFld() );
    if ( dlg.go() )
    {
	const char* fmtnm = dlg.text();
	IOPar* newiop = new IOPar;
	fd_.fillPar( *newiop );
	Repos::Source src = (Repos::Source)(srcfld->getIntValue()+1);
	Table::FFR().set( fd_.name(), fmtnm, newiop, src );
	if ( !Table::FFR().write(src) )
	    uiMSG().error( "Cannot write format" );
	else
	{
	    ds_.storediop_.clear(); fd_.fillPar( ds_.storediop_ );
	    ds_.fmtname_ = fmtnm;
	}
    }
}


class uiTableImpDataSelUnits : public uiDialog
{
public:

uiTableImpDataSelUnits( uiParent* p, Table::FormatDesc& fd )
    : uiDialog(p,uiDialog::Setup(fd.name(),""))
    , fd_(fd)
{
    new uiLabel( this, "Units-of-measure handling\nis under construction" );
}

    Table::FormatDesc&	fd_;

};


void uiTableFormatDescFldsEd::handleUnits( CallBacker* )
{
    uiTableImpDataSelUnits dlg( this, fd_ );
    dlg.go();
}


class uiTableFmtDescFldsParSel : public uiCompoundParSel
{
public:

uiTableFmtDescFldsParSel( uiTableImpDataSel* p )
    : uiCompoundParSel( p, "Information content", "Define" )
    , impsel_(*p)
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
    if ( !impsel_.commitHdr() )
	return;

    uiTableFormatDescFldsEd dlg( &impsel_ );
    dlg.go();
}

    uiTableImpDataSel&	impsel_;

};


uiTableImpDataSel::uiTableImpDataSel( uiParent* p, Table::FormatDesc& fd )
	: uiGroup(p,fd.name())
	, fd_(fd)
{
    static const char* hdrtyps[] = { "No header", "Fixed size", "Variable", 0 };
    const CallBack typchgcb = mCB(this,uiTableImpDataSel,typChg);
    hdrtypefld_ = new uiGenInput( this, "File header",
	    			  StringListInpSpec(hdrtyps) );
    hdrtypefld_->valuechanged.notify( typchgcb );

    uiToolButton* button = new uiToolButton( this, "Open button",
	    			ioPixmap("openfmt.png"),
				mCB(this,uiTableImpDataSel,openFmt) );
    button->setToolTip( "Open existing format" );
    button->setPrefWidthInChar( 6 );
    button->attach( rightOf, hdrtypefld_ );

    int nrlns = fd_.nrHdrLines(); if ( nrlns < 1 ) nrlns = 1;
    hdrlinesfld_ = new uiGenInput( this, "Header size (number of lines)",
	    			   IntInpSpec(nrlns) );
    hdrlinesfld_->attach( alignedBelow, hdrtypefld_ );
    hdrtokfld_ = new uiGenInput( this, "End-of-header 'word'",
	    			 StringInpSpec(fd_.token_) );
    hdrtokfld_->attach( alignedBelow, hdrtypefld_ );

    fmtdeffld_ = new uiTableFmtDescFldsParSel( this );
    fmtdeffld_->attach( alignedBelow, hdrlinesfld_ );

    setHAlignObj( hdrtypefld_ );
    mainObject()->finaliseDone.notify( typchgcb );
}


void uiTableImpDataSel::typChg( CallBacker* )
{
    const int htyp = hdrtypefld_->getIntValue();
    hdrlinesfld_->display( htyp == 1 );
    hdrtokfld_->display( htyp == 2 );
}


void uiTableImpDataSel::openFmt( CallBacker* )
{
    BufferStringSet avfmts;
    Table::FFR().getFormats( fd_.name(), avfmts );
    if ( avfmts.size() < 1 )
    {
	uiMSG().error( "No pre- or user-defined formats available" );
	return;
    }

    uiSelectFromList::Setup setup( "Retrieve data format", avfmts );
    setup.dlgtitle( "Select a format to retrieve" );
    uiSelectFromList dlg( this, setup );
    if ( !dlg.go() || dlg.selection() < 0 )
	return;

    const BufferString& fmtnm = avfmts.get( dlg.selection() );
    const IOPar* iop = Table::FFR().get( fd_.name(), fmtnm );
    if ( !iop ) return; //Huh?

    fd_.usePar( *iop );
    hdrtokfld_->setText( fd_.token_ );
    const int nrlns = fd_.nrHdrLines();
    hdrtypefld_->setValue( nrlns < 0 ? 2 : (nrlns == 0 ? 1 : 0) );
    hdrlinesfld_->setValue( nrlns < 1 ? 1 : nrlns );

    typChg( 0 );
    storediop_.clear(); fd_.fillPar( storediop_ );
    fmtname_ = fmtnm;
    fmtdeffld_->updateSummary();
}


int uiTableImpDataSel::nrHdrLines() const
{
    const int htyp = hdrtypefld_->getIntValue();
    return htyp == 0 ? 0 : (htyp == 2 ? -1 : hdrlinesfld_->getIntValue());
}


bool uiTableImpDataSel::commitHdr()
{
    const int htyp = hdrtypefld_->getIntValue();
    if ( htyp == 2 )
    {
	BufferString tok = hdrtokfld_->text();
	if ( tok.isEmpty() )
	{
	    uiMSG().error( "Please enter the string marking the end-of-header");
	    return false;
	}
	if ( strchr( tok.buf(), ' ' ) )
	{
	    uiMSG().error( "The end-of-header 'word' cannot contain spaces");
	    return false;
	}
	fd_.token_ = tok;
    }

    fd_.nrhdrlines_ = nrHdrLines();
    return true;
}


bool uiTableImpDataSel::commit()
{
    if ( !commitHdr() )
	return false;

    if ( !fd_.isGood() )
    {
	uiMSG().error( "The format definition is incomplete");
	return false;
    }

    return true;
}
