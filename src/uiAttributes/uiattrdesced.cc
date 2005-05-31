/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          May 2005
 RCS:		$Id: uiattrdesced.cc,v 1.1 2005-05-31 12:54:14 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uiattrdesced.h"
#include "uiattrsel.h"
#include "uisteersel.h"
#include "uilabel.h"
#include "attribdesc.h"
#include "attribdescset.h"
#include "attribdescsetman.h"
#include "attribparam.h"
#include "iopar.h"
#include "survinfo.h"
#include "uiattrfact.h"

const char* uiAttrDescEd::timegatestr = "Time gate";
const char* uiAttrDescEd::stepoutstr = "Stepout";
const char* uiAttrDescEd::frequencystr = "Frequency";
const char* uiAttrDescEd::filterszstr = "Filter size";


uiAttrDescEd::uiAttrDescEd( uiParent* p )
    : uiGroup(p,"")
    , attrdesc(0)
{
}


uiAttrDescEd::~uiAttrDescEd()
{
}


void uiAttrDescEd::setDesc( Attrib::Desc* desc, Attrib::DescSetMan* adsm )
{
    attrdesc = desc;
    adsman = adsm;
    if ( attrdesc )
    {
	chtr.setVar( adsman->unSaved() );
	setParameters( *desc );
	setInput( *desc );
	setOutput( *desc );
    }
}


void uiAttrDescEd::fillInp( const uiAttrSel* fld, Attrib::Desc& desc, int inp )
{
    const Attrib::Desc* inpdesc = desc.getInput( inp );
    if ( !inpdesc ) return;

    const int attribid = fld->attribID();
    chtr.set( inpdesc->id(), attribid );
    desc.setInput( inp, desc.descSet()->getDesc(attribid) );
    mDynamicCastGet(const uiImagAttrSel*,imagfld,fld)
    if ( imagfld )
	desc.setInput( inp+1, desc.descSet()->getDesc(imagfld->imagID()) );
}


void uiAttrDescEd::fillInp( const uiSteeringSel* fld, Attrib::Desc& desc, 
			    int inp )
{
    fld->fillDesc( desc, chtr );
}


void uiAttrDescEd::fillOutput( Attrib::Desc& desc, int selout )
{
    if ( chtr.set(desc.selectedOutput(),selout) )
	desc.selectOutput( selout );
}


uiAttrSel* uiAttrDescEd::getInpFld( const char* txt, const uiAttrSelData* asd )
{
    uiAttrSel* fld = asd ? new uiAttrSel( this, txt, *asd )
			 : new uiAttrSel( this, 0, txt );
    fld->selectiondone.notify( mCB(this,uiAttrDescEd,attrInpSel) );
    return fld;
}


uiImagAttrSel* uiAttrDescEd::getImagInpFld()
{
    uiImagAttrSel* fld = new uiImagAttrSel( this, 0, 0 );
    fld->selectiondone.notify( mCB(this,uiAttrDescEd,attrInpSel) );
    return fld;
}


void uiAttrDescEd::attrInpSel( CallBacker* cb )
{
    if ( !adsman || !adsman->descSet() ) return;
    mDynamicCastGet(uiAttrSel*,as,cb)
    if ( as )
	set2D( adsman->descSet()->is2D() );
}


void uiAttrDescEd::putInp( uiAttrSel* inpfld, const Attrib::Desc& ad, 
			   int inpnr )
{
    const Attrib::Desc* inpdesc = ad.getInput( inpnr );
    if ( !inpdesc )
	inpfld->setDescSet( ad.descSet() );
    else
    {
	inpfld->setDesc( inpdesc );
	inpfld->updateHistory( adsman->inputHistory() );
    }
}


void uiAttrDescEd::putInp( uiSteerCubeSel* inpfld, const Attrib::Desc& ad, 
			   int inpnr)
{
    const Attrib::Desc* inpdesc = ad.getInput( inpnr );
    if ( !inpdesc )
        inpfld->setDescSet( ad.descSet() );
    else
    {
	inpfld->setDesc( inpdesc );
	inpfld->updateHistory( adsman->inputHistory() );
    }
}


BufferString uiAttrDescEd::gateLabel() const
{
    BufferString lbl( SI().zIsTime() ? "Time " : "Depth " );
    lbl += "gate "; lbl += SI().getZUnit();
    return lbl;
}


BufferString uiAttrDescEd::shiftLabel() const
{
    BufferString lbl( SI().zIsTime() ? "Time " : "Depth " );
    lbl += "shift "; lbl += SI().getZUnit();
    return lbl;
}


bool uiAttrDescEd::zIsTime() const
{
    return SI().zIsTime();
}
    

const char* uiAttrDescEd::commit( Attrib::Desc* editdesc )
{
    if ( !editdesc ) editdesc = attrdesc;
    if ( !editdesc ) return 0;

    getParameters( *editdesc );
    getInput( *editdesc );
    getOutput( *editdesc );
    return 0;
}


bool uiAttrDescEd::getOutput( Attrib::Desc& desc )
{
    desc.selectOutput( 0 );
    return true;
}
