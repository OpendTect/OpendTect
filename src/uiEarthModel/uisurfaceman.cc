/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          August 2003
 RCS:           $Id: uisurfaceman.cc,v 1.30 2006-05-05 06:46:07 cvsnanne Exp $
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
#include "uiiosurfacedlg.h"
#include "uilistbox.h"
#include "uimsg.h"
#include "uitextedit.h"


uiSurfaceMan::uiSurfaceMan( uiParent* p, bool hor )
    : uiObjFileMan(p,uiDialog::Setup("Surface file management",
                                     hor ? "Manage horizons": "Manage faults",
                                     "104.2.0").nrstatusflds(1),
	    	   hor ? *mMkCtxtIOObj(EMHorizon) : *mMkCtxtIOObj(EMFault) )
{
    createDefaultUI( hor ? "hor" : "flt" );

    manipgrp->addButton( ioPixmap(GetIconFileName("copyobj.png")),
	    		 mCB(this,uiSurfaceMan,copyCB), 
			 hor ? "Copy horizon" : "Copy fault" );

    attribfld = new uiListBox( topgrp, "Calculated attributes", true );
    attribfld->attach( rightTo, manipgrp );
    attribfld->setToolTip( "Calculated attributes" );

    uiManipButGrp* butgrp = new uiManipButGrp( topgrp );
    butgrp->addButton( uiManipButGrp::Remove, mCB(this,uiSurfaceMan,removeCB),
		       "Remove selected attribute(s)" );
#ifdef __debug__
    butgrp->addButton( uiManipButGrp::Rename, mCB(this,uiSurfaceMan,renameCB),
	    	       "Rename selected attribute" );
#endif
    butgrp->attach( rightTo, attribfld );

    uiPushButton* relbut = new uiPushButton( topgrp, "&Relations", false );
    relbut->activated.notify( mCB(this,uiSurfaceMan,setRelations) );
    relbut->attach( alignedBelow, listfld );
    relbut->attach( ensureBelow, manipgrp );
    relbut->attach( ensureBelow, attribfld );

    selChg( this ); 
}


uiSurfaceMan::~uiSurfaceMan()
{
}


void uiSurfaceMan::removeCB( CallBacker* )
{
    if ( !ctio.ioobj ) return;

    if ( ctio.ioobj->implReadOnly() )
    {
	uiMSG().error( "Could not remove attributes. Surface is read-only" );
	return;
    }

    BufferStringSet attribnms;
    attribfld->getSelectedItems( attribnms );
    if ( !attribnms.size() || 
	    !uiMSG().askGoOn("All selected attributes will be removed.\n"
			     "Do you want to continue?") )
	return;

    for ( int ida=0; ida<attribnms.size(); ida++ )
    {
	BufferString filenm = 
	    EM::SurfaceAuxData::getAuxDataFileName( *ctio.ioobj, 
		    				    attribnms.get(ida) );
	if ( !*filenm ) continue;

	File_remove( filenm, NO );
    }

    selChg( this );
}


#define mErrRet(msg) { uiMSG().error(msg); return; }

void uiSurfaceMan::renameCB( CallBacker* )
{
    if ( !ctio.ioobj ) return;
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
    if ( !ctio.ioobj ) return;
    PtrMan<IOObj> ioobj = ctio.ioobj->clone();
    uiCopySurface dlg( this, *ioobj );
    if ( dlg.go() )
	postIomChg(0);
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
    if ( !ctio.ioobj )
    {
	infofld->setText( "" );
	return;
    }

#define mRangeTxt(line) \
    txt += sd.rg.start.line; txt += " - "; txt += sd.rg.stop.line; \
    txt += " - "; txt += sd.rg.step.line; txt += "\n" \

    BufferString txt;
    BinIDSampler bs;
    EM::SurfaceIOData sd;
    EM::EMM().getSurfaceData( ctio.ioobj->key(), sd );
    fillAttribList( sd.valnames );
    txt = "Inline range: "; mRangeTxt(inl);
    txt += "Crossline range: "; mRangeTxt(crl);

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
