/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          25/05/2000
 RCS:           $Id: uigeninput.cc,v 1.7 2001-05-03 13:20:58 bert Exp $
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

//! maps a uiGenInput's idx to a field- and sub-idx
class FieldIdx
{
public:
                FieldIdx( int fidx, int sidx )
                    : fldidx( fidx ), subidx( sidx ) {}

    bool	operator ==( const FieldIdx& idx ) const
		{ return fldidx == idx.fldidx && subidx == idx.subidx; }

    int         fldidx;
    int         subidx;
};


/*! \brief Generalised data input field.

Provides a generalized interface towards data inputs from the user interface.
The default implementation converts everything to and from a string.

Of course it doesn't make much sense to use f.e. setBoolValue on an element
that is supposed to input double precision float's, but that's up to the
programmer to decide.

*/

class uiDataInpFld : public CallBacker
{
public:
                        uiDataInpFld() {}

    virtual const char* text( int idx ) const =0;

    virtual int		nElems()			{ return 1; }
    virtual bool	isValid(int elemidx) const	{ return true; }

    virtual int         getIntValue( int idx ) const
			    { return text(idx) ? atoi( text(idx) ) : 0; }
    virtual double      getValue( int idx ) const
			    { return text(idx) ? atof( text(idx) ) : 0; }
    virtual float       getfValue( int idx ) const
			    { return text(idx) ? atof( text(idx) ) : 0; }
    virtual bool        getBoolValue( int idx ) const
			    { return yesNoFromString( text(idx) ); }

    virtual void        setText( const char*, int idx ) =0;
    virtual void        setValue( int i, int idx )
			    { setText( getStringFromInt(0, i ),idx); }
    virtual void        setValue( double d, int idx )
			    { setText( getStringFromDouble(0, d ),idx); }
    virtual void        setValue( float f, int idx )
			    { setText( getStringFromFloat(0, f ),idx); }
    virtual void        setValue( bool b, int idx )
			    { setText( getYesNoString( b ),idx); }

    virtual void        clear()				
			    { 
				for( int idx=0; idx<nElems(); idx++ ) 
				    setText("",idx); 
			    }

    virtual void        setReadOnly( bool = true )	{}
    virtual bool        isReadOnly() const		{ return false; }

                        // can be a uiGroup, i.e. for radio button group
    virtual uiObject&	uiObj() =0;
};


class uiLineInpFld : public uiDataInpFld
{
public:
			uiLineInpFld( uiObject* p, 
					 const DataInpSpec* spec=0,
					 const char* nm="Line Edit Field" ) 
			: li( *new uiLineEdit(p,0,nm) ) 
			{
			    if( spec )
			    {
				BufferString tmp;
				spec->getText( tmp );
				li.setText( tmp );

				const StringInpSpec* dsc 
				    = dynamic_cast<const StringInpSpec*>(spec);

				int pw = dsc ? dsc->prefWidth() : -1;
				if( pw >= 0 ) li.setPrefWidthInChar( pw );
			    }
			}

    virtual const char*	text(int idx) const		{ return li.text(); }
    virtual void        setText( const char* t,int idx)	{ return li.setText(t);}
    virtual uiObject&	uiObj()				{ return li; }

protected:
    uiLineEdit&	li;
};


class uiBoolInpFld : public uiDataInpFld
{
public:

			uiBoolInpFld( uiObject* p, 
					 const DataInpSpec* spec=0,
					 const char* nm="Bool Input Field" );

