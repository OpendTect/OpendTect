/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          25/05/2000
 RCS:           $Id: uigeninput.cc,v 1.50 2003-04-22 14:06:45 arend Exp $
________________________________________________________________________

-*/

#include "basictypes.h"
#include "uigeninput.h"
#include "uilineedit.h"
#include "uiboolinp.h"
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

class uiInputFld : public CallBacker
{
public:
                        uiInputFld( uiGenInput* p, const DataInpSpec& spec ) 
			: spec_( *spec.clone() ), p_( p ) {}

    virtual		~uiInputFld()			{ delete &spec_; }

    virtual int		nElems() const			{ return 1; }

    virtual UserInputObj* element( int idx=0 )		= 0;

    const UserInputObj*	  element( int idx=0 ) const
			   {return const_cast<uiInputFld*>(this)->element(idx);}

    virtual uiObject*	mainObj()			= 0;

                        // can be a uiGroup, i.e. for radio button group
    virtual uiObject*	elemObj( int idx=0 )
			{
			    UserInputObj* elem = element(idx);
			    if ( !elem )	return 0;

			    uiObject* ob = dynamic_cast<uiObject*>(elem);
			    if ( ob )	return ob;

			    uiGroup* grp = dynamic_cast<uiGroup*>(elem);
			    if ( grp )	return grp->mainObject(); 

			    return 0;
			}

    virtual bool	isValid(int idx)
			{ 
			    if ( isUndef(idx) ) return false;
			    if ( !spec_.hasLimits() ) return true;
			    pErrMsg("Sorry, not implemented..");
			    return true;
			} // TODO implement
    virtual bool	isUndef(int idx) const
			{ return !*text(idx); } 

    const char*		text( int idx ) const
			{ 
			    return element(idx) ? element(idx)->text() 
						: undefVal<const char*>();
			}
    int			getIntValue( int idx )	const
			    { 
				return element(idx) 
						? element(idx)->getIntValue() 
						: undefVal<int>();
			    }
    double		getValue( int idx )
			    { 
				return element(idx) ? element(idx)->getValue() 
						    : undefVal<double>();
			    }
    float		getfValue( int idx )	const
			    { 
				return element(idx) ? element(idx)->getfValue() 
						    : undefVal<float>();
			    }
    bool		getBoolValue( int idx )	const
			    { 
				return element(idx) 
						? element(idx)->getBoolValue() 
						: undefVal<bool>();
			    }

    virtual void	setText( const char* s, int idx )
			    { if ( element(idx) ) element(idx)->setText(s); }
    void		setValue( int i, int idx )
			    { if ( element(idx) ) element(idx)->setValue(i); }
    void		setValue( double d, int idx )
			    { if ( element(idx) ) element(idx)->setValue(d); }
    void		setValue( float f, int idx )
			    { if ( element(idx) ) element(idx)->setValue(f); }
    void		setValue( bool b, int idx )
			    { if ( element(idx) ) element(idx)->setValue(b); }

			//! stores current value as clear state.
    void		initClearValue()
			    { 
				for(int idx=0;idx<nElems()&&element(idx);idx++)
				    element(idx)->initClearValue();
			    }
    void		clear()
			    { 
				for(int idx=0;idx<nElems()&&element(idx);idx++)
				    element(idx)->clear();
			    }

    virtual void	setReadOnly( bool yn = true, int idx=0 )
			    { if (element(idx)) element(idx)->setReadOnly(yn);}

    bool		isReadOnly( int idx=0 ) const
			    { 
				return element(idx) ? element(idx)->isReadOnly()
						    : undefVal<bool>();
			    }

    void		setSensitive(bool yn, int idx=-1)		
			    { 
				if ( idx >= 0 ) 
				{ 
				    if (elemObj(idx) )
					elemObj(idx)->setSensitive(yn); 
				}
				else
				    for(int ix=0;ix<nElems()&&elemObj(ix);ix++)
					elemObj(ix)->setSensitive(yn); 
			    }

    DataInpSpec&	spec()				{ return spec_; }

    bool                update( const DataInpSpec& nw )
			    {
				if ( spec_.type() != nw.type() ) return false;

				if ( update_(nw) )
				{
				    spec_ = nw;
				    return true;
				}

				return false;
			    }

