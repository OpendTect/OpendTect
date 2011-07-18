/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          25/05/2000
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uigeninput.cc,v 1.94 2011-07-18 14:05:14 cvsbert Exp $";

#include "uigeninput.h"
#include "uilineedit.h"
#include "uiboolinp.h"
#include "uilabel.h"
#include "uibutton.h"
#include "uicombobox.h"
#include "datainpspec.h"
#include "survinfo.h"
#include "undefval.h"


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
                        uiInputFld( uiGenInput* p, const DataInpSpec& dis ) 
			: spec_( *dis.clone() ), p_( p ) {}

    virtual		~uiInputFld()			{ delete &spec_; }

    virtual int		nElems() const			{ return 1; }

    virtual UserInputObj* element( int idx=0 )		= 0;

    const UserInputObj*	element( int idx=0 ) const
			{ return const_cast<uiInputFld*>(this)->element(idx); }

    virtual uiObject*	mainObj()			= 0;

                        // can be a uiGroup, i.e. for radio button group
    virtual uiObject*	elemObj( int idx=0 )
			{
			    UserInputObj* elem = element(idx);
			    if ( !elem ) return 0;
			    mDynamicCastGet(uiGroup*,grp,elem)
			    if ( grp ) return grp->mainObject();
			    mDynamicCastGet(uiObject*,ob,elem)
			    return ob;
			}

    virtual bool	isUndef( int idx ) const
			{ return mIsUdf(text(idx)); } 

    const char*		text( int idx ) const
			{ 
			    const UserInputObj* obj = element( idx );
			    const char* ret = obj ? obj->text() : 0;
			    return ret ? ret : mUdf(const char*);
			}
#define mImplGetFn(typ,fn) \
    typ			fn( int idx ) const \
			{  \
			    const UserInputObj* obj = element( idx ); \
			    return obj ? obj->fn() : mUdf(typ); \
			}
    			mImplGetFn(int,getIntValue)
    			mImplGetFn(float,getfValue)
    			mImplGetFn(double,getdValue)
    			mImplGetFn(bool,getBoolValue)

    template <class T>
    void		setValue( T t, int idx )
			{
			    UserInputObj* obj = element( idx );
			    if ( !obj ) return;
			    if ( mIsUdf(t) )
				obj->clear();
			    else
				obj->setValue(t);
			}
    virtual void	setText( const char* s, int idx )
			    { setValue( s, idx ); }
    void		setValue( bool b, int idx )
			    { if ( element(idx) ) element(idx)->setValue(b); }

#define mDoAllElems(fn) \
			for( int idx=0; idx<nElems(); idx++ ) \
			{ \
			    UserInputObj* obj = element( idx ); \
			    if ( obj ) obj->fn; \
			    else pErrMsg("Found null field"); \
			}
#define mDoAllElemObjs(fn) \
			for( int idx=0; idx<nElems(); idx++ ) \
			{ \
			    uiObject* obj = elemObj( idx ); \
			    if ( obj ) obj->fn; \
			    else pErrMsg("Found null elemObj"); \
			}

			//! stores current value as clear state.
    void		initClearValue()
			{ mDoAllElems(initClearValue()) }
    void		clear()
			{ mDoAllElems(clear()) }

    void		display( bool yn, int elemidx )
			{
			    if ( elemidx < 0 )
				{ mDoAllElemObjs(display(yn)) }
			    else
			    {
				uiObject* obj = elemObj( elemidx );
				if ( obj )
				    obj->display( yn );
			    }
			} 


    bool		isReadOnly( int idx=0 ) const
			{ 
			    const UserInputObj* obj = element( idx );
			    return obj && obj->isReadOnly();
			}
    virtual void	setReadOnly( bool yn=true, int idx=0 )
			{
			    UserInputObj* obj = element( idx );
			    if ( obj ) obj->setReadOnly( yn );
			}

    void		setSensitive(bool yn, int elemidx=-1 )
			{ 
			    if ( elemidx < 0 )
				{ mDoAllElemObjs(setSensitive(yn)) }
			    else
			    {
				uiObject* obj = elemObj( elemidx );
				if ( obj )
				    obj->setSensitive( yn );
			    }
			}

    DataInpSpec&	spec()				{ return spec_; }
    const DataInpSpec&	spec() const			{ return spec_; }

    bool                update( const DataInpSpec& nw )
			{
			    if ( spec_.type() == nw.type() && update_(nw) )
			    {
				spec_ = nw;
				return true;
			    }

			    return false;
			}

    virtual void	updateSpec()
			{ 
			    for( int idx=0; idx<nElems()&&element(idx); idx++ )
				spec_.setText( element(idx)->text(), idx );
			}

    void		valChangingNotify(CallBacker*)
			    { p_->valuechanging.trigger( *p_ ); }

    void		valChangedNotify(CallBacker*)
			    { p_->valuechanged.trigger( *p_ ); }

    void		updateReqNotify(CallBacker*)
			{ p_->updateRequested.trigger( *p_ ); }

