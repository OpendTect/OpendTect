/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          25/05/2000
 RCS:           $Id: uigeninput.cc,v 1.2 2001-01-24 12:58:59 arend Exp $
________________________________________________________________________

-*/

#include "uigeninput.h"
#include "uilineedit.h"
#include "uilabel.h"
#include "uibutton.h"

static FixedString<20> s2;

void uiGenInpLine::setReadOnly( bool yn, int nr )
{
    if( nr >= 0 ) { leds[nr]->setReadOnly( yn ); return; }

    for( int idx=0; idx < leds.size(); idx++ )
	leds[idx]->setReadOnly( yn );
}


void uiGenInpLine::clearEdits()
{
    for( int idx=0; idx < leds.size(); idx++ )
	leds[idx]->setText( "" );
}

#define mFromLE_g(fn,ret) \
    ret uiGenInpLine::fn( int nr ) const \
    { \
	return leds[nr]->fn(); \
    }

#define mFromLE_s(fn,typ,var) \
    void uiGenInpLine::fn( typ var, int nr ) \
    { \
	leds[nr]->fn( var ); \
    }

mFromLE_g(text,const char*)
mFromLE_g(getIntValue,int)
mFromLE_g(getValue,double)
mFromLE_s(setText,const char*,s)
mFromLE_s(setValue,float,f)
mFromLE_s(setValue,double,d)


#define mGenInputConstructorsImpl( GenInpClss ) \
    GenInpClss::GenInpClss( uiObject* p, const char* disptxt, const char* t )\
	    : uiGenInpLine(p,"Geninput: string")\
    {\
	init( disptxt, t, 0 );\
    }\
    \
    \
    GenInpClss::GenInpClss( uiObject* p, const char* disptxt,\
			    const char* t1, const char* t2 )\
	    : uiGenInpLine(p,"Geninput: string string")\
    {\
	init( disptxt, t1, t2 );\
    }\
    \
    \
    GenInpClss::GenInpClss( uiObject* p, const char* disptxt, int v )\
	    : uiGenInpLine(p,"Geninput: int")\
    {\
	init( disptxt, getStringFromInt(0,v), 0 );\
    }\
    \
    \
    GenInpClss::GenInpClss( uiObject* p, const char* disptxt, float v )\
	    : uiGenInpLine(p,"Geninput: float")\
    {\
	init( disptxt, getStringFromFloat(0,v), 0 );\
    }\
    \
    \
    GenInpClss::GenInpClss( uiObject* p, const char* disptxt, double v )\
	    : uiGenInpLine(p,"Geninput: double")\
    {\
	init( disptxt, getStringFromDouble(0,v), 0 );\
    }\
    \
    \
    GenInpClss::GenInpClss( uiObject* p, const char* disptxt, int v1, int v2 )\
	    : uiGenInpLine(p,"Geninput: int int")\
    {\
	s2 = getStringFromInt(0,v2);\
	init( disptxt, getStringFromInt(0,v1), s2 );\
    }\
    \
    \
    GenInpClss::GenInpClss( uiObject* p, const char* disptxt, float v1,\
			    float v2 )\
	    : uiGenInpLine(p,"Geninput: float float")\
    {\
	s2 = getStringFromFloat(0,v2);\
	init( disptxt, getStringFromFloat(0,v1), s2 );\
    }\
    \
    \
    GenInpClss::GenInpClss( uiObject* p, const char* disptxt, double v1, \
			    double v2)\
	    : uiGenInpLine(p,"Geninput: double double")\
    {\
	s2 = getStringFromDouble(0,v2);\
	init( disptxt, getStringFromDouble(0,v1), s2 );\
    }

mGenInputConstructorsImpl( uiGenCheckInput )
mGenInputConstructorsImpl( uiGenInput )


void uiGenInput::setTitleText( const char* txt )
    { labl->setText( txt ); }


const char* uiGenInput::titleText()
    { return labl->text(); }


void uiGenCheckInput::setTitleText( const char* txt )
    { cbox->setText( txt ); }


const char* uiGenCheckInput::titleText()
    { return cbox->text(); }


void uiGenCheckInput::setChecked( bool yn )
    { cbox->setChecked( yn ); }


bool uiGenCheckInput::isChecked()
    { return cbox->isChecked(); }


/*!
    Must have the same interface for both uiGenInput and uiGenCheckInput
    because it is called from the mGenInputConstructorsImpl macro.
*/
void uiGenInput::init( const char* disptxt, const char* t1, const char* t2 )
{
    uiLineEdit* le = new uiLineEdit( this, t1 );
    labl = new uiLabel( this, disptxt, le );
    setHAlignObj( le );
    leds += le;

    le = t2 ? new uiLineEdit( this, t2 ) : 0;
    if ( le ){
	leds += le;
	le->attach( rightOf, leds[0] );
    }
}


/*!
    Must have the same interface for both uiGenInput and uiGenCheckInput
    because it is called from the mGenInputConstructorsImpl macro.
*/
void uiGenCheckInput::init(const char* disptxt, const char* t1, const char* t2 )
{
    uiLineEdit *le = new uiLineEdit( this, t1 );

    cbox = new uiCheckBox( this, disptxt );
    cbox->attach( leftTo, le );
    cbox->notify( mCB(this,uiGenCheckInput,checkBoxSel) );


    setHAlignObj( le );
    leds += le;

    le = t2 ? new uiLineEdit( this, t2 ) : 0;
    if ( le ){
	leds += le;
	le->attach( rightOf, leds[0] );
    }
}

void uiGenCheckInput::checkBoxSel_( CallBacker* cb )
{

    for( int idx=0; idx < leds.size(); idx++ )
	leds[idx]->setSensitive( cbox->isChecked() );
}
