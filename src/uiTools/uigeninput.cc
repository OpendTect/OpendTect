/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          25/05/2000
 RCS:           $Id: uigeninput.cc,v 1.16 2001-05-10 12:50:29 arend Exp $
________________________________________________________________________

-*/

#include "uigeninput.h"
#include "uilineedit.h"
#include "uilabel.h"
#include "uibutton.h"
#include "uicombobox.h"
#include "datainpspec.h"
#include "survinfo.h"


#define mCheckFinalised() \
    if ( ! finalised ) \
    { pErrMsg( "Do not use before popped up" ); return; }

#define mCheckFinalisedv( retval ) \
    if ( ! finalised ) \
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

Of course it doesn't make much sense to use f.e. setBoolValue on an element
that is supposed to input double precision float's, but that's up to the
programmer to decide.

*/

class uiDataInpFld : public CallBacker
{
public:
                        uiDataInpFld( const DataInpSpec& spec ) 
			: spec_( *spec.clone() ) {}

    virtual const char* text( int idx ) const =0;

    virtual int		nElems()			{ return 1; }
    virtual uiObject&	element( int  )			{ return uiObj(); }
    virtual bool	isValid(int idx)
			{ 
			    pErrMsg("Sorry, not implemented..");
			    if ( isUndef(idx) ) return false;
			    if ( !spec_.hasLimits() ) return true;
			    return true;
			} // TODO implement
    virtual bool	isUndef(int idx) const
			    { return !*text(idx); } 

    virtual int         getIntValue( int idx )	const	=0;
    virtual double      getValue( int idx )	const	=0;
    virtual float       getfValue( int idx )	const	=0;
    virtual bool        getBoolValue( int idx )	const	=0;

    virtual void        setText( const char*, int idx ) =0;
    virtual void        setValue( int i, int idx )	=0;
    virtual void        setValue( double d, int idx )	=0;
    virtual void        setValue( float f, int idx )	=0;
    virtual void        setValue( bool b, int idx )	=0;

    virtual void        clear()				=0;

    virtual void        setReadOnly( bool = true )	{}
    virtual bool        isReadOnly() const		{ return false; }

                        // can be a uiGroup, i.e. for radio button group
    virtual uiObject&	uiObj() =0;

    virtual void	changeNotify( const CallBack& cb ) = 0;

    DataInpSpec&	spec() { return spec_; }

    bool                update( const DataInpSpec* nw )
                        {
                            if (spec_.type() != nw->type()) return false;
                            return update_(nw);
                        }
protected:

    virtual bool        update_( const DataInpSpec* )
			   { return false; } // TODO: implement

    DataInpSpec&	spec_;

    void		init()
			{
			    int pw = spec_.prefFldWidth();
			    if (pw>=0) 
				for( int idx=0; idx<nElems(); idx++ )
				    element(idx).setPrefWidthInChar( pw );
			}
};

/*! \brief Text oriented data input field.

Provides a text-oriented implementation for uiDataInpFld. 
Converts everything to and from a string.

*/

class uiTextInpFld : public uiDataInpFld
{
public:
                        uiTextInpFld( const DataInpSpec& spec )
			: uiDataInpFld( spec ) {}

    virtual int         getIntValue( int idx ) const
			    { return text(idx) ? atoi( text(idx) ) : 0; }
    virtual double      getValue( int idx ) const
			    { return text(idx) ? atof( text(idx) ) : 0; }
    virtual float       getfValue( int idx ) const
			    { return text(idx) ? atof( text(idx) ) : 0; }
    virtual bool        getBoolValue( int idx ) const
			    { return yesNoFromString( text(idx) ); }

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
};

/*! \brief Int oriented general data input field.

converts everything formats to and from int

*/
class uiIntInpField : public uiDataInpFld
{
public:
                        uiIntInpField(const DataInpSpec& spec,int clearValue=0)
                        : uiDataInpFld( spec )
			, clear_(clearValue) {}

    virtual const char* text( int idx ) const
			    { return getStringFromInt(0, getIntValue(idx)); }
    virtual double      getValue( int idx ) const
			    { return (double) getIntValue( idx ); }
    virtual float       getfValue( int idx ) const
			    { return (float) getIntValue( idx ); }
    virtual bool        getBoolValue( int idx ) const
			    { return getIntValue( idx ) ? true : false ; }

