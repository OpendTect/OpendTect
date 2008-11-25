/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Helene Huck
 Date:          August 2008
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uimultcomputils.cc,v 1.2 2008-11-25 15:35:26 cvsbert Exp $";

#include "uimultcompdlg.h"
#include "ioman.h"
#include "ioobj.h"
#include "linekey.h"
#include "ptrman.h"
#include "seisread.h"
#include "seistrctr.h"
#include "uispinbox.h"


uiMultCompDlg::uiMultCompDlg( uiParent* p, LineKey lkey )
    	: uiDialog(p,uiDialog::Setup("Component dialog",
		    		     "Choose the component to display","")
		     		.modal(false) )
	, needdisplay_(true)
	, compfld_(0)
{
    int nrcomp = getNrCompAvail( lkey );

    if ( nrcomp>1 )
    {
	compfld_ = new uiLabeledSpinBox( this, "Component nr", 0, "Compfld" );
	compfld_->box()->setInterval( StepInterval<int>(1,nrcomp,1) );
    }
    else
	needdisplay_ = false;
}


int uiMultCompDlg::getNrCompAvail( LineKey lkey )
{
    const MultiID key( lkey );
    PtrMan<IOObj> ioobj = IOM().get( key );
    SeisTrcReader rdr( ioobj );
    if ( !rdr.prepareWork(Seis::PreScan) ) return 0;

    SeisTrcTranslator* transl = rdr.seisTranslator();
    return transl ? transl->componentInfo().size() : 0; 
}


int uiMultCompDlg::getCompNr() const
{
    return compfld_->box()->getValue();
}