    void		valChangingNotify(CallBacker*)
			    { p_->valuechanging.trigger( *p_ ); }

    void		valChangedNotify(CallBacker*)
			    { p_->valuechanged.trigger( *p_ ); }

protected:

    virtual bool        update_( const DataInpSpec& nw )
			    { 
				return nElems() == 1 && element()
						    ? element()->update(nw)
						    : false; 
			    } 

    DataInpSpec&	spec_;
    uiGenInput*		p_;

    void		init()
			    {
				if ( !update_( spec_ ) )
				{
				    pErrMsg("huh?");
				    update_( spec_ );
				}

				uiObject::SzPolicy hpol =
					 p_ ? p_->elemSzPol() : uiObject::undef;

				if ( hpol == uiObject::undef )
				{
				    int nel = p_ ? p_->nElements() : nElems();

				    switch( spec_.type().rep() )
				    {
				    case DataType::stringTp:
					hpol = nel > 1 ? uiObject::smallvar
						       : uiObject::medvar;
				    break;
				    default:
					hpol = nel > 1 ? uiObject::small
						       : uiObject::medium;
				    break;
				    }
				}

				for( int idx=0; idx<nElems() && elemObj(idx)
								      ; idx++)
				{
				    elemObj(idx)->setHSzPol( hpol );
				}
			    }
};

template <class T>
class uiSimpleInputFld : public uiInputFld
{
public:
			uiSimpleInputFld( uiGenInput* p, 
					 const DataInpSpec& spec,
					 const char* nm="Line Edit Field" ) 
			    : uiInputFld( p, spec )
			    , usrinpobj( *new T(p, spec) ) 
			    {
				init();

				setReadOnly( false );

				usrinpobj.notifyValueChanging( 
				   mCB(this,uiInputFld,valChangingNotify) );

				usrinpobj.notifyValueChanged( 
				   mCB(this,uiInputFld,valChangedNotify) );
			    }

    virtual		~uiSimpleInputFld()	{ delete &usrinpobj; }

    virtual UserInputObj* element( int idx=0 )
			    { return idx==0 ? &usrinpobj : 0; }

    virtual uiObject*	mainObj()
			    { return dynamic_cast<uiObject*>(&usrinpobj); }

protected:

    T&			usrinpobj;
};

typedef uiSimpleInputFld<uiLineEdit>	uiTextInputFld;


class uiFileInputFld : public uiSimpleInputFld<uiLineEdit>
{
public:
			uiFileInputFld( uiGenInput* p, 
					 const DataInpSpec& spec,
					 const char* nm="File Input Field" ) 
			    : uiSimpleInputFld<uiLineEdit>( p, spec, nm )
			    { setText( spec.text() ? spec.text() : "", 0 ); }

    virtual void	setText( const char* s, int idx )
			    { 
				if ( idx ) return;
				usrinpobj.setText(s);
				usrinpobj.end();
			    }
};

class uiBoolInpFld : public uiSimpleInputFld<uiBoolInput>
{
public:
			uiBoolInpFld( uiGenInput* p, 
					 const DataInpSpec& spec,
					 const char* nm="Bool Input Field" ) 
			    : uiSimpleInputFld<uiBoolInput>( p, spec, nm )
			    {}

    virtual uiObject*	mainObj()	{ return usrinpobj.mainObject(); }
};


class uiBinIDInpFld : public uiInputFld
{
public:

			uiBinIDInpFld( uiGenInput* p, 
					 const DataInpSpec& spec,
					 const char* nm="BinID Input Field" );

    virtual uiObject*	mainObj()	{ return binidGrp.mainObject(); }

    virtual int		nElems() const		{ return 2; }
    virtual UserInputObj* element( int idx )	{ return idx ? &crl_y : &inl_x;}

    bool		notifyValueChanged(const CallBack& cb )
                            { valueChanged.notify(cb); return true; }

    Notifier<uiBinIDInpFld> valueChanged;

protected:

    // don't change order of these 3 attributes!
    uiGroup&		binidGrp;
    uiLineEdit&		inl_x; // inline or x-coordinate
    uiLineEdit&		crl_y; // crossline or y-coordinate
    const BinID2Coord*	b2c;