    virtual const char*	text(int idx) const{ return yn ? truetxt : falsetxt; }
    virtual void        setText( const char* t, int idx)	
			    {  
				if( t == truetxt ) yn = true;
				else if( t == falsetxt ) yn = false;
				else yn = yesNoFromString(t);

				setValue(yn);
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

uiBoolInpFld::uiBoolInpFld( uiObject* p, const DataInpSpec* spec=0,
				    const char* nm="Bool Input Field" ) 
    : butOrGrp( 0 ) , cb( 0 ), rb1( 0 ), rb2( 0 ), yn( true )
{
    const BoolInpSpec* spc = dynamic_cast< const BoolInpSpec* >(spec);
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
    rb1->notify( mCB(this,uiBoolInpFld,radioButSel) );
    rb2 = new uiRadioButton( butOrGrp, falsetxt );
    rb2->notify( mCB(this,uiBoolInpFld,radioButSel) );

    rb2->attach( rightTo, rb1 );
    grp_->setHAlignObj( rb1 );

    setValue( yn );
}


void uiBoolInpFld::radioButSel(CallBacker* cb)
{
    if( cb == rb1 )		{ yn = rb1->isChecked(); }
    else if( cb == rb2 )	{ yn = !rb2->isChecked(); }
    else return;

    setValue( yn );
}

void uiBoolInpFld::setValue( bool b )
{ 
    yn = b; 

    if( cb ) { cb->setChecked( yn ); return; }

    if( !rb1 || !rb2 ) { pErrMsg("Huh?"); return; }

    rb1->setChecked(yn); 
    rb2->setChecked(!yn); 
}


class uiBinIDInpFld : public uiDataInpFld
{
public:

			uiBinIDInpFld( uiObject* p, 
					 const DataInpSpec* spec=0,
					 const char* nm="BinID Input Field" );

    virtual uiObject&	uiObj()				{ return binidGrp; }

    virtual const char*	text(int idx) const		
			    { return idx ? crl_y.text() : inl_x.text(); }
    virtual void        setText( const char* t,int idx)
			    { if(idx) crl_y.setText(t); else inl_x.setText(t); }

protected:

    uiLineEdit&		inl_x; // inline or x-coordinate
    uiLineEdit&		crl_y; // crossline or y-coordinate
    uiGroup&		binidGrp;

    uiPushButton*	ofrmBut; // other format: BinId / Coordinates 
    void		otherFormSel(CallBacker*);

};

uiBinIDInpFld::uiBinIDInpFld( uiObject* p, const DataInpSpec* spec,
			      const char* nm ) 
    : inl_x( *new uiLineEdit(p,0,nm) ), crl_y( *new uiLineEdit(p,0,nm) )
    , binidGrp( *new uiGroup(p,nm) ) , ofrmBut( 0 )
{
    const BinIDCoordInpSpec* spc 
			    = dynamic_cast< const BinIDCoordInpSpec* >(spec);
    if( !spc ) return;

    ofrmBut = new uiPushButton( p, spc->otherTxt() );
    ofrmBut->notify( mCB(this,uiBinIDInpFld,otherFormSel) );

}


void uiBinIDInpFld::otherFormSel(CallBacker* cb)
{
// pop dialog box
// transform using SI()
// set value
}

template<class T>
class uiIntervalInpFld : public uiDataInpFld
{
public:

			uiIntervalInpFld( uiObject* p, 
					 const DataInpSpec* spec=0,
					 const char* nm="Bool Input Field" );

    //virtual bool        getBoolValue() const	{ return yn; }

    virtual uiObject&	uiObj()			{ return intvalGrp; }

    virtual const char*	text(int idx) const		
			    { return  le(idx) ? le(idx)->text() : 0; }
    virtual void        setText( const char* t,int idx)	
			    { if(le(idx)) le(idx)->setText(t); }

protected:
    uiGroup&		intvalGrp;

    uiLineEdit&		start;
    uiLineEdit&		stop;
    uiLineEdit*		step;

    inline const uiLineEdit* le( int idx ) const 
			{ 
			    return const_cast<uiLineEdit*>
				(const_cast<uiIntervalInpFld*>(this)->le(idx));
			}
    uiLineEdit*		le( int idx ) 
			{ 
			    if( idx>1 ) return step;
			    return idx ? &stop : &start;
			}
};

template<class T>
uiIntervalInpFld<T>::uiIntervalInpFld<T>(uiObject* p, const DataInpSpec* spec=0,
				    const char* nm="Bool Input Field" ) 
    : intvalGrp( *new uiGroup(p,nm) ) 
    , start( *new uiLineEdit(p,0,nm) )
    , stop( *new uiLineEdit(p,0,nm) )
    , step( 0 )
{
    const NumInpIntervalSpec<T>* spc = 
			dynamic_cast< const NumInpIntervalSpec<T>* >(spec);
    if(!spc) return;

    start.setValue(spc->value(0));
    stop.setValue(spc->value(1));

    if( spc-> hasStep() )
    {
	step = new uiLineEdit(p,0,nm);
	step->setValue(spc->value(2));
    }
}

/*!

creates a new InpFld and attaches it rightTo the last one
already present in 'flds'.

*/
uiDataInpFld& uiGenInput::createInpFld( const DataInpSpec* desc )
{
    uiDataInpFld* fld;

    if( !desc )
	{ fld = new uiLineInpFld( this ); }
    else
    {
	switch( desc->type() )
	{
	    case DataInpSpec::stringTp:
	    case DataInpSpec::fileNmTp:
		{
		    fld = new uiLineInpFld( this, desc ); 
		}
		break;

	    case DataInpSpec::floatTp:
	    case DataInpSpec::doubleTp:
	    case DataInpSpec::intTp:
		{
		    fld = new uiLineInpFld( this, desc ); 
		}
		break;

	    case DataInpSpec::boolTp:
		{
		    fld = new uiBoolInpFld( this, desc ); 
		}
		break;

	    case DataInpSpec::intIntervalTp:
		{
		    fld = new uiIntervalInpFld<int>( this, desc ); 
		}
		break;
	    case DataInpSpec::floatIntervalTp:
		{
		    fld = new uiIntervalInpFld<float>( this, desc ); 
		}
		break;
	    case DataInpSpec::doubleIntervalTp:
		{
		    fld = new uiIntervalInpFld<double>( this, desc ); 
		}
		break;

	    case DataInpSpec::binIDCoordTp:
		{
		    fld = new uiBinIDInpFld( this, desc ); 
		}
		break;
#ifdef TODO
	    case DataInpSpec::stringListTp:
		{
		    fld = new uiStrLstInpFld( this, desc ); 
		}
		break;
#endif
	    default:
		{
		    fld = new uiLineInpFld( this, desc ); 
		}
		break;
	}
    }

    uiObject* other= flds.size() ? &flds[ flds.size()-1 ]->uiObj() : 0;
    if( other ) fld->uiObj().attach( rightTo, other );

    flds += fld;

    for( int idx=0; idx<fld->nElems(); idx++ )
	idxes += FieldIdx( flds.size()-1, idx );

    return *fld;
}


//-----------------------------------------------------------------------------


uiGenInput::uiGenInput( uiObject* p, const char* disptxt)
    : uiGroup( p, disptxt )
    , selText("") , withchk(false) , withclr(false)
    , labl(0), cbox(0), selbut(0), clrbut(0)
    , checked( this ), changed( this )
    , checked_(false), ro(false)
{}

uiGenInput::uiGenInput( uiObject* p, const char* disptxt
	    , const DataInpSpec& inp1 )
    : uiGroup( p, disptxt )
    , selText("") , withchk(false) , withclr(false)
    , labl(0), cbox(0), selbut(0), clrbut(0)
    , checked( this ), changed( this )
    , checked_(false), ro(false)
{
    inputs += inp1.clone();
}


uiGenInput::uiGenInput( uiObject* p, const char* disptxt
	    , const DataInpSpec& inp1 , const DataInpSpec& inp2 )
    : uiGroup( p, disptxt )
    , selText("") , withchk(false) , withclr(false)
    , labl(0), cbox(0), selbut(0), clrbut(0)
    , checked( this ), changed( this )
    , checked_(false), ro(false)
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
    , checked( this ), changed( this )
    , checked_(false), ro(false)
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
	lastElem = &createInpFld( inputs[0] ).uiObj();
	setHAlignObj( lastElem );
    }

