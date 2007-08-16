/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          June 2002
 RCS:           $Id: uicolortable.cc,v 1.1 2007-08-16 15:55:46 cvsbert Exp $
________________________________________________________________________

-*/

#include "uicolortable.h"
#include "colortab.h"

#include "uilineedit.h"
#include "uicombobox.h"
#include "uirgbarray.h"
#include "uirgbarraycanvas.h"

#define mStdInitList \
	  vertical_(vert) \
	, rgbarr_(*new uiRGBArray) \
	, coltab_(*new ColorTable) \
	, scale_(*new ColTabScaling) \
	, tableSelected(this) \
	, scaleChanged(this)


uiColorTable::uiColorTable( uiParent* p, bool vert )
	: uiGroup(p,"Color table display/edit")
	, mStdInitList
{
    maxfld_ = new uiLineEdit( this, "", "Max" );
    maxfld_->returnPressed.notify( mCB(this,uiColorTable,rangeEntered) );

    mkDispFld( Interval<float>(0,1) );

    minfld_ = new uiLineEdit( this, "", "Min" );
    minfld_->attach( vertical_ ? rightAlignedBelow : rightOf, dispfld_ );
    minfld_->returnPressed.notify( mCB(this,uiColorTable,rangeEntered) );

    selfld_ = new uiComboBox( this, "Table selection" );
    selfld_->attach( vertical_ ? alignedBelow : rightOf, minfld_ );
    selfld_->selectionChanged.notify( mCB(this,uiColorTable,tabSel) );

    setHAlignObj(selfld_); setHCentreObj(selfld_);
}


uiColorTable::uiColorTable( uiParent* p, const ColorTable& c,
			    Interval<float> rg, bool vert )
	: uiGroup(p,"Color table display")
	, mStdInitList
	, maxfld_(0)
	, selfld_(0)
{
    mkDispFld( rg );
    setHAlignObj(dispfld_); setHCentreObj(dispfld_);
}


uiColorTable::~uiColorTable()
{
    delete &rgbarr_;
    delete &coltab_;
}


void uiColorTable::mkDispFld( const Interval<float>& rg )
{
}


void uiColorTable::tabSel( CallBacker* )
{
}


void uiColorTable::rangeEntered( CallBacker* )
{
}
