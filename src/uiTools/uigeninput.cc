/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          25/05/2000
 RCS:           $Id: uigeninput.cc,v 1.3 2001-02-16 17:02:21 arend Exp $
________________________________________________________________________

-*/

#include "uigeninput.h"
#include "uilineedit.h"
#include "uilabel.h"
#include "uibutton.h"
#include <datainpspec.h>

#define mCheckFinalised() \
    if( ! finalised ) \
    { pErrMsg( "Do not use before popped up" ); return; }

#define mCheckFinalisedv( retval ) \
    if( ! finalised ) \
    { pErrMsg( "Do not use before popped up" ); return retval; }



/*! \brief Generalised data input field.

Provides a generalized interface towards data inputs from the user interface.
The default implementation converts everything to and from a string.

Of course it doesn't make much sense to use f.e. setBoolValue on an element
that is supposed to input double precision float's, but that's up to the
programmer to decide.

*/

class uiDataInpField : public CallBacker
{
public:
                        uiDataInpField() {}

    virtual const char* text() const =0;

    virtual int         getIntValue() const
			    { return text() ? atoi( text() ) : 0; }
    virtual double      getValue() const
			    { return text() ? atof( text() ) : 0; }
    virtual float       getfValue() const
			    { return text() ? atof( text() ) : 0; }
    virtual bool        getBoolValue() const
			    { return yesNoFromString( text() ); }

    virtual void        setText( const char* ) =0;
    virtual void        setValue( int i )
			    { setText( getStringFromInt(0, i )); }
    virtual void        setValue( double d )
			    { setText( getStringFromDouble(0, d )); }
    virtual void        setValue( float f )
			    { setText( getStringFromFloat(0, f )); }
    virtual void        setValue( bool b )
			    { setText( getYesNoString( b )); }

    virtual void        clear()				{ setText(""); }

    virtual void        setReadOnly( bool = true )	{}
    virtual bool        isReadOnly() const		{ return false; }

                        // can be a uiGroup, i.e. for radio button group
    virtual uiObject&	uiObj() =0;
};


class uiLineEditField : public uiDataInpField
{
public:
			uiLineEditField( uiObject* p, 
					 const DataInpSpec* spec=0,
					 const char* nm="Line Edit Field" ) 
			: li( *new uiLineEdit(p,0,nm) ) 
			{
			    if( spec )
			    {
				BufferString tmp;
				spec->getText( tmp );
				li.setText( tmp );

				const StringInp* dsc 
				    = dynamic_cast< const StringInp* >(spec);

				int pw = dsc ? dsc->prefWidth() : -1;
				if( pw >= 0 ) li.setPrefWidthInChar( pw );
			    }
			}

    virtual const char*	text() const		{ return li.text(); }
    virtual void        setText( const char* t)	{ return li.setText(t); }
    virtual uiObject&	uiObj()			{ return li; }

protected:
    uiLineEdit&	li;
};


class uiBoolInputField : public uiDataInpField
{
public:

			uiBoolInputField( uiObject* p, 
					 const DataInpSpec* spec=0,
					 const char* nm="Bool Input Field" );

    virtual const char*	text() const	{ return yn ? truetxt : falsetxt; }
    virtual void        setText( const char* t)	
			    {  
				if( t == truetxt ) yn = true;
				else if( t == falsetxt ) yn = false;
				else yn = yesNoFromString(t);
			    }

    virtual bool        getBoolValue() const	{ return yn; }
    virtual void        setValue( bool b );

    virtual uiObject&	uiObj()			{ return *butOrGrp; }

protected:

    void		radioButSel(CallBacker*);

    BufferString        truetxt;
    BufferString        falsetxt;
    bool                yn;
    uiObject*		butOrGrp; //! either uiButton or uiButtonGroup
    uiCheckBox*		cb;
    uiRadioButton*	rb1;
    uiRadioButton*	rb2;
};