    virtual void	setIntValue( int i, int idx )	=0;
    virtual void        setValue( int i, int idx )
			    { setIntValue( i , idx); }
    virtual void        setText( const char* s, int idx )
			    { setIntValue( atoi(s), idx); }
    virtual void        setValue( double d, int idx )
			    { setIntValue( mNINT(d), idx); }
    virtual void        setValue( float f, int idx )
			    { setIntValue( mNINT(f), idx); }
    virtual void        setValue( bool b, int idx )
			    { setIntValue( b ? 1 : 0, idx); }

    virtual void        clear()				
			{ 
			    for( int idx=0; idx<nElems(); idx++ ) 
				setIntValue( clear_, idx);
			}

protected:
			//! value used to clear field.
    int			clear_;

			//! stores current value as clear state.
    void		initClear() { clear_ = getIntValue(0); }
};

class uiLineInpFld : public uiTextInpFld
{
public:
			uiLineInpFld( uiObject* p, 
					 const DataInpSpec& spec,
					 const char* nm="Line Edit Field" ) 
			    : uiTextInpFld( spec )
			    , li( *new uiLineEdit(p,0,nm) ) 
			{
			    BufferString tmp;
			    spec.getText( tmp );
			    li.setText( tmp );
			    init();
			}

    virtual const char*	text(int idx) const		{ return li.text(); }
    virtual void        setText( const char* t,int idx)	{ li.setText(t);}
    virtual uiObject&	uiObj()				{ return li; }

    virtual void	changeNotify( const CallBack& cb ) 
			    { li.textChanged.notify(cb); }
protected:
    uiLineEdit&	li;
};


class uiBoolInpFld : public uiIntInpField
{
public:

			uiBoolInpFld( uiObject* p, 
				      const DataInpSpec& spec,
				      const char* nm="Bool Input Field" );

    virtual const char*	text(int idx) const{ return yn ? truetxt : falsetxt; }
    virtual void        setText( const char* t, int idx)	
			{  
			    if ( t == truetxt ) yn = true;
			    else if ( t == falsetxt ) yn = false;
			    else yn = yesNoFromString(t);

			    setValue(yn);
			}

    virtual bool	isUndef(int) const		{ return false; }

    virtual bool        getBoolValue() const		{ return yn; }
    virtual void        setValue( bool b, int idx=0 );

    virtual int         getIntValue( int idx ) const	{ return yn ? 1 : 0; }
    virtual void        setIntValue( int i, int idx )
			    { setValue( i ? true : false, idx ); }

    virtual uiObject&	uiObj()				{ return *butOrGrp; }
    virtual void	changeNotify( const CallBack& cb )
			{ changed.notify(cb); }

protected:

    void		radioButSel(CallBacker*);

    BufferString        truetxt;
    BufferString        falsetxt;
    bool                yn;
    uiObject*		butOrGrp; //! either uiButton or uiButtonGroup
    uiCheckBox*		cb;
    uiRadioButton*	rb1;
    uiRadioButton*	rb2;

    Notifier<uiBoolInpFld> changed;

};

uiBoolInpFld::uiBoolInpFld(uiObject* p, const DataInpSpec& spec, const char* nm)
    :uiIntInpField( spec )
    , butOrGrp( 0 ) , cb( 0 ), rb1( 0 ), rb2( 0 ), yn( true ), changed( this )
{
    const BoolInpSpec* spc = dynamic_cast< const BoolInpSpec* >(&spec);
    if ( !spc ) { pErrMsg("huh?");butOrGrp = new uiGroup(p,nm); return; }

    yn=spc->checked(); initClear();

    truetxt = spc->trueFalseTxt(true);
    falsetxt = spc->trueFalseTxt(false);

    if ( truetxt == "" )
    { 
	cb = new uiCheckBox( p, nm ); 
	butOrGrp = cb;
	setValue( yn );
	return; 
    }

    if ( falsetxt == "" )
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

    init();
}


void uiBoolInpFld::radioButSel(CallBacker* cb)
{
    if ( cb == rb1 )		{ yn = rb1->isChecked(); }
    else if ( cb == rb2 )	{ yn = !rb2->isChecked(); }
    else return;

    setValue( yn );
}

