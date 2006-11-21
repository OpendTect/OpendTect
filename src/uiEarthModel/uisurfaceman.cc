/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          August 2003
 RCS:           $Id: uisurfaceman.cc,v 1.34 2006-11-21 14:00:07 cvsbert Exp $
________________________________________________________________________

-*/


#include "uisurfaceman.h"

#include "binidselimpl.h"
#include "ctxtioobj.h"
#include "filegen.h"
#include "ioobj.h"
#include "oddirs.h"
#include "pixmap.h"

#include "emmanager.h"
#include "emsurfaceauxdata.h"
#include "emsurfaceiodata.h"
#include "emsurfacetr.h"
#include "emsurfauxdataio.h"

#include "uibutton.h"
#include "uigeninputdlg.h"
#include "uihorizonrelations.h"
#include "uiioobjmanip.h"
#include "uiioobjsel.h"
#include "uiiosurfacedlg.h"
#include "uilistbox.h"
#include "uimsg.h"
#include "uitextedit.h"


uiSurfaceMan::uiSurfaceMan( uiParent* p, bool hor )
    : uiObjFileMan(p,uiDialog::Setup("Surface file management",
                                     hor ? "Manage horizons": "Manage faults",
                                     "104.2.0").nrstatusflds(1),
	    	   hor ? EMHorizonTranslatorGroup::ioContext()
		       : EMFaultTranslatorGroup::ioContext() )
{
    createDefaultUI();
    uiIOObjManipGroup* manipgrp = selgrp->getManipGroup();

    manipgrp->addButton( ioPixmap("copyobj.png"), mCB(this,uiSurfaceMan,copyCB),
			 hor ? "Copy horizon" : "Copy fault" );

    attribfld = new uiListBox( this, "Calculated attributes", true );
    attribfld->attach( rightOf, selgrp );
    attribfld->setToolTip( "Calculated attributes" );

    uiManipButGrp* butgrp = new uiManipButGrp( this );
    butgrp->addButton( uiManipButGrp::Remove, mCB(this,uiSurfaceMan,removeCB),
		       "Remove selected attribute(s)" );
#ifdef __debug__
    butgrp->addButton( uiManipButGrp::Rename, mCB(this,uiSurfaceMan,renameCB),
	    	       "Rename selected attribute" );
#endif
    butgrp->attach( rightTo, attribfld );

    uiPushButton* relbut = new uiPushButton( this, "&Relations", false );
    relbut->activated.notify( mCB(this,uiSurfaceMan,setRelations) );
    relbut->attach( alignedBelow, selgrp );
    relbut->attach( ensureBelow, attribfld );

    infofld->attach( ensureBelow, relbut );
    selChg( this ); 
}


uiSurfaceMan::~uiSurfaceMan()
{
}


void uiSurfaceMan::removeCB( CallBacker* )
{
    if ( !curioobj_ ) return;

    if ( curioobj_->implReadOnly() )
    {
	uiMSG().error( "Could not remove attributes. Surface is read-only" );
	return;
    }

    BufferStringSet attribnms;
    attribfld->getSelectedItems( attribnms );
    if ( attribnms.isEmpty() || 
	    !uiMSG().askGoOn("All selected attributes will be removed.\n"
			     "Do you want to continue?") )
	return;

    for ( int ida=0; ida<attribnms.size(); ida++ )
    {
	BufferString filenm = 
	    EM::SurfaceAuxData::getAuxDataFileName( *curioobj_, 
		    				    attribnms.get(ida) );
	if ( !*filenm ) continue;

	File_remove( filenm, NO );
    }

    selChg( this );
}


#define mErrRet(msg) { uiMSG().error(msg); return; }

void uiSurfaceMan::renameCB( CallBacker* )
{
    if ( !curioobj_ ) return;
    BufferString attribnm = attribfld->getText();
    BufferString titl( "Rename '" ); titl += attribnm; titl += "'";
    uiGenInputDlg dlg( this, titl, "New name", new StringInpSpec(attribnm) );
    if ( !dlg.go() ) return;

    BufferString newnm = dlg.text();
    if ( attribfld->isPresent(newnm) )
	mErrRet("Name already in use")

}


void uiSurfaceMan::copyCB( CallBacker* )
{
    if ( !curioobj_ ) return;
    PtrMan<IOObj> ioobj = curioobj_->clone();
    uiCopySurface dlg( this, *ioobj );
    if ( dlg.go() )
	selgrp->fullUpdate( ioobj->key() );
}


void uiSurfaceMan::fillAttribList( const BufferStringSet& strs )
{
    attribfld->empty();
    for ( int idx=0; idx<strs.size(); idx++)
	attribfld->addItem( strs[idx]->buf() );
    attribfld->selectAll( false );
}


void uiSurfaceMan::mkFileInfo()
{
#define mRangeTxt(line) \
    txt += sd.rg.start.line; txt += " - "; txt += sd.rg.stop.line; \
    txt += " - "; txt += sd.rg.step.line; txt += "\n" \

    BufferString txt;
    BinIDSampler bs;
    EM::SurfaceIOData sd;
    const char* res = EM::EMM().getSurfaceData( curioobj_->key(), sd );
    if ( !res )
    {
	fillAttribList( sd.valnames );
	txt = "Inline range: "; mRangeTxt(inl);
	txt += "Crossline range: "; mRangeTxt(crl);
    }

    txt += getFileInfo();

    if ( sd.sections.size() > 1 )
    {
	txt += "Nr of sections: "; txt += sd.sections.size(); txt += "\n";
	for ( int idx=0; idx<sd.sections.size(); idx++ )
	{
	    txt += "\tPatch "; txt += idx+1; txt += ": "; 
	    txt += sd.sections[idx]->buf(); txt += "\n";
	}
    }

    infofld->setText( txt );
}


void uiSurfaceMan::setRelations( CallBacker* )
{
    uiHorizonRelationsDlg dlg( this );
    dlg.go();
}