uiBoolInputField::uiBoolInputField( uiObject* p, const DataInpSpec* spec=0,
				    const char* nm="Bool Input Field" ) 
    : butOrGrp( 0 ) , cb( 0 ), rb1( 0 ), rb2( 0 ), yn( true )
{
    const BoolInp* spc = dynamic_cast< const BoolInp* >(spec);
    if( !spc ) { butOrGrp = new uiGroup(p,nm); return; }

    yn=spc->checked();
    truetxt = spc->trueFalseTxt(true);
    falsetxt = spc->trueFalseTxt(false);

    if( truetxt == "" )
    { 
	cb = new uiCheckBox( p, nm ); 
	butOrGrp = cb;
	setValue( yn );
	return; 
    }

    if( falsetxt == "" )
    { 
	cb = new uiCheckBox( p, truetxt );
	butOrGrp = cb;
	setValue( yn );
	return; 
    }

    // we have two labelTxt()'s, so we'll make radio buttons
    uiGroup* grp_ = new uiGroup( p, nm ); 
    butOrGrp = grp_;

    rb1 = new uiRadioButton( butOrGrp, truetxt );
    rb1->notify( mCB(this,uiBoolInputField,radioButSel) );
    rb2 = new uiRadioButton( butOrGrp, falsetxt );
    rb2->notify( mCB(this,uiBoolInputField,radioButSel) );

    rb2->attach( rightTo, rb1 );
    grp_->setHAlignObj( rb1 );

    setValue( yn );
}


void uiBoolInputField::radioButSel(CallBacker* cb)
{
    if( cb == rb1 )		{ yn = rb1->isChecked(); }
    else if( cb == rb2 )	{ yn = !rb2->isChecked(); }
    else return;

    setValue( yn );
}

void uiBoolInputField::setValue( bool b )
{ 
    yn = b; 

    if( cb ) { cb->setChecked( yn ); return; }

    if( !rb1 || !rb2 ) { pErrMsg("Huh?"); return; }

    rb1->setChecked(yn); 
    rb2->setChecked(!yn); 
}


/*!

creates a new inputfield and attaches it rightTo the last one
already present in 'flds'.

*/
uiDataInpField& uiGenInput::createInpField( uiObject* p, 
					    const DataInpSpec* desc )
{
    uiDataInpField* fld;

    if( !desc )
	{ fld = new uiLineEditField( p ); }
    else
    {
	switch( desc->type() )
	{
	    case DataInpSpec::stringTp:
	    case DataInpSpec::fileNmTp:
		{
		    fld = new uiLineEditField( p, desc ); 
		}
		break;

	    case DataInpSpec::floatTp:
	    case DataInpSpec::doubleTp:
	    case DataInpSpec::intTp:
		{
		    fld = new uiLineEditField( p, desc ); 
		}
		break;

	    case DataInpSpec::boolTp:
		{
		    fld = new uiBoolInputField( p, desc ); 
		}
		break;
	    default:
		{
		    fld = new uiLineEditField( p, desc ); 
		}
		break;
	}
    }

    uiObject* other= flds.size() ? &flds[ flds.size()-1 ]->uiObj() : 0;
    if( other ) fld->uiObj().attach( rightTo, other );

    flds += fld;
    return *fld;
}


//-----------------------------------------------------------------------------


uiGenInput::uiGenInput( uiObject* p, const char* disptxt)
    : uiGroup( p, disptxt )
    , selText("") , withchk(false) , withclr(false)
    , labl(0), cbox(0), selbut(0), clrbut(0)
    , checked(false), ro(false)
{}

uiGenInput::uiGenInput( uiObject* p, const char* disptxt
	    , const DataInpSpec& inp1 )
    : uiGroup( p, disptxt )
    , selText("") , withchk(false) , withclr(false)
    , labl(0), cbox(0), selbut(0), clrbut(0)
    , checked(false), ro(false)
{
    inputs += inp1.clone();
}


uiGenInput::uiGenInput( uiObject* p, const char* disptxt
	    , const DataInpSpec& inp1 , const DataInpSpec& inp2 )
    : uiGroup( p, disptxt )
    , selText("") , withchk(false) , withclr(false)
    , labl(0), cbox(0), selbut(0), clrbut(0)
    , checked(false), ro(false)
{
    inputs += inp1.clone();
    inputs += inp2.clone();
}


uiGenInput::uiGenInput( uiObject* p, const char* disptxt
	    , const DataInpSpec& inp1, const DataInpSpec& inp2 
	    , const DataInpSpec& inp3 )
    : uiGroup( p, disptxt )
    , selText("") , withchk(false) , withclr(false)
    , labl(0), cbox(0), selbut(0), clrbut(0)
    , checked(false), ro(false)
{
    inputs += inp1.clone();
    inputs += inp2.clone();
    inputs += inp3.clone();
}