protected:

    virtual bool        update_( const DataInpSpec& nw )
			    { 
				return nElems() == 1 && element( 0 )
						    ? element( 0 )->update(nw)
						    : false; 
			    } 

    DataInpSpec&	spec_;
    uiGenInput*		p_;

    void		init()
			    {
				update_( spec_ );

				uiObject::SzPolicy hpol =
					 p_ ? p_->elemSzPol() : uiObject::Undef;

				if ( hpol == uiObject::Undef )
				{
				    int nel = p_ ? p_->nrElements() : nElems();

				    switch( spec_.type().rep() )
				    {
				    case DataType::stringTp:
					hpol = nel > 1 ? uiObject::SmallVar
						       : uiObject::MedVar;
				    break;
				    case DataType::boolTp:
					hpol = nel > 1 ? uiObject::SmallVar
						       : uiObject::MedVar;
				    break;
				    default:
					hpol = nel > 1 ? uiObject::Small
						       : uiObject::Medium;
				    break;
				    }
				}

				mDoAllElemObjs(setHSzPol(hpol))
			    }
};


#define mName( dis, idx, defnm ) \
    ( dis.name(idx) ? dis.name(idx) : defnm )
				      
template <class T>
class uiSimpleInputFld : public uiInputFld
{
public:
uiSimpleInputFld( uiGenInput* p, const DataInpSpec& dis,
		  const char* nm="Line Edit Field" ) 
    : uiInputFld(p,dis)
    , usrinpobj(*new T(p,dis,mName(dis,0,nm))) 
{
    init();
    setReadOnly( false );

    usrinpobj.notifyValueChanging( mCB(this,uiInputFld,valChangingNotify) );
    usrinpobj.notifyValueChanged( mCB(this,uiInputFld,valChangedNotify) );
    usrinpobj.notifyUpdateRequested( mCB(this,uiInputFld,updateReqNotify) );
}

virtual	~uiSimpleInputFld()	{ delete &usrinpobj; }

virtual UserInputObj* element( int idx=0 )
{ return idx == 0 ? &usrinpobj : 0; }

virtual uiObject* mainObj()
{ return dynamic_cast<uiObject*>(&usrinpobj); }

protected:

    T&			usrinpobj;
};

typedef uiSimpleInputFld<uiLineEdit>	uiTextInputFld;


class uiFileInputFld : public uiSimpleInputFld<uiLineEdit>
{
public:
			uiFileInputFld( uiGenInput* p, 
					 const DataInpSpec& dis,
					 const char* nm="File Input Field" ) 
			    : uiSimpleInputFld<uiLineEdit>( p, dis, 
				    			    mName(dis,0,nm) )
			    { setText( dis.text() ? dis.text() : "", 0 ); }

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
					 const DataInpSpec& dis,
					 const char* nm="Bool Input Field" ) 
			    : uiSimpleInputFld<uiBoolInput>( p, dis, 
				    			     mName(dis,0,nm) )
			    {}

    virtual uiObject*	mainObj()	{ return usrinpobj.mainObject(); }
};


class uiPositionInpFld : public uiInputFld
{
public:
			uiPositionInpFld( uiGenInput* p, 
					 const DataInpSpec& dis,
					 const char* nm="Position Inp Field" );