    uiPushButton*	ofrmBut; // other format: BinId / Coordinates 
    void		otherFormSel(CallBacker*);


    virtual const char*	getvalue_(int idx) const		
			    { return idx ? crl_y.text() : inl_x.text(); }
    virtual void        setvalue_( const char* t,int idx)
			    { if (idx) crl_y.setText(t); else inl_x.setText(t);}

    virtual bool	update_( const DataInpSpec& spec );

};

uiBinIDInpFld::uiBinIDInpFld( uiGenInput* p, const DataInpSpec& spec,
			      const char* nm ) 
    : uiInputFld( p, spec )
    , binidGrp( *new uiGroup(p,nm) )
    , inl_x( *new uiLineEdit(&binidGrp,0,nm) )
    , crl_y( *new uiLineEdit(&binidGrp,0,nm) )
    , ofrmBut( 0 )
    , b2c(0)
    , valueChanged(this)
{
    mDynamicCastGet(const BinIDCoordInpSpec*,spc,&spec)
    if ( !spc ){ pErrMsg("huh"); return; }

    binidGrp.setHAlignObj( &inl_x );
    crl_y.attach( rightTo, &inl_x );

    inl_x.notifyValueChanging( mCB(this,uiInputFld,valChangingNotify) );
    crl_y.notifyValueChanging( mCB(this,uiInputFld,valChangingNotify) );

    inl_x.notifyValueChanged( mCB(this,uiInputFld,valChangedNotify) );
    crl_y.notifyValueChanged( mCB(this,uiInputFld,valChangedNotify) );

    if ( spc->otherTxt() )
    {
	ofrmBut = new uiPushButton( &binidGrp, spc->otherTxt() );
	ofrmBut->activated.notify( mCB(this,uiBinIDInpFld,otherFormSel) );

	ofrmBut->attach( rightTo, &crl_y );
    }

    b2c = spc->binID2Coord();

    init();

    inl_x.setReadOnly(false);
    crl_y.setReadOnly(false);
}

bool uiBinIDInpFld::update_( const DataInpSpec& spec )
{
    mDynamicCastGet(const BinIDCoordInpSpec*,spc,&spec)
    if ( !spc ){ pErrMsg("huh"); return false; }

    inl_x.setText( spec.text(0) );
    crl_y.setText( spec.text(1) );

    return true;
}


void uiBinIDInpFld::otherFormSel(CallBacker* cb)
{
// TODO  implement:
// pop dialog box
// transform using b2c
// set value
}

template<class T>
class uiIntervalInpFld : public uiInputFld
{
public:

			uiIntervalInpFld( uiGenInput* p, 
					 const DataInpSpec& spec,
					 const char* nm="Interval Input Field");

    virtual int		nElems() const		{ return step ? 3 : 2; }
    virtual UserInputObj* element( int idx=0 )	{ return le(idx); }

    virtual uiObject*	mainObj()	{ return intvalGrp.mainObject(); }

protected:
    uiGroup&		intvalGrp;

    uiLineEdit&		start;
    uiLineEdit&		stop;
    uiLineEdit*		step;
    uiLabel*		lbl;

    virtual T		getvalue_(int idx) const		
			    { 
				return le(idx) ? 
					  convertTo<T>(le(idx)->text()) 
					: undefVal<T>(); 
			    }

    virtual void        setvalue_( T t, int idx)	
			    { 
				if ( le(idx) ) 
				    le(idx)->setText(convertTo<const char*>(t));
			    }


    inline const uiLineEdit* le( int idx ) const 
			    { 
				return const_cast<uiIntervalInpFld*>(this)->
									le(idx);
			    }

    uiLineEdit*		le( int idx ) 
			    { 
				if ( idx>1 ) return step;
				return idx ? &stop : &start;
			    }

    virtual bool        update_( const DataInpSpec& nw );
};

