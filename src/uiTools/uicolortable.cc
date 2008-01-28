/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          June 2002
 RCS:           $Id: uicolortable.cc,v 1.4 2008-01-28 10:35:55 cvsbert Exp $
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
	, coltab_(*new ColorTable) \
	, scale_(*new ColTabScaling) \
	, tableSelected(this) \
	, scaleChanged(this)


uiColorTable::uiColorTable( uiParent* p, bool vert )
	: uiGroup(p,"Color table display/edit")
	, mStdInitList
{
    maxfld_ = new uiLineEdit( this, "", "ColTab Max" );
    maxfld_->returnPressed.notify( mCB(this,uiColorTable,rangeEntered) );
    maxfld_->setHSzPol( uiObject::Small );

    canvas_ = new uiColorTableCanvas( this, coltab_, vertical_ );
    canvas_->getMouseEventHandler().buttonPressed.notify(
			mCB(this,uiColorTable,canvasClick) );

    minfld_ = new uiLineEdit( this, "", "ColTab Min" );
    minfld_->setHSzPol( uiObject::Small );
    minfld_->attach( vertical_ ? rightAlignedBelow : rightOf, canvas_ );
    minfld_->returnPressed.notify( mCB(this,uiColorTable,rangeEntered) );

    selfld_ = new uiComboBox( this, "Table selection" );
    selfld_->attach( vertical_ ? alignedBelow : rightOf, minfld_ );
    selfld_->selectionChanged.notify( mCB(this,uiColorTable,tabSel) );
    selfld_->setStretch( 0, vertical_ ? 1 : 0 );

    setHAlignObj(selfld_); setHCentreObj(selfld_);
}


uiColorTable::uiColorTable( uiParent* p, const ColorTable& c,
			    Interval<float> rg, bool vert )
	: uiGroup(p,"Color table display")
	, mStdInitList
	, maxfld_(0)
	, selfld_(0)
{
    canvas_ = new uiColorTableCanvas( this, coltab_, vertical_ );
    canvas_->setRange( rg );
    setHAlignObj(canvas_); setHCentreObj(canvas_);
}


uiColorTable::~uiColorTable()
{
    delete &coltab_;
}


void uiColorTable::fillTabList()
{
    selfld_->empty();
    NamedBufferStringSet ctabs( "Color table" );
    ColorTable::getNames( ctabs ); ctabs.sort();
    coltabsel_->addItems( ctabs );
    coltabsel_->setCurrentItem( coltab_.name() );
    for ( int idx=0; idx<ctabs.size(); idx++ )
	coltabsel_->setPixmap( ioPixmap(ctabs.get(idx),16,10) );
}


void uiColorTable::doApply4Man
{
    mDynamicCastGet(uiColorTableMan*,dlg,cb) if ( !dlg ) return;
    IOPar par; dlg->currentColTab().fillPar( par );
    ColorTable::get( ct.name(), coltab_ );
    coltab_.usePar( par );
    applyScaling();
    selfld_->setCurrentItem( coltab_.name() );
    canvas_->forceNewFill();
    setRangeFields();
}


void uiColorTable::applyScaling()
{
    if ( scale_.autoscale_ )
    coltab_.scaleTo( scale_.range_ );
    if ( !isEditable() ) return;
    minfld_->setValue( coltabrg_.start );
    maxfld_->setValue( coltabrg_.stop );
}


void uiColorTable::tabSel( CallBacker* )
{
}


void uiColorTable::rangeEntered( CallBacker* )
{
}
