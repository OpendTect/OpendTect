/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          Feb 2006
 RCS:           $Id: uitblimpexpdatasel.cc,v 1.16 2007-01-03 17:50:55 cvsbert Exp $
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
#include "uimsg.h"
#include "pixmap.h"
#include "ioman.h"
#include "tabledef.h"

#define mChoiceBoxWidth 10


class uiTableTargetInfoEd : public uiGroup
{
public:

uiTableTargetInfoEd( uiParent* p, Table::TargetInfo& tinf, bool ishdr )
    : uiGroup(p,tinf.name())
    , tinf_(tinf)
    , formfld_(0)
    , specfld_(0)
    , ishdr_(ishdr)
{
    if ( tinf_.nrForms() < 1 )
	return;

    const CallBack boxcb( mCB(this,uiTableTargetInfoEd,boxChg) );
    if ( tinf_.nrForms() > 1 )
    {
	formfld_ = new uiComboBox( this, "Form choice" );
	formfld_->selectionChanged.notify( boxcb );
    }

    if ( ishdr_ )
    {
	specfld_ = new uiComboBox( choicegrp_, "Specify/read" );
	specfld_->addItem( "provide" ); specfld_->addItem( "read" );
	specfld_->setPrefWidthInChar( mChoiceBoxWidth );
	specfld_->selectionChanged.notify( boxcb );
	specfld_->setCurrentItem( tinf_.selection_.havePos(0) ? 1 : 0 );
    }

    uiLabel* lbl = new uiLabel( this, tinf_.name() );
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
	rowspinbox->setInterval( 1, 999, 1 );
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
    const bool isspec = specfld_ && specfld_->currentItem() == 0;

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
    const bool doread = !specfld_ || specfld_->currentItem() == 1;
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
	: uiDialog(ds,uiDialog::Setup("Format definition","Specify format",
		    		     "0.0.0"))
	, ds_(*ds)
	, fd_(ds->desc())
	, hdrinpgrp_(0)
	, bodyinpgrp_(0)
{
    elemgrp_ = new uiGroup( this, "Elem group" );
    mkElemFlds( true );
    mkElemFlds( false );
    if ( hdrinpgrp_ )
	bodyinpgrp_->attach( alignedBelow, hdrinpgrp_ );

    uiSeparator* sep = new uiSeparator( this, "V sep", false );
    sep->attach( stretchedRightTo, elemgrp_ );

    uiGroup* utilsgrp = new uiGroup( this, "Utils group" );
    uiButtonGroup* fmtiogrp = new uiButtonGroup( utilsgrp, "" );
    uiToolButton* button = new uiToolButton( fmtiogrp, "Open button",
	    			ioPixmap("openset.png"),
				mCB(this,uiTableFormatDescFldsEd,openFmt) );
    button->setToolTip( "Open existing format" );
    button = new uiToolButton( fmtiogrp, "Save button",
	    			ioPixmap("saveset.png"),
				mCB(this,uiTableFormatDescFldsEd,saveFmt) );
    button->setToolTip( "Save format" );

    uiButtonGroup* unitsgrp = new uiButtonGroup( utilsgrp, "" );
    button = new uiToolButton( unitsgrp, "Units button",
	    			ioPixmap("unitsofmeasure.png"),
				mCB(this,uiTableFormatDescFldsEd,handleUnits) );
    button->setToolTip( "Specify units of measure" );
    unitsgrp->attach( alignedBelow, fmtiogrp );

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
	uiTableTargetInfoEd::choicegrp_ = new uiGroup( this, "Choice group" );
	elemgrp_->attach( rightOf, uiTableTargetInfoEd::choicegrp_ );
    }

    for ( int idx=0; idx<infos.size(); idx++ )
    {
	uiTableTargetInfoEd* elem = new uiTableTargetInfoEd( grp,
							*infos[idx], ishdr );
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


void uiTableFormatDescFldsEd::openFmt( CallBacker* )
{
    uiMSG().error( "The ability to retrieve pre- or user-defined formats\n"
	           "is under construction" );
}


void uiTableFormatDescFldsEd::saveFmt( CallBacker* )
{
    uiMSG().error( "The ability to save user-defined formats\n"
	           "is under construction" );
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
    : uiCompoundParSel( p, "Format definition", "Define" )
    , impsel_(*p)
{
    butPush.notify( mCB(this,uiTableFmtDescFldsParSel,doDlg) );
}

BufferString getSummary() const
{
    BufferString ret;
    if ( !impsel_.desc().isGood() )
	ret = "<Not defined>";
    else if ( !impsel_.storID().isEmpty() )
	ret = IOM().nameOf( impsel_.storID() );
    else
	ret = "<Defined>";
    return ret;
}

void doDlg( CallBacker* )
{
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
    hdrtypefld_ = new uiGenInput( this, "File header",
	    			  StringListInpSpec(hdrtyps) );
    hdrlinesfld_ = new uiGenInput( this, "Header size (number of lines)",
	    			   IntInpSpec(fd_.nrHdrLines()) );
    hdrlinesfld_->attach( alignedBelow, hdrtypefld_ );
    hdrtokfld_ = new uiGenInput( this, "End-of-header 'word'",
	    			 StringInpSpec(fd_.token_) );
    hdrtokfld_->attach( alignedBelow, hdrtypefld_ );

    fmtdeffld = new uiTableFmtDescFldsParSel( this );
    fmtdeffld->attach( alignedBelow, hdrlinesfld_ );
    setHAlignObj( hdrtypefld_ );
}


bool uiTableImpDataSel::commit()
{
    const int htyp = hdrtypefld_->getIntValue();
    if ( htyp == 0 )
	fd_.nrhdrlines_ = 0;
    else if ( htyp == 1 )
	fd_.nrhdrlines_ = hdrlinesfld_->getIntValue();
    else
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
	fd_.nrhdrlines_ = -1;
	fd_.token_ = tok;
    }

    if ( !fd_.isGood() )
    {
	uiMSG().error( "The format definition is incomplete");
	return false;
    }

    return true;
}
