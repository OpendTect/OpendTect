/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          August 2003
 RCS:           $Id: uisurfaceman.cc,v 1.22 2004-11-09 13:13:58 nanne Exp $
________________________________________________________________________

-*/


#include "uisurfaceman.h"
#include "ioobj.h"
#include "iostrm.h"
#include "strmprov.h"
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
#include "uimsg.h"


uiSurfaceMan::uiSurfaceMan( uiParent* p, bool hor )
    : uiObjFileMan(p,uiDialog::Setup("Surface file management",
                                     hor ? "Manage horizons": "Manage faults",
                                     "104.2.0").nrstatusflds(1),
	    	   hor ? *mMkCtxtIOObj(EMHorizon) : *mMkCtxtIOObj(EMFault) )
{
    createDefaultUI( hor ? "hor" : "flt" );

    attribfld = new uiListBox( topgrp, "Calculated attributes" );
    attribfld->attach( rightTo, manipgrp );
    attribfld->setToolTip( "Calculated attributes" );

    uiManipButGrp* butgrp = new uiManipButGrp( topgrp );
    butgrp->addButton( uiManipButGrp::Remove, mCB(this,uiSurfaceMan,remPush),
		       "Remove this attribute" );
    butgrp->attach( rightTo, attribfld );

    selChg( this ); 
}


uiSurfaceMan::~uiSurfaceMan()
{
}


void uiSurfaceMan::remPush( CallBacker* )
{
    if ( !attribfld->size() || !attribfld->nrSelected() ) return;
   
    mDynamicCastGet(const IOStream*,iostrm,ctio.ioobj)
    if ( !iostrm ) return;
    StreamProvider sp( iostrm->fileName() );
    sp.addPathIfNecessary( iostrm->dirName() );

    const char* attrnm = attribfld->getText();
    int gap = 0;
    for ( int idx=0; ; idx++ )
    {
	if ( gap > 100 ) return;

	BufferString fnm(
		EM::dgbSurfDataWriter::createHovName(sp.fileName(),idx) );
	if ( File_isEmpty(fnm) )
	{ gap++; continue; }

	EM::dgbSurfDataReader rdr( fnm );
	BufferString datanm = rdr.dataName();
	if ( datanm == attrnm )
	{
	    BufferString msg( "Remove attribute: '" );
	    msg += datanm; msg += "'?";
	    if ( uiMSG().askGoOn( msg ) )
		File_remove( (const char*)fnm, NO );

	    selChg(this);
	    return;
	}
    }
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
    EM::EMM().getSurfaceData( ctio.ioobj->key(),sd);
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
