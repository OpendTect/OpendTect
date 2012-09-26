/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nageswara
 Date:          Feb 2009
________________________________________________________________________
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "uiimpfaultstickset2d.h"

#include "bufstringset.h"
#include "emfault3d.h"

#include "uicombobox.h"
#include "uifileinput.h"
#include "uiioobjsel.h"
#include "uimsg.h"
#include "uiseispartserv.h"
#include "uitblimpexpdatasel.h"

uiImportFaultStickSet2D::uiImportFaultStickSet2D( uiParent* p,
						  const char* type )
    : uiImportFault(p,type,true)
    , linesetnms_(*new BufferStringSet)
{
    fd_ = EM::FaultAscIO::getDesc(true);
    createUI();

    TypeSet<BufferStringSet> linenames;
    uiSeisPartServer::get2DLineInfo( linesetnms_, setids_, linenames );
    uiLabeledComboBox* linesetbox = new uiLabeledComboBox(
	    				this, "Select Line Set", "2D Lineset" );
    linesetbox->attach( alignedBelow, infld_ );
    linesetfld_ = linesetbox->box();
    linesetfld_->addItems( linesetnms_ );

    dataselfld_->attach( alignedBelow, linesetbox );
    outfld_->attach( alignedBelow, dataselfld_ );
}


bool uiImportFaultStickSet2D::getFromAscIO( std::istream& strm, EM::Fault& flt )
{
    const int setidx = linesetfld_->currentItem();
    const MultiID setid = setids_[setidx];

    EM::FaultAscIO ascio( *fd_ );
    return ascio.get( strm, flt, false, &setid, true );
}


bool uiImportFaultStickSet2D::acceptOK( CallBacker* )
{
    if ( !checkInpFlds() ) return false;
    return handleAscii();
}
