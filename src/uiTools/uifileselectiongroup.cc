/*+
 ________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          March 2010
 ________________________________________________________________________

-*/

#include "uipositiontable.h"

#include "uilistbox.h"
#include "uicombobox.h"
#include "uilabel.h"
#include "uilineedit.h"

#include "ranges.h"
#include "survinfo.h"


uiFileSelectionGroup::uiFileSelectionGroup( uiParent* p, const Setup& su )
    : uiGroup(p,"File selection group")
    , setup_(su)
{
    uiListBox::Setup lbsu;
    lbsu.prefwidth( 30 );
    uiGroup* leftgrp = 0;
    if ( setup_.isForWrite() || setup_.isForDirectory() )
	leftgrp = new uiGroup( this, "Left group" );
    dirselfld_ = new uiListBox( leftgrp ? leftgrp : this, lbsu, "Directories" );
    if ( !leftgrp )
	leftgrp = dirselfld_;
    else
    {
	//TODO button + dirname field for new directory
    }

    lbsu.prefwidth( 50 ).cm( sSingle(setup_.selmode_) ? OD::ChooseOnlyOne
	    					      : OD::ChooseZeroOrMore );
    leafselfld_ = new uiListBox( this, lbsu, "Selectables" );
    leafselfld_->attach( rightOf, leftgrp );
}
