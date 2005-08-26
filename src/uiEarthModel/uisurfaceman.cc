/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          August 2003
 RCS:           $Id: uisurfaceman.cc,v 1.26 2005-08-26 18:19:28 cvsbert Exp $
________________________________________________________________________

-*/


#include "uisurfaceman.h"
#include "ioobj.h"
#include "ctxtioobj.h"
#include "uilistbox.h"
#include "uiioobjmanip.h"
#include "uitextedit.h"
#include "filegen.h"
#include "binidselimpl.h"
#include "emmanager.h"
#include "emsurfacetr.h"
#include "emsurfaceiodata.h"
#include "emsurfauxdataio.h"
#include "emsurfaceauxdata.h"
#include "uimsg.h"
#include "uigeninputdlg.h"
#include "uiiosurfacedlg.h"
#include "pixmap.h"
#include "oddirs.h"


uiSurfaceMan::uiSurfaceMan( uiParent* p, bool hor )
    : uiObjFileMan(p,uiDialog::Setup("Surface file management",
                                     hor ? "Manage horizons": "Manage faults",
                                     "104.2.0").nrstatusflds(1),
	    	   hor ? *mMkCtxtIOObj(EMHorizon) : *mMkCtxtIOObj(EMFault) )
{
    createDefaultUI( hor ? "hor" : "flt" );

    manipgrp->addButton( ioPixmap(GetDataFileName("copyobj.png")),
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

    selChg( this ); 
}


uiSurfaceMan::~uiSurfaceMan()
{
}


void uiSurfaceMan::removeCB( CallBacker* )
{
    if ( !ctio.ioobj ) return;
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
    txt += " - "; txt += sd.rg.step.line; \

    BufferString txt;
    BinIDSampler bs;
    EM::SurfaceIOData sd;
    EM::EMM().getSurfaceData( ctio.ioobj->key(), sd );
    fillAttribList( sd.valnames );
    txt = "Inline range: "; mRangeTxt(inl);
    txt += "\nCrossline range: "; mRangeTxt(crl);

    txt += getFileInfo();

    if ( sd.sections.size() > 1 )
    {
	txt += "\nNr of sections: "; txt += sd.sections.size();
	for ( int idx=0; idx<sd.sections.size(); idx++ )
	{
	    txt += "\n\tPatch "; txt += idx+1; txt += ": "; 
	    txt += sd.sections[idx]->buf();
	}
    }

    infofld->setText( txt );
}