void uiGenInput::addInput( const DataInpSpec& inp )
{
    mCheckFinalised();
    inputs += inp.clone();
}


DataInpSpec* uiGenInput::getInput( int nr )  
{ 
    if( finalised ) 
	{ pErrMsg("Don't use when already finalised") ; return 0; }
    return ( nr<inputs.size() && inputs[nr] ) ? inputs[nr] : 0;
}


void uiGenInput::finalise_()
{
    uiGroup::finalise_();
    uiObject *lastElem = 0;
    if( inputs.size() )
    {
	lastElem = &createInpField( this, inputs[0] ).uiObj();
	setHAlignObj( lastElem );
    }

    if( withchk )
    {
	cbox = new uiCheckBox( this, name() );
	cbox->attach( leftTo, lastElem );
	cbox->notify( mCB(this,uiGenInput,checkBoxSel) );
	setChecked( checked );
    }
    else if( *name() ) 
    {
	labl = new uiLabel( this, name() );
	labl->attach( leftTo, lastElem );
    }

    for( int i=1; i<inputs.size(); i++ )
	lastElem = &createInpField( this, inputs[i] ).uiObj();

    if( selText != "" )
    {
	selbut = new uiPushButton( this, selText );
	selbut->notify( mCB(this,uiGenInput,doSelect_) );
	selbut->attach( rightOf, lastElem );
    }

    if ( withclr )
    {
	clrbut = new uiPushButton( this, "Clear ..." );
	clrbut->attach( rightOf, selbut ? selbut : lastElem );
	clrbut->notify( mCB(this,uiGenInput,doClear) );
    }

    setReadOnly( ro );
    deepErase( inputs );
}



void uiGenInput::setReadOnly( bool yn, int nr )
{
    mCheckFinalised();
    if( nr >= 0  ) 
	{ if( nr<flds.size() && flds[nr] ) flds[nr]->setReadOnly(yn); return; }

    ro = yn;

    for( int idx=0; idx < flds.size(); idx++ )
	flds[idx]->setReadOnly( yn );
}


void uiGenInput::clear( int nr )
{
    mCheckFinalised();
    if( nr >= 0 )
	{ if( nr<flds.size() && flds[nr] ) flds[nr]->clear(); return; }

    for( int idx=0; idx < flds.size(); idx++ )
	flds[idx]->clear();
}

#define mFromLE_g(fn,ret, undefval ) \
    ret uiGenInput::fn( int nr ) const \
    { \
	mCheckFinalisedv( undefval );\
	return nr<flds.size() && flds[nr] ? flds[nr]->fn() : undefval; \
    }

#define mFromLE_s(fn,typ,var) \
    void uiGenInput::fn( typ var, int nr ) \
    { \
	mCheckFinalised();\
	flds[nr]->fn( var ); \
    }

mFromLE_g(text,const char*, sUndefValue )
mFromLE_g(getIntValue,int, 0 )
mFromLE_g(getValue,double, mUndefValue )
mFromLE_g(getfValue,float, mUndefValue )
mFromLE_g(getBoolValue,bool, false )
mFromLE_s(setText,const char*,s)
mFromLE_s(setValue,float,f)
mFromLE_s(setValue,double,d)
mFromLE_s(setValue,bool,yn)


const char* uiGenInput::titleText()
{ 
    if( labl ) return labl->text(); 
    if( cbox ) return cbox->text(); 
    return 0;
}


void uiGenInput::setTitleText( const char* txt )
{ 
    if( labl ) labl->setText( txt );
    if( cbox ) cbox->setText( txt ); 
}


void uiGenInput::setChecked( bool yn )
    { checked = yn; if( cbox ) cbox->setChecked( yn ); }


bool uiGenInput::isChecked()
    { return checked; }


void uiGenInput::checkBoxSel( CallBacker* cb )
{
    checked = cbox->isChecked();

    for( int idx=0; idx < flds.size(); idx++ )
	flds[idx]->uiObj().setSensitive( isChecked() );

    if( selbut ) selbut->setSensitive( isChecked() );
    if( clrbut ) clrbut->setSensitive( isChecked() );
}


void uiGenInput::doSelect_( CallBacker* cb )
    { doSelect( cb ); }


void uiGenInput::doClear( CallBacker* )
    { clear(); }