    if( withchk )
    {
	cbox = new uiCheckBox( this, name() );
	cbox->attach( leftTo, lastElem );
	cbox->notify( mCB(this,uiGenInput,checkBoxSel) );
	setChecked( checked_ );
    }
    else if( *name() ) 
    {
	labl = new uiLabel( this, name() );
	labl->attach( leftTo, lastElem );
    }

    for( int i=1; i<inputs.size(); i++ )
	lastElem = &createInpFld( inputs[i] ).uiObj();

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
	return nr<idxes.size() && flds[idxes[nr].fldidx] \
		? flds[idxes[nr].fldidx]->fn(idxes[nr].subidx) : undefval; \
    }

#define mFromLE_s(fn,typ,var) \
    void uiGenInput::fn( typ var, int nr ) \
    { \
	mCheckFinalised();\
	if( nr<idxes.size() && flds[idxes[nr].fldidx] )\
	    flds[idxes[nr].fldidx]->fn( var, idxes[nr].subidx ); \
    }

mFromLE_g(isValid,bool, false )

mFromLE_g(text,const char*, sUndefValue )
mFromLE_g(getIntValue,int, 0 )
mFromLE_g(getValue,double, mUndefValue )
mFromLE_g(getfValue,float, mUndefValue )
mFromLE_g(getBoolValue,bool, false )
mFromLE_s(setText,const char*,s)
mFromLE_s(setValue,int,i)
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
    { checked_ = yn; if( cbox ) cbox->setChecked( yn ); }


bool uiGenInput::isChecked()
    { return checked_; }


void uiGenInput::checkBoxSel( CallBacker* cb )
{
    checked_ = cbox->isChecked();

    for( int idx=0; idx < flds.size(); idx++ )
	flds[idx]->uiObj().setSensitive( isChecked() );

    if( selbut ) selbut->setSensitive( isChecked() );
    if( clrbut ) clrbut->setSensitive( isChecked() );
}


void uiGenInput::doSelect_( CallBacker* cb )
    { doSelect( cb ); }


void uiGenInput::doClear( CallBacker* )
    { clear(); }

