/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          August 2003
 RCS:           $Id: uisurfaceman.cc,v 1.37 2007-05-22 03:23:23 cvsnanne Exp $
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


#define mGet( typ, hor2d, hor3d, flt ) \
    !strcmp(typ,EMHorizon2DTranslatorGroup::keyword) ? hor2d : \
    (!strcmp(typ,EMHorizon3DTranslatorGroup::keyword) ? hor3d : flt)

#define mGetIoContext(typ) \
    mGet( typ, EMHorizon2DTranslatorGroup::ioContext(), \
	       EMHorizon3DTranslatorGroup::ioContext(), \
	       EMFaultTranslatorGroup::ioContext() )

#define mGetManageStr(typ) \
    mGet( typ, "Manage 2D horizons", "Manage horizons", "Manage faults" )

#define mGetCopyStr(typ) \
    mGet( typ, "Copy 2D horizon", "Copy horizon", "Copy fault" )

using namespace EM;

uiSurfaceMan::uiSurfaceMan( uiParent* p, const char* typ )
    : uiObjFileMan(p,uiDialog::Setup("Surface file management",
                                     mGetManageStr(typ),
                                     "104.2.0").nrstatusflds(1),
		   mGetIoContext(typ) )
    , is2d_(mGet(typ,true,false,false))
{
    createDefaultUI();
    uiIOObjManipGroup* manipgrp = selgrp->getManipGroup();

    manipgrp->addButton( ioPixmap("copyobj.png"), mCB(this,uiSurfaceMan,copyCB),
			 mGetCopyStr(typ) );

    attribfld = new uiListBox( this, "Calculated attributes", true );
    attribfld->attach( rightOf, selgrp );
    attribfld->setToolTip( "Calculated attributes" );

    uiManipButGrp* butgrp = new uiManipButGrp( this );
    butgrp->addButton( uiManipButGrp::Remove,
	    	       mCB(this,uiSurfaceMan,removeAttribCB),
		       "Remove selected attribute(s)" );
#ifdef __debug__
    butgrp->addButton( uiManipButGrp::Rename,
	    	       mCB(this,uiSurfaceMan,renameAttribCB),
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
{}


void uiSurfaceMan::copyCB( CallBacker* )
{
    if ( !curioobj_ ) return;
    PtrMan<IOObj> ioobj = curioobj_->clone();
    uiCopySurface dlg( this, *ioobj );
    if ( dlg.go() )
	selgrp->fullUpdate( ioobj->key() );
}


void uiSurfaceMan::setRelations( CallBacker* )
{
    uiHorizonRelationsDlg dlg( this, is2d_ );
    dlg.go();
}


void uiSurfaceMan::removeAttribCB( CallBacker* )
{
    if ( !curioobj_ ) return;

    if ( curioobj_->implReadOnly() )
    {
	uiMSG().error( "Could not remove attributes. Surface is read-only" );
	return;
    }

    BufferStringSet attrnms;
    attribfld->getSelectedItems( attrnms );
    if ( attrnms.isEmpty() || 
	    !uiMSG().askGoOn("All selected attributes will be removed.\n"
			     "Do you want to continue?") )
	return;

    for ( int ida=0; ida<attrnms.size(); ida++ )
    {
	const BufferString filenm = 
	    SurfaceAuxData::getAuxDataFileName( *curioobj_, attrnms.get(ida) );
	if ( filenm.isEmpty() ) continue;

	File_remove( filenm, NO );
    }

    selChg( this );
}


#define mErrRet(msg) { uiMSG().error(msg); return; }

void uiSurfaceMan::renameAttribCB( CallBacker* )
{
    if ( !curioobj_ ) return;

    const char* attribnm = attribfld->getText();
    BufferString titl( "Rename '" ); titl += attribnm; titl += "'";
    uiGenInputDlg dlg( this, titl, "New name", new StringInpSpec(attribnm) );
    if ( !dlg.go() ) return;

    const char* newnm = dlg.text();
    if ( attribfld->isPresent(newnm) )
	mErrRet("Name is already in use")

    const BufferString filename =
		SurfaceAuxData::getAuxDataFileName( *curioobj_, attribnm );
    if ( !File_exists(filename) )
	mErrRet( "Cannot find file to change attribute name" )

// TODO: Read file, replace attribute name and write file again.
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
    SurfaceIOData sd;
    const char* res = EMM().getSurfaceData( curioobj_->key(), sd );
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