void uiBoolInpFld::setValue( bool b, int idx )
{ 
    if ( yn != b ) changed.trigger();
    yn = b; 

    if ( cb ) { cb->setChecked( yn ); return; }

    if ( !rb1 || !rb2 ) { pErrMsg("Huh?"); return; }

    rb1->setChecked(yn); 
    rb2->setChecked(!yn); 
}


class uiBinIDInpFld : public uiTextInpFld
{
public:

			uiBinIDInpFld( uiObject* p, 
					 const DataInpSpec& spec,
					 const char* nm="BinID Input Field" );

    virtual uiObject&	uiObj()			{ return binidGrp; }

    virtual int		nElems()		{ return 2; }
    virtual uiObject&	element( int idx  )	{ return idx ? crl_y : inl_x; }

    virtual const char*	text(int idx) const		
			    { return idx ? crl_y.text() : inl_x.text(); }
    virtual void        setText( const char* t,int idx)
			    { if (idx) crl_y.setText(t); else inl_x.setText(t);}

    virtual void	changeNotify( const CallBack& cb ) 
			{ 
			    inl_x.textChanged.notify(cb); 
			    crl_y.textChanged.notify(cb); 
			}
protected:
    // don't change order of these 3 attributes!
    uiGroup&		binidGrp;
    uiLineEdit&		inl_x; // inline or x-coordinate
    uiLineEdit&		crl_y; // crossline or y-coordinate
    const BinID2Coord*	b2c;

    uiPushButton*	ofrmBut; // other format: BinId / Coordinates 
    void		otherFormSel(CallBacker*);

};

uiBinIDInpFld::uiBinIDInpFld( uiObject* p, const DataInpSpec& spec,
			      const char* nm ) 
    : uiTextInpFld( spec )
    , binidGrp( *new uiGroup(p,nm) )
    , inl_x( *new uiLineEdit(&binidGrp,0,nm) )
    , crl_y( *new uiLineEdit(&binidGrp,0,nm) )
    , ofrmBut( 0 )
    , b2c(0)
{
    const BinIDCoordInpSpec*spc = dynamic_cast<const BinIDCoordInpSpec*>(&spec);
    if ( !spc ){ pErrMsg("huh"); return; }

    BufferString tmp;
    spec.getText( tmp, 0 ); inl_x.setText( tmp );
    spec.getText( tmp, 1 ); crl_y.setText( tmp );

    binidGrp.setHAlignObj( &inl_x );
    crl_y.attach( rightTo, &inl_x );

    if ( spc->otherTxt() )
    {
	ofrmBut = new uiPushButton( &binidGrp, spc->otherTxt() );
	ofrmBut->notify( mCB(this,uiBinIDInpFld,otherFormSel) );

	ofrmBut->attach( rightTo, &crl_y );
    }

    b2c = spc->binID2Coord();
    if ( !b2c ) b2c = &SI().binID2Coord();

    init();
}


void uiBinIDInpFld::otherFormSel(CallBacker* cb)
{
// TODO  implement:
// pop dialog box
// transform using b2c
// set value
}

template<class T>
class uiIntervalInpFld : public uiTextInpFld
{
public:

			uiIntervalInpFld( uiObject* p, 
					 const DataInpSpec& spec,
					 const char* nm="Bool Input Field" );

    virtual int		nElems()		{ return step ? 3 : 2; }
    virtual uiObject&	element( int idx  )	{ return *le(idx); }

    virtual uiObject&	uiObj()			{ return intvalGrp; }

    virtual const char*	text(int idx) const		
			    { return  le(idx) ? le(idx)->text() : 0; }
    virtual void        setText( const char* t,int idx)	
			    { if (le(idx)) le(idx)->setText(t); }

    virtual void	changeNotify( const CallBack& cb ) 
			{ 
			    start.textChanged.notify(cb); 
			    stop.textChanged.notify(cb); 
			    if (step) step->textChanged.notify(cb); 
			}
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
			    if ( idx>1 ) return step;
			    return idx ? &stop : &start;
			}
};