template<class T>
uiIntervalInpFld<T>::uiIntervalInpFld(uiGenInput* p, const DataInpSpec& spec,
				    const char* nm) 
    : uiInputFld( p, spec )
    , intvalGrp( *new uiGroup(p,nm) ) 
    , start( *new uiLineEdit(&intvalGrp,0,nm) )
    , stop( *new uiLineEdit(&intvalGrp,0,nm) )
    , step( 0 )
{
    mDynamicCastGet(const NumInpIntervalSpec<T>*,spc,&spec)
    if (!spc) { pErrMsg("Huh"); return; }

    if ( spc->hasLimits() ) 
    { 
	// TODO: implement check for limits
    }

    start.notifyValueChanging( mCB(this,uiInputFld,valChangingNotify) );
    stop.notifyValueChanging( mCB(this,uiInputFld,valChangingNotify) );

    start.notifyValueChanged( mCB(this,uiInputFld,valChangedNotify) );
    stop.notifyValueChanged( mCB(this,uiInputFld,valChangedNotify) );

    start.setReadOnly( false );
    stop.setReadOnly( false );

    if ( spc->hasStep() )
    {
	step = new uiLineEdit(&intvalGrp,"",nm);

	step->notifyValueChanging( mCB(this,uiInputFld,valChangingNotify) );
	step->notifyValueChanged( mCB(this,uiInputFld,valChangedNotify) );
	step->setReadOnly( false );

	lbl = new uiLabel(&intvalGrp, "Step" );
    }

    intvalGrp.setHAlignObj( &start );

    stop.attach( rightTo, &start );
    if ( step ) 
    {
	lbl->attach( rightTo, &stop );
        step->attach( rightTo, lbl );
    }

    init();
}

template<class T>
bool uiIntervalInpFld<T>::update_( const DataInpSpec& spec )
{
    start.setText( spec.text(0) );
    stop.setText( spec.text(1) );
    if ( step  )
	step->setText( spec.text(2) );

    return true;
}



class uiStrLstInpFld : public uiInputFld
{
public:
			uiStrLstInpFld( uiGenInput* p, 
					 const DataInpSpec& spec,
					 const char* nm="uiStrLstInpFld" ) 
			    : uiInputFld( p, spec )
			    , cbb( *new uiComboBox(p,nm) ) 
			{
			    init();

			    cbb.setReadOnly( true );

			    cbb.selectionChanged.notify( 
				mCB(this,uiInputFld,valChangedNotify) );
			}

    virtual bool	isUndef(int) const		{ return false; }

    virtual const char*	text(int idx) const		{ return cbb.text();}
    virtual void        setText( const char* t,int idx)	
			    { cbb.setCurrentItem(t); }

    virtual void	setReadOnly( bool yn = true, int idx=0 )
			    { 
			      if ( !yn )
				pErrMsg("Stringlist input must be read-only");
			    }

    virtual UserInputObj* element( int idx=0 )		{ return &cbb; }
    virtual uiObject*	mainObj()			{ return &cbb; }

protected:

    virtual void	setvalue_( int i, int idx )
			    { cbb.setCurrentItem(i); }
    virtual int		getvalue_( int idx )	const
			    { return cbb.currentItem(); }

    uiComboBox&		cbb;
};

/*!

creates a new InpFld and attaches it rightTo the last one
already present in 'flds'.

*/
uiInputFld& uiGenInput::createInpFld( const DataInpSpec& desc )
{
    uiInputFld* fld=0;

    switch( desc.type().rep() )
    {

    case DataType::boolTp:
    {
	fld = new uiBoolInpFld( this, desc ); 
    }
    break;

    case DataType::stringTp:
    {
	if ( desc.type().form() == DataType::list )
	    fld = new uiStrLstInpFld( this, desc ); 

	else if ( desc.type().form() == DataType::filename )
	    fld = new uiFileInputFld( this, desc ); 

	else
	    fld = new uiTextInputFld( this, desc ); 
    }
    break;

    case DataType::floatTp:
    case DataType::doubleTp:
    case DataType::intTp:
    {
	if ( desc.type().form() == DataType::interval )
	{
	    switch( desc.type().rep() )
	    {

	    case DataType::intTp:
		fld = new uiIntervalInpFld<int>( this, desc ); 
	    break;
	    case DataType::floatTp:
		fld = new uiIntervalInpFld<float>( this, desc ); 
	    break;
	    case DataType::doubleTp:
		fld = new uiIntervalInpFld<double>( this, desc ); 
	    break;
	    }
	}
	else if ( desc.type().form() == DataType::binID )
	    fld = new uiBinIDInpFld( this, desc ); 
	else
	    fld = new uiTextInputFld( this, desc ); 
    }
    break;
    }

    if ( ! fld ) { pErrMsg("huh"); fld = new uiTextInputFld( this, desc ); }


    uiObject* other= flds.size() ? flds[ flds.size()-1 ]->mainObj() : 0;
    if ( other )
	fld->mainObj()->attach( rightTo, other );

    flds += fld;

    for( int idx=0; idx<fld->nElems(); idx++ )
	idxes += FieldIdx( flds.size()-1, idx );

    return *fld;
}


