/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          Feb 2006
 RCS:           $Id: uitblimpexpdatasel.cc,v 1.8 2006-11-21 14:00:08 cvsbert Exp $
________________________________________________________________________

-*/

#include "uitblimpexpdatasel.h"
#include "uicombobox.h"
#include "uispinbox.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "tabledef.h"


class uiTableImpDataSelElem : public uiGroup
{
public:

uiTableImpDataSelElem( uiParent* p, Table::FormatInfo& fi, bool ishdr )
    : uiGroup(p,fi.name())
    , fi_(fi)
    , elemfld(0)
    , specfld(0)
    , ishdr_(ishdr)
{
    if ( fi_.nrElements() < 1 )
	return;

    uiComboBox* rightmostcb = 0;
    const CallBack boxcb( mCB(this,uiTableImpDataSelElem,boxChg) );
    if ( fi_.nrElements() > 1 )
    {
	rightmostcb = elemfld = new uiComboBox( this, "Element choice" );
	elemfld->selectionChanged.notify( boxcb );
    }

    if ( ishdr_ )
    {
	rightmostcb = specfld = new uiComboBox( this, "Specify/read" );
	specfld->addItem( "provide" ); specfld->addItem( "read" );
	specfld->setPrefWidthInChar( 9 );
	if ( elemfld )
	    specfld->attach( rightOf, elemfld );
	specfld->selectionChanged.notify( boxcb );
	specfld->setCurrentItem( fi_.selection_.havePos(0) ? 1 : 0 );
    }

    for ( int ielem=0; ielem<fi_.nrElements(); ielem++ )
    {
	const char* elemnm = fi_.elementName( ielem );
	const BufferStringSet* elemdef = fi_.elementDef( ielem );
	if ( elemfld )
	    elemfld->addItem( elemnm );

	BufferString lbltxt;
	if ( !elemdef )
	    lbltxt = elemnm;
	else
	{
	    for ( int ifld=0; ifld<elemdef->size(); ifld++ )
	    {
		if ( ifld ) lbltxt += "/";
		lbltxt += elemdef->get(ifld);
	    }
	}
	uiLabel* lbl = new uiLabel( this, lbltxt );
	if ( rightmostcb )
	    lbl->attach( rightOf, rightmostcb );
	lbls_ += lbl;

	mkColFlds( ielem );
    }

    mainObject()->finaliseDone.notify( boxcb );
}

void mkColFlds( int ielem )
{
    colboxes_ += new ObjectSet<uiSpinBox>;
    if ( ishdr_ )
    {
	rowboxes_ += new ObjectSet<uiSpinBox>;
	inps_ += new ObjectSet<uiGenInput>;
    }

    const int nrflds = fi_.nrElements();
    for ( int ifld=0; ifld<nrflds; ifld++ )
    {
	addBox( ielem, ifld );
	if ( ishdr_ )
	    addInp( ielem, ifld );
    }
}

void addBox( int ielem, int ifld )
{
    ObjectSet<uiSpinBox>& colboxes = *colboxes_[ielem];
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
	*rowboxes_[ielem] += rowspinbox;
    }

    if ( !fi_.selection_.havePos(ifld) )
    {
	colspinbox->setValue( defcol_ );
	if ( !ishdr_ ) defcol_++;
	else if ( rowspinbox )
	    { rowspinbox->setValue( defrow_ ); defrow_++; }
    }
    else
    {
	const RowCol& rc = fi_.selection_.pos_[ifld];
	if ( rowspinbox )
	    rowspinbox->setValue( rc.r() + 1 );
	colspinbox->setValue( rc.c() + 1 );
    }

    uiSpinBox* leftbox = rowspinbox ? rowspinbox : colspinbox;

    if ( ifld != 0 )
	leftbox->attach( rightOf, colboxes[ifld-1] );
    else
    {
	leftbox->attach( rightOf, lbls_[ielem] );
	if ( ielem == 0 )
	    setHAlignObj( leftbox );
    }

    if ( rowspinbox )
	colspinbox->attach( rightOf, rowspinbox );
}

void addInp( int ielem, int ifld )
{
    ObjectSet<uiGenInput>& colinps = *inps_[ielem];
    uiGenInput* inp = new uiGenInput( this, "", fi_.selection_.getVal(ifld) );
    colinps += inp;

    if ( ifld )
	inp->attach( rightOf, colinps[ifld-1] );
    else
	inp->attach( rightOf, lbls_[ielem] );
}