template<class T>
uiIntervalInpFld<T>::uiIntervalInpFld<T>(uiObject* p, const DataInpSpec& spec,
				    const char* nm) 
    : uiTextInpFld( spec )
    , intvalGrp( *new uiGroup(p,nm) ) 
    , start( *new uiLineEdit(&intvalGrp,0,nm) )
    , stop( *new uiLineEdit(&intvalGrp,0,nm) )
    , step( 0 )
{
    const NumInpIntervalSpec<T>* spc = 
			dynamic_cast< const NumInpIntervalSpec<T>* >(&spec);
    if (!spc) { pErrMsg("huh"); return; }

    if ( spc->hasLimits() ) 
    { 
	// TODO: implement check for limits
    }

    BufferString tmp;
    spec.getText( tmp, 0 ); start.setText( tmp );
    spec.getText( tmp, 1 ); stop.setText( tmp );
    if ( spc-> hasStep() )
    {
	step = new uiLineEdit(&intvalGrp,0,nm);
	spec.getText( tmp, 2 ); step->setText( tmp );
    }

    intvalGrp.setHAlignObj( &start );

    stop.attach( rightTo, &start );
    if ( step ) step->attach( rightTo, &stop );

    init();
}


class uiStrLstInpFld : public uiIntInpField
{
public:
			uiStrLstInpFld( uiObject* p, 
					 const DataInpSpec& spec,
					 const char* nm="Line Edit Field" ) 
			    : uiIntInpField( spec )
			    , cbb( *new uiComboBox(p,nm) ) 
			{
			    const StringListInpSpec* spc = 
				dynamic_cast<const StringListInpSpec*>(&spec);
			    if ( !spc ) { pErrMsg("Huh") ; return; }

			    cbb.addItems( spc->strings() );
			    init();
			}

    virtual bool	isUndef(int) const		{ return false; }

    virtual const char*	text(int idx) const		{ return cbb.getText();}
    virtual void        setText( const char* t,int idx)	
			    { cbb.setCurrentItem(t); }

    virtual void	setIntValue( int i, int idx )	
			    { cbb.setCurrentItem(i); }
    virtual int		getIntValue( int idx )	const
			    { return cbb.currentItem(); }

    virtual uiObject&	uiObj()				{ return cbb; }

    virtual void	changeNotify( const CallBack& cb ) 
			    { cbb.selChanged.notify(cb); }

protected:
    uiComboBox&		cbb;
};

/*!

creates a new InpFld and attaches it rightTo the last one
already present in 'flds'.

*/
uiDataInpFld& uiGenInput::createInpFld( const DataInpSpec& desc )
{
    uiDataInpFld* fld;

    switch( desc.type() )
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
	case DataInpSpec::stringListTp:
	    {
		fld = new uiStrLstInpFld( this, desc ); 
	    }
	    break;
	default:
	    {
		fld = new uiLineInpFld( this, desc ); 
	    }
	    break;
    }

    uiObject* other= flds.size() ? &flds[ flds.size()-1 ]->uiObj() : 0;
    if ( other ) fld->uiObj().attach( rightTo, other );

    flds += fld;

    for( int idx=0; idx<fld->nElems(); idx++ )
	idxes += FieldIdx( flds.size()-1, idx );

    fld->changeNotify( mCB(this,uiGenInput,inpFldChanged) );
    return *fld;
}


//-----------------------------------------------------------------------------


uiGenInput::uiGenInput( uiObject* p, const char* disptxt, const char* inputStr)
    : uiGroup( p, disptxt )
    , idxes( *new TypeSet<FieldIdx> )
    , selText("") , withchk(false) , withclr(false)
    , labl(0), cbox(0), selbut(0), clrbut(0)
    , checked( this ), changed( this )
    , checked_(false), ro(false)
{ 
    inputs += new StringInpSpec( inputStr ); 
}

uiGenInput::uiGenInput( uiObject* p, const char* disptxt
	    , const DataInpSpec& inp1 )
    : uiGroup( p, disptxt )
    , idxes( *new TypeSet<FieldIdx> )
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
    , idxes( *new TypeSet<FieldIdx> )
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
    , idxes( *new TypeSet<FieldIdx> )
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


const DataInpSpec* uiGenInput::spec( int nr ) const
{ 
    if ( finalised ) 
	return( nr >= 0 && nr<flds.size() && flds[nr] ) ? &flds[nr]->spec(): 0;
    return ( nr<inputs.size() && inputs[nr] ) ? inputs[nr] : 0;
}

bool uiGenInput::newSpec(DataInpSpec* nw, int nr)
{
    return ( nr >= 0 && nr<flds.size() && flds[nr] ) 
	    ? flds[nr]->update(nw) : false; 
}



