/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          Feb 2006
 RCS:           $Id: uitblimpexpdatasel.cc,v 1.11 2006-12-19 18:19:15 cvsbert Exp $
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
#include "uimsg.h"
#include "pixmap.h"
#include "tabledef.h"


class uiTableImpDataSelElem : public uiGroup
{
public:

uiTableImpDataSelElem( uiParent* p, Table::TargetInfo& tinf, bool ishdr )
    : uiGroup(p,tinf.name())
    , tinf_(tinf)
    , formfld(0)
    , specfld(0)
    , ishdr_(ishdr)
{
    if ( tinf_.nrForms() < 1 )
	return;

    uiComboBox* leftmostcb = 0;
    const CallBack boxcb( mCB(this,uiTableImpDataSelElem,boxChg) );
    if ( tinf_.nrForms() > 1 )
    {
	leftmostcb = formfld = new uiComboBox( this, "Form choice" );
	formfld->selectionChanged.notify( boxcb );
    }

    if ( ishdr_ )
    {
	leftmostcb = specfld = new uiComboBox( this, "Specify/read" );
	specfld->addItem( "provide" ); specfld->addItem( "read" );
	specfld->setPrefWidthInChar( 9 );
	if ( formfld )
	    specfld->attach( rightOf, formfld );
	specfld->selectionChanged.notify( boxcb );
	specfld->setCurrentItem( tinf_.selection_.havePos(0) ? 1 : 0 );
    }

    uiLabel* lbl = new uiLabel( this, tinf_.name() );
    if ( leftmostcb )
    {
	lbl->attach( rightOf, leftmostcb );
	if ( formfld && leftmostcb != formfld )
	    formfld->attach( rightOf, lbl );
    }
    rightmostleftfld_ = formfld ? (uiObject*)formfld : (uiObject*)lbl;

    for ( int iform=0; iform<tinf_.nrForms(); iform++ )
    {
	const Table::TargetInfo::Form& form = tinf_.form( iform );
	if ( formfld )
	    formfld->addItem( form.name() );
	mkColFlds( iform );
    }

    mainObject()->finaliseDone.notify( boxcb );
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
	const RowCol& rc = tinf_.selection_.pos_[ifld];
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
    if ( !formfld && !specfld ) return;

    const int selformidx = formfld ? formfld->currentItem() : 0;
    const bool isspec = specfld && specfld->currentItem() == 0;

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
    const int formnr = formfld ? formfld->currentItem() : 0;
    const bool doread = !specfld || specfld->currentItem() == 1;
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
    tinf_.selection_.pos_.erase();
    tinf_.selection_.vals_.erase();

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
	    tinf_.selection_.pos_ += rc;
	}
    }
    else
    {
	ObjectSet<uiGenInput>& colinps = *inps_[formnr];
	for ( int idx=0; idx<colinps.size(); idx++ )
	    tinf_.selection_.vals_.add( colinps[idx]->text() );
    }

    return true;
}

    Table::TargetInfo&			tinf_;
    bool				ishdr_;
    BufferString			errmsg_;

    uiComboBox*				formfld;
    uiComboBox*				specfld;
    ObjectSet< ObjectSet<uiSpinBox> >	colboxes_;
    ObjectSet< ObjectSet<uiSpinBox> >	rowboxes_;
    ObjectSet< ObjectSet<uiGenInput> >	inps_;
    uiObject*				rightmostleftfld_;

    static int				defrow_;
    static int				defcol_;
    static void				initDefs()
    					{ defrow_ = defcol_ = 1; }

};

int uiTableImpDataSelElem::defrow_ = 1;
int uiTableImpDataSelElem::defcol_ = 1;


