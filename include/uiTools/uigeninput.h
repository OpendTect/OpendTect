#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "uigroup.h"
#include "datainpspec.h"
#include "position.h"
#include "uistring.h"

class BinIDValue;
class uiLineEdit;
class uiLabel;
class uiCheckBox;
class uiString;
class uiButton;
class uiGenInputInputFld;
class uiGenInputFieldIdx;
class uiTextValidator;
class DataInpSpec;
class UserInputObj;

/*! \brief General Input Element

The General Input line consists of a number of input elements,
optionally accompanied with either a label or a checkbox.

The uiGeninput is constructed with a number of DataInpSpec's, which determine
what kind of input field is going to be under the hood.

The usage of the DataInpSpec keeps the constructor for uiGenInput very simple
and also it's most common use. For example, to define a uiGenInput that inputs
a string, you'd do something like

\code
uiGenInput* xx = new uiGenInput( this,"Input name", StringInpSpec("Initial"));
\endcode

For more complicated inputs, you'd have to explicitely construct a
DataInpSpec sub-class, and fill in it's details, like allowed range, etc.

When passed to the constructor of a uiGenInput, the DataInpSpec objects
are copied. You can use uiGenInput::getInpSpec to get a hold of them, but
only if the uiGenInput has not been finalized yet.


*/

