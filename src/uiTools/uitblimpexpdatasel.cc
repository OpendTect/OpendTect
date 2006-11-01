/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          Feb 2006
 RCS:           $Id: uitblimpexpdatasel.cc,v 1.2 2006-11-01 17:06:40 cvsbert Exp $
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
{
    if ( fi.elements_.size() < 1 )
	return;
    else if ( fi.elements_.size() > 1 )
	elemfld = new uiComboBox( this, "Element choice" );

    BufferStringSet lbls;
    for ( int ielem=0; ielem<fi.elements_.size(); ielem++ )
    {
	const BufferStringSet& flds = *fi.elements_[ielem];
	BufferString txt;
	for ( int ifld=0; ifld<flds.size(); ifld++ )
	{
	    txt += flds.get(ifld);
	    txt += (ifld < flds.size()-1) ? "/" : " ";
	}
	txt += ishdr ? "row.col" : "column";
	if ( flds.size() > 1 ) txt += ishdr ? "'s" : "s";
	uiLabel* lbl = new uiLabel( this, txt );
	if ( elemfld )
	{
	    lbl->attach( rightOf, elemfld );
	    elemfld->addItem( fi.name() );
	}

	ObjectSet<uiSpinBox>& colboxes = *new ObjectSet<uiSpinBox>;
	boxes_ += &colboxes;
	const bool havesel = fi.selection_.pos_.size() > 0;
	for ( int ifld=0; ifld<flds.size(); ifld++ )
	{
	    uiSpinBox* fld = new uiSpinBox( this );
	    if ( ishdr ) fld->setNrDecimals( 1 );
	    colboxes += fld;
	    if ( !havesel )
		fld->setValue( ideffld_++ );
	    else if ( fi.selection_.pos_.size() > ifld )
	    {
		const RowCol& rc = fi.selection_.pos_[ifld];
		if ( ishdr )
		    fld->setValue( (float)(rc.r() + 0.1 * rc.c()) );
		else
		    fld->setValue( rc.c() );
	    }
	    if ( ifld )
		fld->attach( rightOf, colboxes[ifld-1] );
	    else
	    {
		fld->attach( rightOf, lbl );
		if ( ielem == 0 )
		    setHAlignObj( fld );
	    }
	}
    }
}

bool commit()
{
    //TODO
    return true;
}

    uiComboBox*				elemfld;
    ObjectSet< ObjectSet<uiSpinBox> >	boxes_;
    Table::FormatInfo&			fi_;
    BufferString			errmsg_;

    static int				ideffld_;

};

int uiTableImpDataSelElem::ideffld_ = 0;


uiTableImpDataSel::uiTableImpDataSel( uiParent* p, Table::FormatDesc& fd )
	: uiGroup(p,fd.name())
	, fd_(fd)
	, errmsg_(0)
{
    uiTableImpDataSelElem::ideffld_ = 0;
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

    //TODO support setting tokencol_
    BufferString txt( fd_.token_ );
    if ( txt == "" ) txt = "0";
    if ( fd_.nrhdrlines_ > 0 ) txt = fd_.nrhdrlines_;
    hdrendfld = new uiGenInput( this, "Header stops after (nr lines, or token)",
				txt );
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

    uiTableImpDataSelElem::ideffld_ = 0;
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
    if ( txt == "" )
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