uiTableImpDataSel::uiTableImpDataSel( uiParent* p, Table::FormatDesc& fd )
	: uiGroup(p,fd.name())
	, fd_(fd)
	, errmsg_(0)
{
    uiGroup* leftgrp = new uiGroup( this, "Left group" );
    uiGroup* hfldsgrp = mkElemFlds( leftgrp, fd_.headerinfos_, hdrelems_, true);

    int maxhdrline = 0;
    for ( int idx=0; idx<fd_.headerinfos_.size(); idx++ )
    {
	const Table::TargetInfo& tinf = *fd_.headerinfos_[idx];
	if ( tinf.selection_.pos_.size() > idx
	  && tinf.selection_.pos_[idx].r() > maxhdrline )
	    maxhdrline = tinf.selection_.pos_[idx].r();
    }
    if ( fd_.nrhdrlines_ < maxhdrline )
	fd_.nrhdrlines_ = maxhdrline;

    // No support for setting tokencol_ (yet?) ...
    BufferString valstr( fd_.token_ );
    if ( valstr.isEmpty() ) valstr = "0";
    if ( fd_.nrhdrlines_ > 0 ) valstr = fd_.nrhdrlines_;
    hdrendfld = new uiGenInput( leftgrp,
		    "Header stops after (nr lines, or token)", valstr );
    if ( hfldsgrp )
	hdrendfld->attach( alignedBelow, hfldsgrp );

    uiGroup* bfldsgrp = mkElemFlds( leftgrp, fd_.bodyinfos_, bodyelems_, false);
    if ( bfldsgrp )
	bfldsgrp->attach( alignedBelow, hdrendfld );

    setHAlignObj( hdrendfld );

    uiSeparator* sep = new uiSeparator( this, "V sep", false );
    sep->attach( stretchedRightTo, leftgrp );

    uiButtonGroup* bgrp = new uiButtonGroup( this, "Formats" );
    uiToolButton* button = new uiToolButton( bgrp, "Open button",
	    			ioPixmap("openset.png"),
				mCB(this,uiTableImpDataSel,openFmt) );
    button->setToolTip( "Open existing format" );
    button = new uiToolButton( bgrp, "Save button",
	    			ioPixmap("saveset.png"),
				mCB(this,uiTableImpDataSel,saveFmt) );
    button->setToolTip( "Save format" );
    bgrp->attach( rightTo, leftgrp );
    bgrp->attach( ensureRightOf, sep );
}


uiGroup* uiTableImpDataSel::mkElemFlds( uiGroup* par,
					ObjectSet<Table::TargetInfo>& infos,
					ObjectSet<uiTableImpDataSelElem>& elms,
					bool ishdr )
{
    if ( infos.size() < 1 )
	return 0;

    uiTableImpDataSelElem::initDefs();
    uiGroup* grp = new uiGroup( par, ishdr ? "Header fields" : "Body fields" );
    for ( int idx=0; idx<infos.size(); idx++ )
    {
	Table::TargetInfo& tinf = *infos[idx];
	uiTableImpDataSelElem* elem = new uiTableImpDataSelElem(grp,tinf,ishdr);
	elms += elem;
	if ( idx )
	    elem->attach( alignedBelow, elms[idx-1] );
	else
	    grp->setHAlignObj( elem );
    }
    return grp;
}


#define mDoCommit(elms) \
    for ( int idx=0; idx<elms.size(); idx++ ) \
    { \
	if ( !elms[idx]->commit() ) \
	{ \
	    errmsg_ = elms[idx]->errmsg_.buf(); \
	    return false; \
	} \
    }

bool uiTableImpDataSel::commit()
{
    mDoCommit(hdrelems_)
    mDoCommit(bodyelems_)

    BufferString txt = hdrendfld->text();
    if ( txt.isEmpty() )
	fd_.nrhdrlines_ = 0;
    else if ( isNumberString(txt.buf(),YES) )
	fd_.nrhdrlines_ = atoi( txt.buf() );
    else
    {
	fd_.nrhdrlines_ = -1;
	fd_.token_ = txt;
    }

    return true;
}


void uiTableImpDataSel::openFmt( CallBacker* )
{
    uiMSG().error( "The ability to retrieve pre- or user-defined formats\n"
	           "is under construction" );
}


void uiTableImpDataSel::saveFmt( CallBacker* )
{
    uiMSG().error( "The ability to save or user-defined formats\n"
	           "is under construction" );
}
