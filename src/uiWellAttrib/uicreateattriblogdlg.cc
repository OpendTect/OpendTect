/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Satyaki
 Date:          March 2008
 RCS:           $Id: uicreateattriblogdlg.cc,v 1.1 2008-03-13 11:17:05 cvssatyaki Exp $
_______________________________________________________________________

-*/

#include "uicreateattriblogdlg.h"
#include "attribdescset.h"
#include "bufstringset.h"
#include "datainpspec.h"
#include "wellman.h"
#include "welldata.h"
#include "wellmarker.h"
#include "uiattrsel.h"
#include "uilistbox.h"
#include "uigeninput.h"

uiCreateAttribLogDlg::uiCreateAttribLogDlg( uiParent* p, BufferStringSet list,
    const Attrib::DescSet* attrib )
    :uiDialog(p,uiDialog::Setup("Create Attribute Log",
				"Select Wells from the List", 0) )
    , childlist_(0)
    , attrib_(attrib)
    , parent_(p)
    , wellist_(0)
    , wellselected(this)
{
    attribfld_ = new uiAttrSel( this, attrib_, attrib_->is2D() );
    childlist_ = new uiListBox( this );
    childlist_->attach( alignedBelow, attribfld_ );
    childlist_->setMultiSelect();
    childlist_->addItems( list );
    BufferStringSet markernames;
    for ( int idx=0; idx<Well::MGR().wells()[1]->markers().size(); idx++ )
    {
	markernames.add( Well::MGR().wells()[1]->markers()[idx]->name() );
    }
    StringListInpSpec slis( markernames );
    topmrkfld_ = new uiGenInput( this, "Extract between", slis );
    topmrkfld_->attach( alignedBelow, childlist_ );
    topmrkfld_->setValue( (int)0 );
    topmrkfld_->setElemSzPol( uiObject::Medium );
    botmrkfld_ = new uiGenInput( this, "", slis );
    botmrkfld_->attach( rightOf, topmrkfld_ );
    botmrkfld_->setValue( markernames.size()-1 );
    botmrkfld_->setElemSzPol( uiObject::Medium );

    //Well::MGR().wells()
}


uiCreateAttribLogDlg::~uiCreateAttribLogDlg()
{}


const TypeSet<int>& uiCreateAttribLogDlg::getSelectedItems() const
{
    childlist_->getSelectedItems( *wellist_ );
    return *wellist_;
}


bool uiCreateAttribLogDlg::acceptOK( CallBacker* )
{
    wellselected.trigger();
    return true;
}
