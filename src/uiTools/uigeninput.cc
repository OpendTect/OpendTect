/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          25/05/2000
 RCS:           $Id: uigeninput.cc,v 1.22 2001-07-12 15:24:06 nanne Exp $
________________________________________________________________________

-*/

#include "uigeninput.h"
#include "uilineedit.h"
#include "uilabel.h"
#include "uibutton.h"
#include "uicombobox.h"
#include "datainpspec.h"
#include "survinfo.h"


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
                        uiDataInpFld( uiGenInput* p, const DataInpSpec& spec ) 
			: spec_( *spec.clone() ), p_( p ) {}

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

    virtual void	setSensitive(bool yn)		
			    { uiObj().setSensitive(yn); }

                        // can be a uiGroup, i.e. for radio button group
    virtual uiObject&	uiObj() =0;


    DataInpSpec&	spec() { return spec_; }

    bool                update( const DataInpSpec* nw )
                        {
                            if (spec_.type() != nw->type()) return false;
                            return update_(nw);
                        }
protected:

    virtual bool        update_( const DataInpSpec* )
			    { return false; } // TODO: implement

    void		changeNotify()	{ p_->changed.trigger( *p_ ); }
    void		checkNotify()	{ p_->checked.trigger( *p_ ); }

    DataInpSpec&	spec_;
    uiGenInput*		p_;

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
                        uiTextInpFld( uiGenInput* p, const DataInpSpec& spec )
			: uiDataInpFld( p, spec ) {}

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
                        uiIntInpField( uiGenInput* p, const DataInpSpec& spec, 
				       int clearValue=0)
                        : uiDataInpFld( p, spec )
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
			uiLineInpFld( uiGenInput* p, 
					 const DataInpSpec& spec,
					 const char* nm="Line Edit Field" ) 
			    : uiTextInpFld( p, spec )
			    , li( *new uiLineEdit(p,0,nm) ) 
			{
			    li.setText( spec.text() );
			    init();
			    li.textChanged.notify( 
				mCB(this,uiDataInpFld,changeNotify) );
			}

    virtual const char*	text(int idx) const		{ return li.text(); }
    virtual void        setText( const char* t,int idx)	{ li.setText(t);}
    virtual uiObject&	uiObj()				{ return li; }

    virtual void        setReadOnly( bool yn )	{ li.setReadOnly(yn); }
    virtual bool        isReadOnly() const	{ return li.isReadOnly(); }


protected:
    uiLineEdit&	li;
};


class uiBoolInpFld : public uiIntInpField
{
public:

			uiBoolInpFld( uiGenInput* p, 
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

protected:

    void		selected(CallBacker*);

    BufferString        truetxt;
    BufferString        falsetxt;
    bool                yn;
    uiObject*		butOrGrp; //! either uiButton or uiButtonGroup
    uiCheckBox*		cb;
    uiRadioButton*	rb1;
    uiRadioButton*	rb2;
};

uiBoolInpFld::uiBoolInpFld(uiGenInput* p, const DataInpSpec& spec, const char* nm)
    :uiIntInpField( p, spec )
    , butOrGrp( 0 ) , cb( 0 ), rb1( 0 ), rb2( 0 ), yn( true )
{
    mDynamicCastGet(const BoolInpSpec*,spc,&spec)
    if ( !spc ) { pErrMsg("huh?");butOrGrp = new uiGroup(p,nm); return; }

    yn=spc->checked(); initClear();

    truetxt = spc->trueFalseTxt(true);
    falsetxt = spc->trueFalseTxt(false);

    if ( truetxt == ""  || falsetxt == "" )
    { 
	cb = new uiCheckBox( p, (truetxt == "") ? nm : (const char*)truetxt ); 
	cb->notify( mCB(this,uiBoolInpFld,selected) );
	butOrGrp = cb;
	setValue( yn );
	return; 
    }

    // we have two labelTxt()'s, so we'll make radio buttons
    uiGroup* grp_ = new uiGroup( p, nm ); 
    butOrGrp = grp_;

    rb1 = new uiRadioButton( butOrGrp, truetxt );
    rb1->notify( mCB(this,uiBoolInpFld,selected) );
    rb2 = new uiRadioButton( butOrGrp, falsetxt );
    rb2->notify( mCB(this,uiBoolInpFld,selected) );

    rb2->attach( rightTo, rb1 );
    grp_->setHAlignObj( rb1 );

    setValue( yn );

    init();
}


void uiBoolInpFld::selected(CallBacker* cber)
{
    bool yn_ = yn;
    if ( cber == rb1 )		{ yn = rb1->isChecked(); }
    else if ( cber == rb2 )	{ yn = !rb2->isChecked(); }
    else if( cber == cb)	{ yn = cb->isChecked(); }
    else return;

    if ( yn != yn_ )		changeNotify();
    setValue( yn );
}


void uiBoolInpFld::setValue( bool b, int idx )
{
    yn = b; 

    if ( cb ) { cb->setChecked( yn ); return; }

    if ( !rb1 || !rb2 ) { pErrMsg("Huh?"); return; }

    rb1->setChecked(yn); 
    rb2->setChecked(!yn); 
}


class uiBinIDInpFld : public uiTextInpFld
{
public:

