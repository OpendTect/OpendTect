/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          March 2003
 RCS:           $Id: uiattribcrossplot.cc,v 1.7 2008-02-06 04:22:04 cvsraman Exp $
________________________________________________________________________

-*/

#include "uiattribcrossplot.h"
#include "seisioobjinfo.h"
#include "iodirentry.h"
#include "attribsel.h"
#include "posvecdataset.h"
#include "attribdescset.h"
#include "attribposvecoutput.h"
#include "posvecdatasettr.h"
#include "picksettr.h"
#include "keystrs.h"
#include "ioobj.h"
#include "ioman.h"
#include "iopar.h"

#include "uimsg.h"
#include "uilistbox.h"
#include "uitaskrunner.h"
#include "uiioobjsel.h"
#include "uiposdataedit.h"

using namespace Attrib;

uiAttribCrossPlot::uiAttribCrossPlot( uiParent* p, const DescSet& d )
	: uiDialog(p,uiDialog::Setup("Attribute cross-plotting",
		     "Select attributes and locations for cross-plot"
		     ,"101.3.0"))
	, ads_(d)
{
    attrsfld = new uiLabeledListBox( this, "Attributes to calculate" );
    SelInfo attrinf( &ads_, 0, ads_.is2D() );
    for ( int idx=0; idx<attrinf.attrnms.size(); idx++ )
    {
	attrsfld->box()->addItem( attrinf.attrnms.get(idx), false );
	attrdefs_.add( attrinf.attrnms.get(idx) );
    }
    for ( int idx=0; idx<attrinf.ioobjids.size(); idx++ )
    {
	BufferStringSet bss;
	SeisIOObjInfo sii( MultiID( attrinf.ioobjids.get(idx) ) );
	sii.getDefKeys( bss, true );
	for ( int inm=0; inm<bss.size(); inm++ )
	{
	    const char* defkey = bss.get(inm).buf();
	    const char* ioobjnm = attrinf.ioobjnms.get(idx).buf();
	    attrsfld->box()->addItem(
		    SeisIOObjInfo::defKey2DispName(defkey,ioobjnm) );
	    attrdefs_.add( defkey );
	}
    }
    if ( !attrsfld->box()->isEmpty() )
	attrsfld->box()->setCurrentItem( int(0) );
    attrsfld->box()->setMultiSelect( true );

    pssfld = new uiLabeledListBox( this, "Evaluate at locations from" );
    IOM().to( PickSetTranslatorGroup::ioContext().getSelKey() );
    IODirEntryList del( IOM().dirPtr(), &PickSetTranslatorGroup::theInst(),
	    		false, 0 );
    del.sort();
    for ( int idx=0; idx<del.size(); idx++ )
    {
	const IODirEntry& de = *del[idx];
	if ( de.ioobj )
	{
	    psdefs_.add( de.ioobj->key().buf() );
	    pssfld->box()->addItem( de.ioobj->name() );
	}
    }
    pssfld->box()->setMultiSelect( true );
    pssfld->attach( alignedBelow, attrsfld );
}


uiAttribCrossPlot::~uiAttribCrossPlot()
{
}


#define mErrRet(s) { uiMSG().error(s); return false; }

bool uiAttribCrossPlot::acceptOK( CallBacker* )
{
    BufferStringSet attrssel, psssel;
    for ( int idx=0; idx<attrdefs_.size(); idx++ )
	if ( attrsfld->box()->isSelected(idx) )
	    attrssel.add( attrdefs_.get(idx) );
    BufferStringSet psusrnms;
    for ( int idx=0; idx<psdefs_.size(); idx++ )
    {
	if ( pssfld->box()->isSelected(idx) )
	{
	    psssel.add( psdefs_.get(idx) );
	    psusrnms.add( pssfld->box()->textOfItem(idx) );
	}
    }
    if ( attrssel.size() < 1 || psssel.size() < 1 )
    {
	BufferString msg( "Please select at least one " );
	msg += attrssel.size() < 1 ? "Attribute" : "Pick Set";
	mErrRet( msg );
    }

    ObjectSet<BinIDValueSet> bivsets;
    PickSetTranslator::createBinIDValueSets( psssel, bivsets );
    bool havedata = !bivsets.isEmpty();
    if ( havedata )
    {
	havedata = false;
	for ( int idx=0; idx<bivsets.size(); idx++ )
	    if ( !bivsets[idx]->isEmpty() )
		{ havedata = true; break; }
    }
    if ( !havedata )
	mErrRet( "No valid positions in Pick Sets" );

    ObjectSet<PosVecDataSet> outvdss;
    PosVecOutputGen pvog( ads_, attrssel, bivsets, outvdss );
    uiTaskRunner taskrunner( this );
    if ( !taskrunner.execute(pvog) )
	return false;

    for ( int idx=0; idx<outvdss.size(); idx++ )
	outvdss[idx]->setName( psusrnms.get(idx) );

    uiPosDataEdit dlg( this, outvdss, "Attribute data", uiPosDataEdit::Both,
	    		ads_.is2D() );
    dlg.saveData.notify( mCB(this, uiAttribCrossPlot,saveData) );
    return dlg.go() ? true : false;
}


void uiAttribCrossPlot::saveData( CallBacker* cb )
{
    mDynamicCastGet(uiPosDataEdit*,dlg,cb)
    if ( !dlg ) { pErrMsg("Huh"); return; }

    CtxtIOObj ctio( PosVecDataSetTranslatorGroup::ioContext() );
    ctio.ctxt.forread = false;
    uiIOObjSelDlg seldlg( this, ctio );
    if ( !seldlg.go() )
	return;
    ctio.setObj( seldlg.ioObj()->clone() );

    dlg->stdSave( *ctio.ioobj, true );
    delete ctio.ioobj;
}
