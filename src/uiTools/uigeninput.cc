/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          25/05/2000
 RCS:           $Id: uigeninput.cc,v 1.1 2000-11-27 10:20:46 bert Exp $
________________________________________________________________________

-*/

#include "uigeninput.h"
#include "uilineedit.h"
#include "uilabel.h"

static FixedString<20> s2;


void uiGenInput::init( const char* disptxt, const char* t1, const char* t2 )
{
    setBorder( 0 );
    le[0] = new uiLineEdit( this, t1 );
    labl = new uiLabel( this, disptxt, le[0] );
    setHAlignObj( le[0] );

    le[1] = t2 ? new uiLineEdit( this, t2 ) : 0;
    if ( le[1] )
	le[1]->attach( rightOf, le[0] );
}


uiGenInput::uiGenInput( uiObject* p, const char* disptxt, const char* t )
	: uiGroup(p,"Geninput: string")
{
    init( disptxt, t, 0 );
}


uiGenInput::uiGenInput( uiObject* p, const char* disptxt,
			const char* t1, const char* t2 )
	: uiGroup(p,"Geninput: string string")
{
    init( disptxt, t1, t2 );
}


uiGenInput::uiGenInput( uiObject* p, const char* disptxt, int v )
	: uiGroup(p,"Geninput: int")
{
    init( disptxt, getStringFromInt(0,v), 0 );
}


uiGenInput::uiGenInput( uiObject* p, const char* disptxt, float v )
	: uiGroup(p,"Geninput: float")
{
    init( disptxt, getStringFromFloat(0,v), 0 );
}


uiGenInput::uiGenInput( uiObject* p, const char* disptxt, double v )
	: uiGroup(p,"Geninput: double")
{
    init( disptxt, getStringFromDouble(0,v), 0 );
}


uiGenInput::uiGenInput( uiObject* p, const char* disptxt, int v1, int v2 )
	: uiGroup(p,"Geninput: int int")
{
    s2 = getStringFromInt(0,v2);
    init( disptxt, getStringFromInt(0,v1), s2 );
}


uiGenInput::uiGenInput( uiObject* p, const char* disptxt, float v1, float v2 )
	: uiGroup(p,"Geninput: float float")
{
    s2 = getStringFromFloat(0,v2);
    init( disptxt, getStringFromFloat(0,v1), s2 );
}


uiGenInput::uiGenInput( uiObject* p, const char* disptxt, double v1, double v2 )
	: uiGroup(p,"Geninput: double double")
{
    s2 = getStringFromDouble(0,v2);
    init( disptxt, getStringFromDouble(0,v1), s2 );
}


#define mFromLE_g(fn,ret) \
    ret uiGenInput::fn( int nr ) const \
    { \
	return le[nr]->fn(); \
    }

#define mFromLE_s(fn,typ,var) \
    void uiGenInput::fn( typ var, int nr ) \
    { \
	le[nr]->fn( var ); \
    }

mFromLE_g(text,const char*)
mFromLE_g(getIntValue,int)
mFromLE_g(getValue,double)
mFromLE_s(setText,const char*,s)
mFromLE_s(setValue,float,f)
mFromLE_s(setValue,double,d)