mExpClass(uiTools) uiGenInput : public uiGroup
{mODTextTranslationClass(uiGenInput)
public:
			uiGenInput(uiParent* p,const uiString& disptxt,
				   const char* inputStr=nullptr);

			uiGenInput(uiParent* p,const uiString& disptxt,
				   const DataInpSpec&);

			uiGenInput(uiParent* p,const uiString&,
				   const DataInpSpec&,const DataInpSpec&);

			uiGenInput(uiParent* p,const uiString&,
				   const DataInpSpec&,const DataInpSpec&,
				   const DataInpSpec& );

    virtual		~uiGenInput();

    void		addInput( const DataInpSpec& );

/*! \brief gives access to the actual spec of an input field.

When the uiGenInput is being finalized, the actual input fields are
constructed. The fields make a clone of their input spec,
which are used for range checking, etc.

The purpose of this is to be able to store the actual current spec's,
instead of the intial specs, which might have been updated with newSpec().
For example, limits might have been set/changed.

\sa  bool newSpec(DataInpSpec* nw, int nr)

*/
    const DataInpSpec*	dataInpSpec(int nr=0) const;

/*! \brief makes it possible to change the actual spec for an input field.

Returns true, if changes are accepted.

*/
    bool		newSpec(const DataInpSpec& nw, int nr);

    //! update dataInpSpec() from current values on screen.
    void		updateSpecs();

    bool		isUndef( int nr=0 ) const;

#undef mDeclArgs
#define mDeclArgs(type)	int nr=0, type undefval=mUdf(type)

    const char*		text( mDeclArgs(const char*) ) const;
    const char*		text( const char* undefval)  const
			    { return text(0,undefval); }

    int			getIntValue( mDeclArgs(int) ) const;
    od_int64		getInt64Value( mDeclArgs(od_int64) ) const;
    bool		getBoolValue( mDeclArgs(bool) ) const;

    double		getDValue( mDeclArgs(double) ) const;
    double		getDValue( double undefval ) const
			    { return getDValue(0,undefval) ; }

    float		getFValue( mDeclArgs(float) ) const;
    float		getFValue( float undefval ) const
			    { return getFValue(0,undefval); }

    inline Interval<int> getIInterval( mDeclArgs(int) ) const
			{ return Interval<int>( getIntValue(nr*2,undefval),
						getIntValue(nr*2+1,undefval) );}

    inline Interval<float> getFInterval( mDeclArgs(float) ) const
			{ return Interval<float>( getFValue(nr*2,undefval),
						  getFValue(nr*2+1,undefval) );}
    inline Interval<float> getFInterval( float undefval ) const
			{ return getFInterval(0,undefval); }

    inline Interval<double> getDInterval( mDeclArgs(double) ) const
			{ return Interval<double>(getDValue(nr*2,undefval),
						  getDValue(nr*2+1,undefval) );}
    inline Interval<double> getDInterval( double undefval ) const
			{ return getDInterval(0,undefval); }

    inline StepInterval<int> getIStepInterval( mDeclArgs(int) ) const
			{ return StepInterval<int>(getIntValue(nr*3,undefval),
						   getIntValue(nr*3+1,undefval),
						   getIntValue(nr*3+2,undefval)
						   ); }
    inline StepInterval<float> getFStepInterval( mDeclArgs(float) ) const
			{ return StepInterval<float>(getFValue(nr*3,undefval),
						     getFValue(nr*3+1,undefval),
						     getFValue(nr*3+2,undefval)
						    ); }
    inline StepInterval<float> getFStepInterval( float undefval ) const
			{ return getFStepInterval(0,undefval); }

    inline StepInterval<double> getDStepInterval( mDeclArgs(double) ) const
			{ return StepInterval<double>(getDValue(nr*3,undefval),
						 getDValue(nr*3+1,undefval),
						 getDValue(nr*3+2,undefval) ); }
    inline StepInterval<double> getDStepInterval( double undefval ) const
			{ return getDStepInterval(0,undefval); }

    Coord		getCoord( mDeclArgs(double) ) const;
    BinID		getBinID( mDeclArgs(int) ) const;
    int			getTrcNr( mDeclArgs(int) ) const;
    float		getOffset( mDeclArgs(float) ) const;

    void		setTextValidator(const uiTextValidator&);
    void		setDefaultTextValidator();


#undef mDeclArgs

    void		setText(const char*,int nr=0);
    void		setValue(int,int nr=0);
    void		setValue(od_int64,int nr=0);
    void		setValue(double,int nr=0);
    void		setValue(float,int nr=0);
    void		setValue(bool,int nr=0);

    inline void		setTexts( const char* v1, const char* v2 )
			{ setText(v1,0); setText(v2,1); }
    inline void		setValues( int v1, int v2 )
			{ setValue(v1,0); setValue(v2,1); }
    inline void		setValues( double v1, double v2 )
			{ setValue(v1,0); setValue(v2,1); }
    inline void		setValues( float v1, float v2 )
			{ setValue(v1,0); setValue(v2,1); }
    inline void		setValues( bool v1, bool v2 )
			{ setValue(v1,0); setValue(v2,1); }
    inline void		setValue( const Coord& c )
			{ setValue(c.x,0); setValue(c.y,1); }
    inline void		setValue( const BinID& b )
			{ setValue(b.inl(),0); setValue(b.crl(),1); }
    inline void		setValue(const BinIDValue&);
    void		setValue(const Interval<int>&); //!< and StepIntv
    void		setValue(const Interval<float>&); //!< and StepIntv
    void		setValue(const Interval<double>&); //!< and StepIntv

    void		displayField(bool yn=true,int elemnr=-1,int fldnr=-1);
    void		setReadOnly( bool yn=true,int elemnr=-1,int fldnr=-1);
    void		setSensitive(bool yn=true,int elemnr=-1,int fldnr=-1);
    void		setEmpty(int nr=-1);

			//! returns 0 if not finalized.
    UserInputObj*	element(int idx);
    uiObject*		rightObj();			//!< for attaching
    int			nrElements() const;
    void		setElemSzPol( uiObject::SzPolicy p )	{ elemszpol_=p;}
    uiObject::SzPolicy	elemSzPol() const		{ return elemszpol_; }
    void		setToolTip(const uiString&,int ielem=0);

    virtual const uiString&	titleText();
    virtual void		setTitleText(const uiString&);

    void		setChecked(bool yn);
    bool		isChecked() const	{ return checked_; }
    bool		isCheckable() const	{ return withchk_ || cbox_; }

    virtual bool	isSingleLine() const		{ return true; }

    void		setWithCheck( bool yn=true )	{ withchk_ = yn; }
    void		setWithSelect( bool yn=true );

    void		setNrDecimals(int nrdec,int fldnr=0);
    void		setRequired(bool yn=true);
    void		setPrefix(const uiString&r,int fldnr=0);

    Notifier<uiGenInput> checked;
    Notifier<uiGenInput> valueChanging;
    Notifier<uiGenInput> valueChanged;
    Notifier<uiGenInput> updateRequested;

// Deprecated
    Notifier<uiGenInput> valuechanging;
    Notifier<uiGenInput> valuechanged;

protected:

    ObjectSet<uiGenInputInputFld>	flds_;
    TypeSet<uiGenInputFieldIdx>&	idxes_;

    bool		finalized_			= false;

    uiString		selText_;
    bool		withchk_			= false;
    bool		isrequired_			= false;

    uiLabel*		labl_				= nullptr;
    uiString		titletext_;
    uiCheckBox*		cbox_				= nullptr;
    uiButton*		selbut_				= nullptr;
    uiTextValidator*	textvl_				= nullptr;

			//! Select is pressed. Calls virtual doSelect
    void		doSelect_(CallBacker*);
			//! Select is pressed. Called by doSelect_
    virtual void	doSelect(CallBacker*)   {}
    void		doClear(CallBacker*);

    void		checkBoxSel(CallBacker*);

			//! DataInpField factory
    uiGenInputInputFld&	createInpFld(const DataInpSpec&);
    void		doFinalize(CallBacker*);
    inline DataInpSpec*	spec( int nr )
			{
			    return const_cast<DataInpSpec*>
				( ((const uiGenInput*)this)->dataInpSpec(nr) );
			}

private:
			uiGenInput(const uiString&,uiParent*);

    bool		checked_			= false;

    bool		rdonly_				= false;
    bool		rdonlyset_			= false;

    ObjectSet<DataInpSpec> inputs_;

    uiObject::SzPolicy	elemszpol_;

    DataInpSpec*	getInputSpecAndIndex(const int,int&) const;
    uiGenInputInputFld*	getInputFldAndIndex(const int,int&) const;

    mDeprecated		("Use getDValue")
    double		getdValue( int nr=0, double uv=mUdf(double) ) const
			    { return getDValue( nr, uv ); }
    mDeprecated		("Use getDValue")
    double		getdValue( float undefval ) const
			    { return getDValue(0,undefval); }

    mDeprecated		("Use getFValue")
    float		getfValue( int nr=0, float udfval=mUdf(float) ) const
			    { return getFValue( nr, udfval ); }
    mDeprecated		("Use getFValue")
    float		getfValue( float undefval ) const
			    { return getFValue(0,undefval); }
};
