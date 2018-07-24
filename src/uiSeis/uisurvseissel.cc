/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          July 2018
________________________________________________________________________

-*/

#include "uisurvseissel.h"

#include "uicombobox.h"
#include "uibutton.h"
#include "ioobjctxt.h"
#include "seisioobjinfo.h"


static IOObjContext& getCtxt( Seis::GeomType gt, int steerflag )
{
    static IOObjContext* ret = 0;
    delete ret;
    ret = Seis::getIOObjContext( gt, true );
    if ( steerflag != 0 )
    {
	IOPar& iop = steerflag < 0 ? ret->toselect_.dontallow_
				   : ret->toselect_.require_;
	iop.set( sKey::Type(), sKey::Steering() );
    }
    return *ret;
}


static uiString getLbl( const uiString& inp, Seis::GeomType gt )
{
    if ( inp == toUiString("-") )
	return uiString::empty();
    else if ( inp.isEmpty() )
	return uiStrings::sSeisGeomTypeName( Seis::is2D(gt), Seis::isPS(gt) );
    return inp;
}


uiSurvSeisSel::uiSurvSeisSel( uiParent* p, const Setup& su )
    : uiSurvIOObjSel(p,getCtxt(su.geomtype_,su.steerflag_),
			getLbl(su.lbltxt_,su.geomtype_),su.fixedsurvey_)
    , setup_(su)
    , compSel(this)
{
    compfld_ = new uiComboBox( this, "Components" );
    if ( survselbut_ )
	compfld_->attach( rightOf, survselbut_ );
    else
	compfld_->attach( rightOf, objfld_ );

    mAttachCB( postFinalise(), uiSurvSeisSel::initSeisGrp );
}


uiSurvSeisSel::~uiSurvSeisSel()
{
}


void uiSurvSeisSel::initSeisGrp( CallBacker* )
{
    updateComps();
    mAttachCB( selChange, uiSurvSeisSel::inpSelChgCB );
    mAttachCB( compfld_->selectionChanged, uiSurvSeisSel::compSelCB );
}


void uiSurvSeisSel::inpSelChgCB( CallBacker* )
{
    updateComps();
}


void uiSurvSeisSel::compSelCB( CallBacker* )
{
    compSel.trigger();
}


void uiSurvSeisSel::setCompNr( int compnr )
{
    compfld_->setCurrentItem( compnr );
}


int uiSurvSeisSel::compNr() const
{
    const int selidx = compfld_->currentItem();
    return selidx < 0 ? 0 : selidx;
}


int uiSurvSeisSel::nrComps() const
{
    return compfld_->size();
}


const char* uiSurvSeisSel::compName( int idx ) const
{
    return compfld_->itemText( idx );
}


void uiSurvSeisSel::updateComps()
{
    compfld_->setEmpty();
    const IOObj* ioobj = ioObj();
    if ( !ioobj )
	return;

    SeisIOObjInfo ioobjinfo( *ioobj );
    BufferStringSet compnms;
    ioobjinfo.getComponentNames( compnms );
    compfld_->addItems( compnms );
    if ( !compnms.isEmpty() )
	compfld_->setCurrentItem( 0 );
}