    virtual uiObject*	mainObj()		{ return flds_[0]; }

    virtual int		nElems() const		{ return posInpSpec().nElems();}
    virtual UserInputObj* element( int idx )
    			{ return idx >= flds_.size() ? 0 : flds_[idx]; }

    bool		notifyValueChanged(const CallBack& cb )
                            { valueChanged.notify(cb); return true; }

    Notifier<uiPositionInpFld> valueChanged;

    void		commitToSetup() const;
    const PositionInpSpec&	posInpSpec() const;

protected:

    ObjectSet<uiLineEdit> flds_;

    virtual const char*	getvalue_(int) const;
    virtual void        setvalue_(const char*,int);

    virtual bool	update_(const DataInpSpec&);

    void		addFld(uiParent*,const char*);

};


uiPositionInpFld::uiPositionInpFld( uiGenInput* p, const DataInpSpec& dis,
			 		const char* nm ) 
    : uiInputFld(p,dis)
    , valueChanged(this)
{
    mDynamicCastGet(const PositionInpSpec*,spc,&dis)
    if ( !spc ) { pErrMsg("HUH - expect crash"); return; }

    addFld( p, mName(dis,0,nm) );
    const bool istrcnr = spc->setup().is2d_ && !spc->setup().wantcoords_;
    if ( spc->setup().isps_ || !istrcnr )
	addFld( p, mName(dis,1,nm) );
    if ( spc->setup().isps_ && !istrcnr )
	addFld( p, mName(dis,2,nm) );

    init();
}


void uiPositionInpFld::addFld( uiParent* p, const char* nm )
{
    const int elemidx = flds_.size();
    BufferString lenm( nm ); nm += elemidx;
    uiLineEdit* fld = new uiLineEdit( p, lenm );
    flds_ += fld;
    fld->notifyValueChanging( mCB(this,uiInputFld,valChangingNotify) );
    fld->notifyValueChanged( mCB(this,uiInputFld,valChangedNotify) );
    if ( elemidx > 0 )
	flds_[elemidx]->attach( rightTo, flds_[elemidx-1] );
}


const PositionInpSpec& uiPositionInpFld::posInpSpec() const
{
    mDynamicCastGet(PositionInpSpec&,spc,spec_)
    return spc;
}


void uiPositionInpFld::commitToSetup() const
{
    PositionInpSpec::Setup& setup
	= const_cast<PositionInpSpec&>( posInpSpec() ).setup();
    if ( setup.wantcoords_ )
    {
	setup.coord_.x = flds_[0]->getdValue();
	setup.coord_.y = flds_[1]->getdValue();
	if ( setup.isps_ )
	    setup.offs_ = flds_[2]->getfValue();
    }
    else if ( setup.is2d_ )
    {
	setup.binid_.crl = flds_[0]->getIntValue();
	if ( setup.isps_ )
	    setup.offs_ = flds_[1]->getfValue();
    }
    else
    {
	setup.binid_.inl = flds_[0]->getIntValue();
	setup.binid_.crl = flds_[1]->getIntValue();
	if ( setup.isps_ )
	    setup.offs_ = flds_[2]->getfValue();
    }
}


bool uiPositionInpFld::update_( const DataInpSpec& dis )
{
    mDynamicCastGet(const PositionInpSpec&,spc,dis)
    PositionInpSpec& myspec = const_cast<PositionInpSpec&>(posInpSpec());
    const PositionInpSpec::Setup& su = spc.setup();
    const PositionInpSpec::Setup& mysu = myspec.setup();
    if ( su.is2d_ != mysu.is2d_ || su.isps_ != mysu.isps_ )
	{ pErrMsg("Bugger"); return false; }

    myspec = spc;

#define mSetFld(nr,val) \
    { if ( mIsUdf(val) ) flds_[nr]->UserInputObjImpl<const char*>::clear(); \
      else flds_[nr]->setValue( val ); }

    if ( su.wantcoords_ )
	mSetFld( 0, su.coord_.x )
    else
	mSetFld( 0, su.is2d_ ? su.binid_.crl : su.binid_.inl )
    if ( flds_.size() < 2 ) return true;

    if ( su.wantcoords_ )
	mSetFld( 1, su.coord_.y )
    else if ( su.isps_ )
	mSetFld( 1, su.is2d_ ? su.offs_ : su.binid_.crl )
    else
	mSetFld( 1, su.binid_.crl )
    if ( flds_.size() < 3 ) return true;

    if ( su.isps_ )
	mSetFld( 2, su.offs_ )

    return true;
}