void boxChg( CallBacker* )
{
    if ( !elemfld && !specfld ) return;

    const int selelemidx = elemfld ? elemfld->currentItem() : 0;
    const bool isspec = specfld && specfld->currentItem() == 0;

    for ( int ielem=0; ielem<fi_.nrElements(); ielem++ )
    {
	const bool isselelem = ielem == selelemidx;
	lbls_[ielem]->display( isselelem );

	ObjectSet<uiSpinBox>& colboxes = *colboxes_[ielem];
	ObjectSet<uiSpinBox>* rowboxes = ielem < rowboxes_.size()
	    			       ? rowboxes_[ielem] : 0;
	for ( int ifld=0; ifld<colboxes.size(); ifld++ )
	{
	    colboxes[ifld]->display( isselelem && !isspec );
	    (*rowboxes)[ifld]->display( isselelem && !isspec );
	}
	if ( ishdr_ )
	{
	    ObjectSet<uiGenInput>& colinps = *inps_[ielem];
	    for ( int ifld=0; ifld<colinps.size(); ifld++ )
		colinps[ifld]->display( isselelem && isspec );
	}
    }
}

bool commit()
{
    const int selelem = elemfld ? elemfld->currentItem() : 0;
    const bool readelem = !specfld || specfld->currentItem() == 1;
    if ( !readelem && !fi_.isOptional() )
    {
	ObjectSet<uiGenInput>& colinps = *inps_[selelem];
	for ( int idx=0; idx<colinps.size(); idx++ )
	{
	    if ( ! *colinps[idx]->text() )
	    {
		errmsg_ = "Please enter a value for ";
		errmsg_ += fi_.elementName(selelem);
		return false;
	    }
	}
    }

    fi_.selection_.elem_ = selelem;
    fi_.selection_.pos_.erase();
    fi_.selection_.vals_.erase();

    if ( readelem )
    {
	ObjectSet<uiSpinBox>& colboxes = *colboxes_[selelem];
	ObjectSet<uiSpinBox>* rowboxes = rowboxes_.size() > selelem
	    			       ? rowboxes_[selelem] : 0;
	for ( int idx=0; idx<colboxes.size(); idx++ )
	{
	    RowCol rc( -1, colboxes[idx]->getValue() );
	    if ( rowboxes )
		rc.r() = (*rowboxes)[idx]->getValue();
	    fi_.selection_.pos_ += rc;
	}
    }
    else
    {
	ObjectSet<uiGenInput>& colinps = *inps_[selelem];
	for ( int idx=0; idx<colinps.size(); idx++ )
	    fi_.selection_.vals_.add( colinps[idx]->text() );
    }

    return true;
}

    Table::FormatInfo&			fi_;
    bool				ishdr_;
    BufferString			errmsg_;

    uiComboBox*				elemfld;
    uiComboBox*				specfld;
    ObjectSet< ObjectSet<uiSpinBox> >	colboxes_;
    ObjectSet< ObjectSet<uiSpinBox> >	rowboxes_;
    ObjectSet< ObjectSet<uiGenInput> >	inps_;
    ObjectSet< uiLabel >		lbls_;

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
    uiGroup* hfldsgrp = mkElemFlds( fd_.headerinfos_, hdrelems_, true );

    int maxhdrline = 0;
    for ( int idx=0; idx<fd_.headerinfos_.size(); idx++ )
    {
	const Table::FormatInfo& fi = *fd_.headerinfos_[idx];
	if ( fi.selection_.pos_.size() > idx
	  && fi.selection_.pos_[idx].r() > maxhdrline )
	    maxhdrline = fi.selection_.pos_[idx].r();
    }
    if ( fd_.nrhdrlines_ < maxhdrline )
	fd_.nrhdrlines_ = maxhdrline;

    // No support for setting tokencol_ (yet?) ...
    BufferString valstr( fd_.token_ );
    if ( valstr.isEmpty() ) valstr = "0";
    if ( fd_.nrhdrlines_ > 0 ) valstr = fd_.nrhdrlines_;
    hdrendfld = new uiGenInput( this, "Header stops after (nr lines, or token)",
				valstr );
    if ( hfldsgrp )
	hdrendfld->attach( alignedBelow, hfldsgrp );

    uiGroup* bfldsgrp = mkElemFlds( fd_.bodyinfos_, bodyelems_, false );
    if ( bfldsgrp )
	bfldsgrp->attach( alignedBelow, hdrendfld );

    setHAlignObj( hdrendfld );
}


uiGroup* uiTableImpDataSel::mkElemFlds( ObjectSet<Table::FormatInfo>& infos,
					ObjectSet<uiTableImpDataSelElem>& elms,
					bool ishdr )
{
    if ( infos.size() < 1 )
	return 0;

    uiTableImpDataSelElem::initDefs();
    uiGroup* grp = new uiGroup( this, ishdr ? "Header fields" : "Body fields" );
    for ( int idx=0; idx<infos.size(); idx++ )
    {
	Table::FormatInfo& fi = *infos[idx];
	uiTableImpDataSelElem* elem = new uiTableImpDataSelElem(grp,fi,ishdr);
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
