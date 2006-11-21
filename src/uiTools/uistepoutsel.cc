/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          April 2001
________________________________________________________________________

-*/

static const char* rcsID = "$Id: uistepoutsel.cc,v 1.1 2006-11-21 17:47:26 cvsbert Exp $";

#include "uistepoutsel.h"
#include "uigeninput.h"
#include "separstr.h"


uiStepOutSel::uiStepOutSel( uiParent* p, const char* seltxt, bool sngl )
    : uiGroup(p,"Stepout")
    , is2d(false)
    , shown(true)
    , single(sngl)
    , valueChanged(this)
{
    BufferString seltxt2d = "Trace Stepout";
    BufferString seltxt3d = "Inl/Crl Stepout";
    if ( seltxt )
    {
	FileMultiString fms( seltxt );
	seltxt2d = fms[0];
	if ( fms.size() > 1 )
	    seltxt3d = fms[1];
	else
	    seltxt3d = seltxt2d;
    }
    inp2dfld = new uiGenInput( this, seltxt2d, IntInpSpec() );
    inp2dfld->valuechanging.notify( mCB(this,uiStepOutSel,valChg) );
    inp3dfld = single ? new uiGenInput( this, seltxt3d, IntInpSpec() )
	     : new uiGenInput( this, seltxt3d, BinIDInpSpec() );
    inp3dfld->valuechanging.notify( mCB(this,uiStepOutSel,valChg) );

    setHAlignObj( inp2dfld );
    mainObject()->finaliseDone.notify( mCB(this,uiStepOutSel,update) );
}


void uiStepOutSel::update( CallBacker* )
{
    inp2dfld->display( is2d && shown );
    inp3dfld->display( !is2d && shown );
}


void uiStepOutSel::set2D( bool yn )
{
    is2d = yn;
    update(0);
}


void uiStepOutSel::display( bool yn, bool shrk, bool maximz )
{
    shown = yn;
    uiGroup::display( yn, shrk, maximz );
    update(0);
}


int uiStepOutSel::val( bool inl ) const
{
    if ( is2d )
	return inl ? 0 : inp2dfld->getIntValue(0);
    else
	return inp3dfld->getIntValue( (single||inl) ? 0 : 1 );
}


void uiStepOutSel::setVal( bool inl, int v )
{
    if ( !inl )
	inp2dfld->setValue( v, 0 );

    if ( single || inl )
	inp3dfld->setValue( v, 0 );
    else
	inp3dfld->setValue( v, 1 );
}


void uiStepOutSel::valChg( CallBacker* cb )
{
    valueChanged.trigger( cb );
}