const char* uiPositionInpFld::getvalue_( int idx ) const		
{
    if ( idx >= flds_.size() )
	return 0;

    return flds_[idx]->text();
}


void uiPositionInpFld::setvalue_( const char* t, int idx )
{
    if ( idx >= flds_.size() )
	return;

    flds_[idx]->setText( t );
}


template<class T>
class uiIntervalInpFld : public uiInputFld
{
public:

			uiIntervalInpFld( uiGenInput* p, 
					 const DataInpSpec& dis,
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
				return le(idx) ? Conv::to<T>( le(idx)->text() ) 
					       : mUdf(T); 
			    }

    virtual void        setvalue_( T t, int idx)	
			    { 
				if ( le(idx) ) 
				    le(idx)->setText( Conv::to<const char*>(t));
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
uiIntervalInpFld<T>::uiIntervalInpFld( uiGenInput* p, const DataInpSpec& dis,
				       const char* nm ) 
    : uiInputFld( p, dis )
    , intvalGrp( *new uiGroup(p,nm) ) 
    , start( *new uiLineEdit(&intvalGrp,mName(dis,0,nm)) )
    , stop( *new uiLineEdit(&intvalGrp,mName(dis,1,nm)) )
    , step( 0 )
{
    mDynamicCastGet(const NumInpIntervalSpec<T>*,spc,&dis)
    if (!spc) { pErrMsg("Huh"); return; }

    if ( (!dis.name(0) || !dis.name(1)) && nm && *nm )
    {
	start.setName( BufferString(nm," start").buf() );
	stop.setName( BufferString(nm," stop").buf() );
    }

    start.notifyValueChanging( mCB(this,uiInputFld,valChangingNotify) );
    stop.notifyValueChanging( mCB(this,uiInputFld,valChangingNotify) );

    start.notifyValueChanged( mCB(this,uiInputFld,valChangedNotify) );
    stop.notifyValueChanged( mCB(this,uiInputFld,valChangedNotify) );

    start.setReadOnly( false );
    stop.setReadOnly( false );

    if ( spc->hasStep() )
    {
	step = new uiLineEdit(&intvalGrp,mName(dis,2,nm));
	if ( !dis.name(2) && nm && *nm )
	    step->setName( BufferString(nm," step").buf() );

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
bool uiIntervalInpFld<T>::update_( const DataInpSpec& dis )
{
    start.setText( dis.text(0) );
    stop.setText( dis.text(1) );
    if ( step  )
	step->setText( dis.text(2) );

    return true;
}


class uiStrLstInpFld : public uiInputFld
{
public:
			uiStrLstInpFld( uiGenInput* p, 
					 const DataInpSpec& dis,
					 const char* nm="uiStrLstInpFld" ) 
			    : uiInputFld( p, dis )
			    , cbb( *new uiComboBox(p,mName(dis,0,nm)) ) 
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

creates a new InpFld and attaches it rightTo the last one already present in
'flds', except if this is a position: in this case it will be alignedBelow it.

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
		fld = new uiIntervalInpFld<int>( this, desc, name() ); 
	    break;
	    case DataType::floatTp:
		fld = new uiIntervalInpFld<float>( this, desc, name() ); 
	    break;
	    case DataType::doubleTp:
		fld = new uiIntervalInpFld<double>( this, desc, name() ); 
	    break;
	    }
	}
	else if ( desc.type().form() == DataType::position )
	    fld = new uiPositionInpFld( this, desc, name() ); 
	else
	    fld = new uiTextInputFld( this, desc ); 
    }
    break;
    }