void uiGenInput::finalise_()
{
    uiGroup::finalise_();
    if ( !inputs.size() )	{ pErrMsg("No inputs specified :("); return; }

    uiObject * lastElem = &createInpFld( *inputs[0] ).uiObj();
    setHAlignObj( lastElem );

    if ( withchk )
    {
	cbox = new uiCheckBox( this, name() );
	cbox->attach( leftTo, lastElem );
	cbox->notify( mCB(this,uiGenInput,checkBoxSel) );
	setChecked( checked_ );
    }
    else if ( *name() ) 
    {
	labl = new uiLabel( this, name() );
	labl->attach( leftTo, lastElem );
    }

    for( int i=1; i<inputs.size(); i++ )
	lastElem = &createInpFld( *inputs[i] ).uiObj();

    if ( selText != "" )
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
    deepErase( inputs ); // have been copied to fields.
}



void uiGenInput::setReadOnly( bool yn, int nr )
{
    mCheckFinalised();
    if ( nr >= 0  ) 
	{ if ( nr<flds.size() && flds[nr] ) flds[nr]->setReadOnly(yn); return; }

    ro = yn;

    for( int idx=0; idx < flds.size(); idx++ )
	flds[idx]->setReadOnly( yn );
}


void uiGenInput::clear( int nr )
{
    mCheckFinalised();
    if ( nr >= 0 )
	{ if ( nr<flds.size() && flds[nr] ) flds[nr]->clear(); return; }

    for( int idx=0; idx < flds.size(); idx++ )
	flds[idx]->clear();
}

#define mFromLE_o(fn,var,undefval) \
    var uiGenInput::fn( int nr ) const \
    { \
	mCheckFinalisedv( undefval );\
	return nr<idxes.size() && flds[idxes[nr].fldidx] \
		? flds[idxes[nr].fldidx]->fn(idxes[nr].subidx) : undefval; \
    }

#define mFromLE_g(fn,var ) \
    var uiGenInput::fn( int nr, var undefVal ) const \
    { \
	mCheckFinalisedv( undefVal );\
	return nr<idxes.size() && flds[idxes[nr].fldidx] \
		? flds[idxes[nr].fldidx]->fn(idxes[nr].subidx) : undefVal; \
    }

#define mFromLE_s(fn,typ,var) \
    void uiGenInput::fn( typ var, int nr ) \
    { \
	mCheckFinalised();\
	if ( nr<idxes.size() && flds[idxes[nr].fldidx] )\
	    flds[idxes[nr].fldidx]->fn( var, idxes[nr].subidx ); \
    }

mFromLE_o(isValid,bool,false )
mFromLE_o(isUndef,bool,true )

mFromLE_g(text,const char* )
mFromLE_g(getIntValue,int )
mFromLE_g(getValue,double )
mFromLE_g(getfValue,float )
mFromLE_g(getBoolValue,bool )
mFromLE_s(setText,const char*,s)
mFromLE_s(setValue,int,i)
mFromLE_s(setValue,float,f)
mFromLE_s(setValue,double,d)
mFromLE_s(setValue,bool,yn)


const char* uiGenInput::titleText()
{ 
    if ( labl ) return labl->text(); 
    if ( cbox ) return cbox->text(); 
    return 0;
}


void uiGenInput::setTitleText( const char* txt )
{ 
    if ( labl ) labl->setText( txt );
    if ( cbox ) cbox->setText( txt ); 
}


void uiGenInput::setChecked( bool yn )
    { checked_ = yn; if ( cbox ) cbox->setChecked( yn ); }


bool uiGenInput::isChecked()
    { return checked_; }


void uiGenInput::checkBoxSel( CallBacker* cb )
{
    checked_ = cbox->isChecked();

    for( int idx=0; idx < flds.size(); idx++ )
	flds[idx]->uiObj().setSensitive( isChecked() );

    if ( selbut ) selbut->setSensitive( isChecked() );
    if ( clrbut ) clrbut->setSensitive( isChecked() );
}

void uiGenInput::inpFldChanged( CallBacker* cb )
{
    changed.trigger();
}


void uiGenInput::doSelect_( CallBacker* cb )
    { doSelect( cb ); }


void uiGenInput::doClear( CallBacker* )
    { clear(); }