			uiBinIDInpFld( uiGenInput* p, 
					 const DataInpSpec& spec,
					 const char* nm="BinID Input Field" );

    virtual uiObject&	uiObj()			{ return binidGrp; }

    virtual int		nElems()		{ return 2; }
    virtual uiObject&	element( int idx  )	{ return idx ? crl_y : inl_x; }

    virtual const char*	text(int idx) const		
			    { return idx ? crl_y.text() : inl_x.text(); }
    virtual void        setText( const char* t,int idx)
			    { if (idx) crl_y.setText(t); else inl_x.setText(t);}

protected:
    // don't change order of these 3 attributes!
    uiGroup&		binidGrp;
    uiLineEdit&		inl_x; // inline or x-coordinate
    uiLineEdit&		crl_y; // crossline or y-coordinate
    const BinID2Coord*	b2c;

    uiPushButton*	ofrmBut; // other format: BinId / Coordinates 
    void		otherFormSel(CallBacker*);

};

uiBinIDInpFld::uiBinIDInpFld( uiGenInput* p, const DataInpSpec& spec,
			      const char* nm ) 
    : uiTextInpFld( p, spec )
    , binidGrp( *new uiGroup(p,nm) )
    , inl_x( *new uiLineEdit(&binidGrp,0,nm) )
    , crl_y( *new uiLineEdit(&binidGrp,0,nm) )
    , ofrmBut( 0 )
    , b2c(0)
{
    mDynamicCastGet(const BinIDCoordInpSpec*,spc,&spec)
    if ( !spc ){ pErrMsg("huh"); return; }

    inl_x.setText( spec.text(0) );
    crl_y.setText( spec.text(1) );

    binidGrp.setHAlignObj( &inl_x );
    crl_y.attach( rightTo, &inl_x );

    inl_x.textChanged.notify( mCB(this,uiDataInpFld,changeNotify) );
    crl_y.textChanged.notify( mCB(this,uiDataInpFld,changeNotify) );

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

			uiIntervalInpFld( uiGenInput* p, 
					 const DataInpSpec& spec,
					 const char* nm="Bool Input Field" );

    virtual int		nElems()		{ return step ? 3 : 2; }
    virtual uiObject&	element( int idx  )	{ return *le(idx); }

    virtual uiObject&	uiObj()			{ return intvalGrp; }

    virtual const char*	text(int idx) const		
			    { return  le(idx) ? le(idx)->text() : 0; }
    virtual void        setText( const char* t,int idx)	
			    { if (le(idx)) le(idx)->setText(t); }

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
uiIntervalInpFld<T>::uiIntervalInpFld<T>(uiGenInput* p, const DataInpSpec& spec,
				    const char* nm) 
    : uiTextInpFld( p, spec )
    , intvalGrp( *new uiGroup(p,nm) ) 
    , start( *new uiLineEdit(&intvalGrp,0,nm) )
    , stop( *new uiLineEdit(&intvalGrp,0,nm) )
    , step( 0 )
{
    mDynamicCastGet(const NumInpIntervalSpec<T>*,spc,&spec)
    if (!spc) { pErrMsg("huh"); return; }

    if ( spc->hasLimits() ) 
    { 
	// TODO: implement check for limits
    }

    start.textChanged.notify( mCB(this,uiDataInpFld,changeNotify) );
    stop.textChanged.notify( mCB(this,uiDataInpFld,changeNotify) );

    start.setText( spec.text(0) );
    stop.setText( spec.text(1) );
    if ( spc-> hasStep() )
    {
	step = new uiLineEdit(&intvalGrp,0,nm);
	step->textChanged.notify( mCB(this,uiDataInpFld,changeNotify) );
	step->setText( spec.text(2) );
    }

    intvalGrp.setHAlignObj( &start );

    stop.attach( rightTo, &start );
    if ( step ) step->attach( rightTo, &stop );

    init();
}


class uiStrLstInpFld : public uiIntInpField
{
public:
			uiStrLstInpFld( uiGenInput* p, 
					 const DataInpSpec& spec,
					 const char* nm="Line Edit Field" ) 
			    : uiIntInpField( p, spec )
			    , cbb( *new uiComboBox(p,nm) ) 
			{
			    mDynamicCastGet(const StringListInpSpec*,spc,&spec)
			    if ( !spc ) { pErrMsg("Huh") ; return; }

			    cbb.addItems( spc->strings() );
			    init();

			    cbb.selectionchanged.notify( 
				mCB(this,uiDataInpFld,changeNotify) );
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
    if( !finalised ) { ro = yn; return; }

    if ( nr >= 0  ) 
	{ if ( nr<flds.size() && flds[nr] ) flds[nr]->setReadOnly(yn); return; }

    ro = yn;

    for( int idx=0; idx < flds.size(); idx++ )
	flds[idx]->setReadOnly( yn );
}

void uiGenInput::setFldsSensible( bool yn, int nr )
{
    if( !finalised ) { ro = yn; return; }

    if ( nr >= 0  )
        { if ( nr<flds.size() && flds[nr] ) flds[nr]->setSensitive(yn); return;}

    ro = yn;

    for( int idx=0; idx < flds.size(); idx++ )
        flds[idx]->setSensitive( yn );
}


void uiGenInput::clear( int nr )
{
    if ( !finalised ){ pErrMsg("Nothing to clear. Not finalised yet.");return; }

    if ( nr >= 0 )
	{ if ( nr<flds.size() && flds[nr] ) flds[nr]->clear(); return; }

    for( int idx=0; idx < flds.size(); idx++ )
	flds[idx]->clear();
}

#define mFromLE_o(fn,var,undefval) \
    var uiGenInput::fn( int nr ) const \
    { \
	if ( !finalised ) return undefval; \
	return nr<idxes.size() && flds[idxes[nr].fldidx] \
		? flds[idxes[nr].fldidx]->fn(idxes[nr].subidx) : undefval; \
    }

#define mFromLE_g(fn,var ) \
    var uiGenInput::fn( int nr, var undefVal ) const \
    { \
	if ( !finalised )\
	{\
	    int inpidx=0; int elemidx=nr;\
	    while(  elemidx>=0 && inpidx<inputs.size() && inputs[inpidx]\
		    && elemidx>=inputs[inpidx]->nElems() )\
	    {\
		elemidx -= inputs[inpidx]->nElems();\
		inpidx++;\
	    }\
	    return inpidx<inputs.size() && inputs[inpidx] \
		    ? inputs[inpidx]->fn(elemidx) : undefVal; \
	}\
	\
	return nr<idxes.size() && flds[idxes[nr].fldidx] \
		? flds[idxes[nr].fldidx]->fn(idxes[nr].subidx) : undefVal; \
    }

#define mFromLE_s(fn,typ,var) \
    void uiGenInput::fn( typ var, int nr ) \
    { \
	if ( !finalised )\
	{\
	    int inpidx=0; int elemidx=nr;\
	    while(  elemidx>=0 && inpidx<inputs.size() && inputs[inpidx]\
		    && elemidx>=inputs[inpidx]->nElems() )\
	    {\
		elemidx -= inputs[inpidx]->nElems();\
		inpidx++;\
	    }\
	    if ( inpidx<inputs.size() && inputs[inpidx] )\
		inputs[inpidx]->setValue( var, elemidx );\
	    return;\
	}\
	\
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

void uiGenInput::doSelect_( CallBacker* cb )
    { doSelect( cb ); }


void uiGenInput::doClear( CallBacker* )
    { clear(); }