    if ( ! fld ) { pErrMsg("huh"); fld = new uiTextInputFld( this, desc ); }

    const bool ispos = desc.type().form() == DataType::position;
    uiObject* other= flds.size() ? flds[ flds.size()-1 ]->mainObj() : 0;
    if ( other )
	fld->mainObj()->attach( ispos ? alignedBelow : rightTo, other );

    flds += fld;

    for( int idx=0; idx<fld->nElems(); idx++ )
	idxes += FieldIdx( flds.size()-1, idx );

    return *fld;
}


//-----------------------------------------------------------------------------

#define mInitStdMembs \
    : uiGroup(p,disptxt) \
    , finalised(false) \
    , idxes(*new TypeSet<FieldIdx>) \
    , selText(""), withchk(false), withclr(false) \
    , labl(0), cbox(0), selbut(0), clrbut(0) \
    , valuechanging(this), valuechanged(this) \
    , checked(this), updateRequested(this) \
    , checked_(false), rdonly_(false), rdonlyset_(false) \
    , elemszpol( uiObject::Undef )


uiGenInput::uiGenInput( uiParent* p, const char* disptxt, const char* inputStr)
    mInitStdMembs
{ 
    inputs += new StringInpSpec( inputStr );
    if ( disptxt && *disptxt )
	inputs[0]->setName( disptxt );
    mainObject()->finaliseStart.notify( mCB(this,uiGenInput,doFinalise) );
}


uiGenInput::uiGenInput( uiParent* p, const char* disptxt,
			const DataInpSpec& inp1 )
    mInitStdMembs
{
    inputs += inp1.clone();
    const bool inputhasnm = inputs[0]->name() && *inputs[0]->name();
    if ( disptxt && *disptxt && !inputhasnm )
	inputs[0]->setName( disptxt );
    mainObject()->finaliseStart.notify( mCB(this,uiGenInput,doFinalise) );
}


uiGenInput::uiGenInput( uiParent* p, const char* disptxt
	    , const DataInpSpec& inp1 , const DataInpSpec& inp2 )
    mInitStdMembs
{
    inputs += inp1.clone();
    inputs += inp2.clone();
    mainObject()->finaliseStart.notify( mCB(this,uiGenInput,doFinalise) );
}


uiGenInput::uiGenInput( uiParent* p, const char* disptxt
	    , const DataInpSpec& inp1, const DataInpSpec& inp2 
	    , const DataInpSpec& inp3 )
    mInitStdMembs
{
    inputs += inp1.clone();
    inputs += inp2.clone();
    inputs += inp3.clone();
    mainObject()->finaliseStart.notify( mCB(this,uiGenInput,doFinalise) );
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
    mainObject()->finaliseStart.notify( mCB(this,uiGenInput,doFinalise) );
}


const DataInpSpec* uiGenInput::dataInpSpec( int nr ) const
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


void uiGenInput::updateSpecs()
{
    if ( !finalised )
	{ pErrMsg("Nothing to update. Not finalised yet.");return; }

    for( int idx=0; idx < flds.size(); idx++ )
	flds[idx]->updateSpec();
}


void uiGenInput::doFinalise( CallBacker* )
{
    if ( finalised )		return;
    if ( inputs.isEmpty() )	{ pErrMsg("Knurft: No inputs"); return; }

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

    if ( !selText.isEmpty() )
    {
	selbut = new uiPushButton( this, selText, false );
	selbut->setName( BufferString(selText," ",name()) );
	selbut->activated.notify( mCB(this,uiGenInput,doSelect_) );
	selbut->attach( rightOf, lastElem );
    }

    if ( withclr )
    {
	clrbut = new uiPushButton( this, "&Clear", true );
	clrbut->setName( BufferString("Clear ",name()) );
	clrbut->attach( rightOf, selbut ? selbut : lastElem );
	clrbut->activated.notify( mCB(this,uiGenInput,doClear) );
    }

    deepErase( inputs ); // have been copied to fields.
    finalised = true;

    if ( rdonlyset_) setReadOnly( rdonly_ );

    if ( withchk ) checkBoxSel(0);	// sets elements (non-)sensitive
}