//-----------------------------------------------------------------------------


uiGenInput::uiGenInput( uiParent* p, const char* disptxt, const char* inputStr)
    : uiGroup( p, disptxt )
    , finalised( false )
    , idxes( *new TypeSet<FieldIdx> )
    , selText("") , withchk(false) , withclr(false)
    , labl(0), cbox(0), selbut(0), clrbut(0)
    , checked( this ), valuechanging( this ), valuechanged( this )
    , checked_(false), rdonly_(false), rdonlyset_(false)
    , elemszpol( uiObject::undef )
{ 
    inputs += new StringInpSpec( inputStr ); 
    mainObject()->finalising.notify( mCB(this,uiGenInput,doFinalise) );
}

uiGenInput::uiGenInput( uiParent* p, const char* disptxt
	    , const DataInpSpec& inp1 )
    : uiGroup( p, disptxt )
    , finalised( false )
    , idxes( *new TypeSet<FieldIdx> )
    , selText("") , withchk(false) , withclr(false)
    , labl(0), cbox(0), selbut(0), clrbut(0)
    , checked( this ), valuechanging( this ), valuechanged( this )
    , checked_(false), rdonly_(false), rdonlyset_(false)
    , elemszpol( uiObject::undef )
{
    inputs += inp1.clone();
    mainObject()->finalising.notify( mCB(this,uiGenInput,doFinalise) );
}


uiGenInput::uiGenInput( uiParent* p, const char* disptxt
	    , const DataInpSpec& inp1 , const DataInpSpec& inp2 )
    : uiGroup( p, disptxt )
    , finalised( false )
    , idxes( *new TypeSet<FieldIdx> )
    , selText("") , withchk(false) , withclr(false)
    , labl(0), cbox(0), selbut(0), clrbut(0)
    , checked( this ), valuechanging( this ), valuechanged( this )
    , checked_(false), rdonly_(false), rdonlyset_(false)
    , elemszpol( uiObject::undef )
{
    inputs += inp1.clone();
    inputs += inp2.clone();
    mainObject()->finalising.notify( mCB(this,uiGenInput,doFinalise) );
}


uiGenInput::uiGenInput( uiParent* p, const char* disptxt
	    , const DataInpSpec& inp1, const DataInpSpec& inp2 
	    , const DataInpSpec& inp3 )
    : uiGroup( p, disptxt )
    , finalised( false )
    , idxes( *new TypeSet<FieldIdx> )
    , selText("") , withchk(false) , withclr(false)
    , labl(0), cbox(0), selbut(0), clrbut(0)
    , checked( this ), valuechanging( this ), valuechanged( this )
    , checked_(false), rdonly_(false), rdonlyset_(false)
    , elemszpol( uiObject::undef )
{
    inputs += inp1.clone();
    inputs += inp2.clone();
    inputs += inp3.clone();
    mainObject()->finalising.notify( mCB(this,uiGenInput,doFinalise) );
}

uiGenInput::~uiGenInput()
{
    deepErase( flds );
    deepErase( inputs ); // doesn't hurt
    delete &idxes;
}


void uiGenInput::addInput( const DataInpSpec& inp )
{
    inputs += inp.clone();
    mainObject()->finalising.notify( mCB(this,uiGenInput,doFinalise) );
}


const DataInpSpec* uiGenInput::spec( int nr ) const
{ 
    if ( finalised ) 
	return( nr >= 0 && nr<flds.size() && flds[nr] ) ? &flds[nr]->spec(): 0;
    return ( nr<inputs.size() && inputs[nr] ) ? inputs[nr] : 0;
}

bool uiGenInput::newSpec(const DataInpSpec& nw, int nr)
{
    return ( nr >= 0 && nr<flds.size() && flds[nr] ) 
	    ? flds[nr]->update(nw) : false; 
}



