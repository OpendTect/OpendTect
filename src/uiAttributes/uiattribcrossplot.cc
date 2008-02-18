/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          March 2003
 RCS:           $Id: uiattribcrossplot.cc,v 1.8 2008-02-18 16:32:17 cvsbert Exp $
________________________________________________________________________

-*/

#include "uiattribcrossplot.h"
#include "seisioobjinfo.h"
#include "attribsel.h"
#include "posvecdataset.h"
#include "attribdescset.h"
#include "attribposvecoutput.h"
#include "posvecdatasettr.h"
#include "keystrs.h"
#include "ioobj.h"
#include "ioman.h"
#include "iopar.h"

#include "uimsg.h"
#include "uilistbox.h"
#include "uiexecutor.h"
#include "uiioobjsel.h"
#include "uiposdataedit.h"
#include "uiposprovider.h"

using namespace Attrib;

uiAttribCrossPlot::uiAttribCrossPlot( uiParent* p, const DescSet& d )
	: uiDialog(p,uiDialog::Setup("Attribute cross-plotting",
		     "Select attributes and locations for cross-plot"
		     ,"101.3.0"))
	, ads_(d)
{
    uiLabeledListBox* llb = new uiLabeledListBox( this,
	    					  "Attributes to calculate" );
    attrsfld_ = llb->box();
    SelInfo attrinf( &ads_, 0, ads_.is2D() );
    for ( int idx=0; idx<attrinf.attrnms.size(); idx++ )
    {
	attrsfld_->addItem( attrinf.attrnms.get(idx), false );
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
	    attrsfld_->addItem(
		    SeisIOObjInfo::defKey2DispName(defkey,ioobjnm) );
	    attrdefs_.add( defkey );
	}
    }
    if ( !attrsfld_->isEmpty() )
	attrsfld_->setCurrentItem( int(0) );
    attrsfld_->setMultiSelect( true );

    uiPosProvider::Setup su( "Select locations by", true );
    su.choicetype( uiPosProvider::Setup::All ).withz(true).is2d( ads_.is2D() );
    posprovfld_ = new uiPosProvider( this, su );
    posprovfld_->attach( alignedBelow, llb );
}


uiAttribCrossPlot::~uiAttribCrossPlot()
{
}


#define mErrRet(s) { uiMSG().error(s); return false; }

bool uiAttribCrossPlot::acceptOK( CallBacker* )
{

    /*
    ObjectSet<PosVecDataSet> outvdss;
    PosVecOutputGen pvog( ads_, attrssel, bivsets, outvdss );
    uiExecutor uiex( this, pvog );
    if ( !uiex.go() )
	return false;

    for ( int idx=0; idx<outvdss.size(); idx++ )
	outvdss[idx]->setName( psusrnms.get(idx) );

    uiPosDataEdit dlg( this, outvdss, "Attribute data", uiPosDataEdit::Both,
	    		ads_.is2D() );
    dlg.saveData.notify( mCB(this, uiAttribCrossPlot,saveData) );
    return dlg.go() ? true : false;
    */
    return true;
}

/*

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
*/