void uiGenInput::displayField( bool yn, int elemnr, int fldnr )
{
    if ( elemnr < 0 && fldnr < 0 )
    {
	uiGroup::display( yn );
	return;
    }

    for ( int idx=0; idx<flds.size(); idx++ )
    {
	if ( fldnr >= 0 && fldnr != idx ) continue;

	flds[idx]->display( yn, elemnr );
    }
}


void uiGenInput::setReadOnly( bool yn, int nr )
{
    if ( !finalised ) { rdonly_ = yn; rdonlyset_=true; return; }

    if ( nr >= 0  ) 
	{ if ( nr<flds.size() && flds[nr] ) flds[nr]->setReadOnly(yn); return; }

    rdonly_ = yn; rdonlyset_=true;

    for( int idx=0; idx<flds.size(); idx++ )
	flds[idx]->setReadOnly( yn );
}


void uiGenInput::setSensitive( bool yn, int elemnr, int fldnr )
{
    if ( elemnr < 0 && fldnr < 0 )
    {
	uiGroup::setSensitive( yn );
	checkBoxSel(0);
	return;
    }

    for ( int idx=0; idx<flds.size(); idx++ )
    {
	if ( fldnr >= 0 && fldnr != idx ) continue;

	flds[idx]->setSensitive( yn, elemnr );
    }
}


void uiGenInput::clear( int nr )
{
    if ( !finalised ){ pErrMsg("Nothing to clear. Not finalised yet.");return; }

    if ( nr >= 0 )
	{ if ( nr<flds.size() && flds[nr] ) flds[nr]->clear(); return; }

    for( int idx=0; idx < flds.size(); idx++ )
	flds[idx]->clear();
}


int uiGenInput::nrElements() const
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


void uiGenInput::setToolTip( const char* tt, int ielem )
{
    UserInputObj* elem = element( ielem );
    if ( elem )
	elem->setToolTip( tt );
}


void uiGenInput::setValue( const Interval<int>& i )
{
    setValue(i.start,0); setValue(i.stop,1);
    mDynamicCastGet(const StepInterval<int>*,si,&i)
    if ( si ) setValue(si->step,2);
}


void uiGenInput::setValue( const Interval<double>& i )
{
    setValue(i.start,0); setValue(i.stop,1);
    mDynamicCastGet(const StepInterval<double>*,si,&i)
    if ( si ) setValue(si->step,2);
}


void uiGenInput::setValue( const Interval<float>& i )
{
    setValue(i.start,0); setValue(i.stop,1);
    mDynamicCastGet(const StepInterval<float>*,si,&i)
    if ( si ) setValue(si->step,2);
}


UserInputObj* uiGenInput::element( int nr )
{ 
    if ( !finalised ) return 0; 
    return nr<idxes.size() && flds[idxes[nr].fldidx]
	    ? flds[idxes[nr].fldidx]->element(idxes[nr].subidx) : 0; 
}


uiObject* uiGenInput::rightObj()
{
    if ( flds.isEmpty() ) return 0;
    uiInputFld& fld = *flds[flds.size()-1];
    const int nelem = fld.nElems();
    if ( nelem < 1 ) return 0;
    return fld.elemObj(nelem-1);
}


DataInpSpec* uiGenInput::getInputSpecAndIndex( const int nr, int& idx ) const
{
    int inpidx=0; idx=nr;
    while(  idx>=0 && inpidx<inputs.size() && inputs[inpidx]
	    && idx>=inputs[inpidx]->nElems() )
    {
	idx -= inputs[inpidx]->nElems();
	inpidx++;
    }

    return inpidx>=inputs.size() || !inputs[inpidx] ? 0
	 : const_cast<DataInpSpec*>( inputs[inpidx] );
}


uiInputFld* uiGenInput::getInputFldAndIndex( const int nr, int& idx ) const
{
    if ( nr < 0 || nr >= idxes.size() ) return 0;

    idx = idxes[nr].subidx;
    return const_cast<uiInputFld*>( flds[idxes[nr].fldidx] );
}