void uiGenInput::doFinalise()
{
    if ( finalised )		return;
    if ( !inputs.size() )	{ pErrMsg("No inputs specified :("); return; }

    uiObject* lastElem = createInpFld( *inputs[0] ).mainObj();
    setHAlignObj( lastElem );

    if ( withchk )
    {
	cbox = new uiCheckBox( this, name() );
	cbox->attach( leftTo, lastElem );
	cbox->activated.notify( mCB(this,uiGenInput,checkBoxSel) );
	setChecked( checked_ );
    }
    else if ( *name() ) 
    {
	labl = new uiLabel( this, name() );
	labl->attach( leftTo, lastElem );
    }

    for( int i=1; i<inputs.size(); i++ )
	lastElem = createInpFld( *inputs[i] ).mainObj();

    if ( selText != "" )
    {
	selbut = new uiPushButton( this, selText );
	selbut->activated.notify( mCB(this,uiGenInput,doSelect_) );
	selbut->attach( rightOf, lastElem );
    }

    if ( withclr )
    {
	clrbut = new uiPushButton( this, "Clear ..." );
	clrbut->attach( rightOf, selbut ? selbut : lastElem );
	clrbut->activated.notify( mCB(this,uiGenInput,doClear) );
    }

    deepErase( inputs ); // have been copied to fields.
    finalised = true;

    if ( rdonlyset_) setReadOnly( rdonly_ );

    if ( withchk ) checkBoxSel(0);	// sets elements (non-)sensitive
}



void uiGenInput::setReadOnly( bool yn, int nr )
{
    if ( !finalised ) { rdonly_ = yn; rdonlyset_=true; return; }

    if ( nr >= 0  ) 
	{ if ( nr<flds.size() && flds[nr] ) flds[nr]->setReadOnly(yn); return; }

    rdonly_ = yn; rdonlyset_=true;

    for( int idx=0; idx < flds.size(); idx++ )
	flds[idx]->setReadOnly( yn );
}

void uiGenInput::setFldsSensible( bool yn, int nr )
{
    if ( nr >= 0  )
        { if ( nr<flds.size() && flds[nr] ) flds[nr]->setSensitive(yn); return;}

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

int uiGenInput::nElements() const
{
    int nel=0;
    if ( finalised ) 
    {
	for( int idx=0; idx<flds.size(); idx++ )
	    if ( flds[idx] ) nel += flds[idx]->nElems();
    }
    else
    {
	for( int idx=0; idx<inputs.size(); idx++ )
	    if ( inputs[idx] ) nel += inputs[idx]->nElems();
    }

    return nel;
}

UserInputObj* uiGenInput::element( int nr )
{ 
    if ( !finalised ) return 0; 
    return nr<idxes.size() && flds[idxes[nr].fldidx]
	    ? flds[idxes[nr].fldidx]->element(idxes[nr].subidx) : 0; 
}

#define mFromLE_o(fn,var,undefval) \
    var uiGenInput::fn( int nr ) const \
    { \
	if ( !finalised ) return undefval; \
	return nr<idxes.size() && flds[idxes[nr].fldidx] \
		? flds[idxes[nr].fldidx]->fn(idxes[nr].subidx) : undefval; \
    }

mFromLE_o(isValid,bool,false )
mFromLE_o(isUndef,bool,true )

#define g_func	text
#define s_func	setText
#define gs_type const char*
#include "fromlegs.h"

#define g_func	getIntValue
#define s_func	setValue
#define gs_type int
#include "fromlegs.h"

#define g_func	getValue
#define s_func	setValue
#define gs_type double
#include "fromlegs.h"

#define g_func	getfValue
#define s_func	setValue
#define gs_type float
#include "fromlegs.h"

#define g_func	getBoolValue
#define s_func	setValue
#define gs_type bool
#include "fromlegs.h"


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

    setFldsSensible( isChecked() );

    if ( selbut ) selbut->setSensitive( isChecked() );
    if ( clrbut ) clrbut->setSensitive( isChecked() );
}

void uiGenInput::doSelect_( CallBacker* cb )
    { doSelect( cb ); }


void uiGenInput::doClear( CallBacker* )
    { clear(); }

