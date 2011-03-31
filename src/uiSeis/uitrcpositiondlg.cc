/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        H. Huck
 Date:          July 2010
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uitrcpositiondlg.cc,v 1.4 2011-03-31 08:24:49 cvshelene Exp $";

#include "uitrcpositiondlg.h"

#include "bufstringset.h"
#include "seistrctr.h"
#include "uicombobox.h"
#include "uiseisioobjinfo.h"
#include "uispinbox.h"

uiTrcPositionDlg::uiTrcPositionDlg( uiParent* p, const CubeSampling& cs,
				    bool is2d, const MultiID& mid )
    : uiDialog( p, uiDialog::Setup("Attribute trace position",0,"101.1.7") )
    , linesfld_( 0 )
    , trcnrfld_( 0 )
    , inlfld_( 0 )
    , crlfld_( 0 )
    , mid_( mid )
{
    if ( is2d )
    {
	BufferStringSet linenames;
	uiSeisIOObjInfo objinfo( mid );
	objinfo.ioObjInfo().getLineNames( linenames );
	BufferString str = "Compute attribute on line:";
	linesfld_ = new uiLabeledComboBox( this, str );
	for ( int idx=0; idx<linenames.size(); idx++ )
	    linesfld_->box()->addItem( linenames.get(idx) );
	linesfld_->box()->selectionChanged.notify(
					mCB(this,uiTrcPositionDlg,lineSel) );
	trcnrfld_ = new uiLabeledSpinBox( this, "at trace nr:" );
	trcnrfld_->attach( alignedBelow, linesfld_ );
	lineSel(0);
    }
    else
    {
	BufferString str = "Compute attribute at position:";
	inlfld_ = new uiLabeledSpinBox( this, str );
	crlfld_ = new uiSpinBox( this );
	crlfld_->attach( rightTo, inlfld_ );
	inlfld_->box()->setInterval( cs.hrg.inlRange() );
	crlfld_->setInterval( cs.hrg.crlRange() );
    }
}


LineKey uiTrcPositionDlg::getLineKey() const
{
    LineKey lk;
    if ( !linesfld_ ) return lk;

    lk.setLineName( linesfld_->box()->text() );
    return lk;
}


CubeSampling uiTrcPositionDlg::getCubeSampling() const
{
    CubeSampling cs;
    if ( trcnrfld_ )
    {
	int trcnr = trcnrfld_->box()->getValue();
	cs.hrg.set( cs.hrg.inlRange(), StepInterval<int>( trcnr, trcnr, 1 ) );
    }
    else
    {
	int inlnr = inlfld_->box()->getValue();
	int crlnr = crlfld_->getValue();
	cs.hrg.set( StepInterval<int>( inlnr, inlnr, 1 ),
		    StepInterval<int>( crlnr, crlnr, 1 ) );
    }
    return cs;
}


void uiTrcPositionDlg::lineSel( CallBacker* cb )
{
    CubeSampling cs;
    SeisTrcTranslator::getRanges( mid_, cs, getLineKey() );
    trcnrfld_->box()->setInterval( cs.hrg.crlRange() );
}