bool uiGenInput::isUndef( int nr ) const 
{ 
    int elemidx=0;
    if ( !finalised ) 
    {
	DataInpSpec* dis = getInputSpecAndIndex(nr, elemidx);

	return dis ? dis->isUndef(elemidx) : true;
    }

    uiInputFld* fld = getInputFldAndIndex( nr, elemidx );

    return fld ? fld->isUndef(elemidx) : true;
}


Coord uiGenInput::getCoord( int nr, double udfval ) const
{
    const DataInpSpec* dis = dataInpSpec( nr );
    if ( !dis || dis->type().form() != DataType::position )
	return Coord( getdValue(nr*2,udfval), getdValue(nr*2+1,udfval));

    mDynamicCastGet(const uiPositionInpFld*,posinpfld,flds[nr])
    posinpfld->commitToSetup();
    return reinterpret_cast<const PositionInpSpec*>(dis)->getCoord( udfval );
}


BinID uiGenInput::getBinID( int nr, int udfval ) const
{
    const DataInpSpec* dis = dataInpSpec( nr );
    if ( !dis || dis->type().form() != DataType::position )
	return BinID( getIntValue(nr*2,udfval), getIntValue(nr*2+1,udfval));

    mDynamicCastGet(const uiPositionInpFld*,posinpfld,flds[nr])
    posinpfld->commitToSetup();
    return reinterpret_cast<const PositionInpSpec*>(dis)->getBinID( udfval );
}


float uiGenInput::getOffset( int nr, float udfval ) const
{
    const DataInpSpec* dis = dataInpSpec( nr );
    if ( !dis || dis->type().form() != DataType::position )
	return getfValue(nr,udfval);

    mDynamicCastGet(const uiPositionInpFld*,posinpfld,flds[nr])
    posinpfld->commitToSetup();
    return reinterpret_cast<const PositionInpSpec*>(dis)->getOffset( udfval );
}


int uiGenInput::getTrcNr( int nr, int udfval ) const
{
    const DataInpSpec* dis = dataInpSpec( nr );
    if ( !dis || dis->type().form() != DataType::position )
	return getIntValue(nr,udfval);

    mDynamicCastGet(const uiPositionInpFld*,posinpfld,flds[nr])
    posinpfld->commitToSetup();
    return reinterpret_cast<const PositionInpSpec*>(dis)->getTrcNr( udfval );
}



#define g_func	text
#define s_func	setText
#define gs_type const char*
#include "fromlegs.h"

#define g_func	getIntValue
#define s_func	setValue
#define gs_type int
#include "fromlegs.h"

#define g_func	getdValue
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
    setName( txt );	//If object is not finalized
    if ( labl ) labl->setText( txt );
    if ( cbox ) cbox->setText( txt ); 
}


void uiGenInput::setChecked( bool yn )
{
    checked_ = yn; 
    if ( cbox ) cbox->setChecked( yn ); 
}


bool uiGenInput::isChecked()
{ return checked_; }


void uiGenInput::checkBoxSel( CallBacker* cb )
{
    if ( !cbox ) return;

    checked_ = cbox->isChecked();

    const bool elemsens = cbox->sensitive() && cbox->isChecked();

    for ( int idx=0; idx<flds.size(); idx++ )
    {
	flds[idx]->setSensitive( elemsens );
    }

    if ( selbut ) selbut->setSensitive( elemsens );
    if ( clrbut ) clrbut->setSensitive( elemsens );
    checked.trigger(cb);
}


void uiGenInput::doSelect_( CallBacker* cb )
{ doSelect(cb); }


void uiGenInput::doClear( CallBacker* )
{ clear(); }


void uiGenInput::setNrDecimals( int nrdec, int fldnr )
{
    if ( !flds.validIdx(fldnr) ) return;

    mDynamicCastGet(uiTextInputFld*,textinp,flds[fldnr])
    if ( !textinp ) return;
    mDynamicCastGet(uiLineEdit*,lineedit,textinp->mainObj())
    if ( !lineedit ) return;

    uiFloatValidator fv; fv.nrdecimals_ = nrdec;
    lineedit->setValidator( fv );
}
