#ifndef uigeninput_h
#define uigeninput_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          Oct 2000
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "uigroup.h"
#include "datainpspec.h"
#include "position.h"
#include "errh.h"

class uiLineEdit;
class uiLabel;
class uiCheckBox;
class uiPushButton;

class uiGenInputInputFld;
class uiGenInputFieldIdx;
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
only if the uiGenInput has not been finalised yet.


*/

mExpClass(uiTools) uiGenInput : public uiGroup
{
public:
			uiGenInput( uiParent* p, const char* disptxt
				  , const char* inputStr=0 ); 

			uiGenInput( uiParent* p, const char* disptxt,
			    const DataInpSpec& );

			uiGenInput( uiParent* p, const char* disptxt,
			    const DataInpSpec& ,const DataInpSpec& );

			uiGenInput( uiParent* p, const char* disptxt,
			    const DataInpSpec&, const DataInpSpec&,
			    const DataInpSpec& );

    virtual		~uiGenInput();

    void		addInput( const DataInpSpec& );

/*! \brief gives access to the actual spec of an input field.

When the uiGenInput is being finalised, the actual input fields are 
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
#define mDeclArgs(type)	int nr=0, type undefVal=mUdf(type)

    const char*		text( mDeclArgs(const char*) ) const;
    const char*		text( const char* undefVal)  const
			    { return text(0,undefVal); }

    int			getIntValue( mDeclArgs(int) ) const;
    bool		getBoolValue( mDeclArgs(bool) ) const;

    double		getdValue( mDeclArgs(double) ) const;
    double		getdValue( double undefVal ) const
			    { return getdValue(0,undefVal) ; }

    float		getfValue( mDeclArgs(float) ) const;
    float		getfValue( float undefVal ) const
			    { return getfValue(0,undefVal); }

    inline Interval<int> getIInterval( mDeclArgs(int) ) const
			{ return Interval<int>( getIntValue(nr*2,undefVal),
						getIntValue(nr*2+1,undefVal) );}

    inline Interval<float> getFInterval( mDeclArgs(float) ) const
			{ return Interval<float>( getfValue(nr*2,undefVal),
						  getfValue(nr*2+1,undefVal) );}
    inline Interval<float> getFInterval( float undefVal ) const
			{ return getFInterval(0,undefVal); }

    inline Interval<double> getDInterval( mDeclArgs(double) ) const
			{ return Interval<double>(getdValue(nr*2,undefVal),
						  getdValue(nr*2+1,undefVal) );}
    inline Interval<double> getDInterval( double undefVal ) const
			{ return getDInterval(0,undefVal); }

    inline StepInterval<int> getIStepInterval( mDeclArgs(int) ) const
			{ return StepInterval<int>(getIntValue(nr*3,undefVal),
						   getIntValue(nr*3+1,undefVal),
						   getIntValue(nr*3+2,undefVal)
						   ); }
    inline StepInterval<float> getFStepInterval( mDeclArgs(float) ) const
			{ return StepInterval<float>(getfValue(nr*3,undefVal),
						     getfValue(nr*3+1,undefVal),
						     getfValue(nr*3+2,undefVal)
						    ); }
    inline StepInterval<float> getFStepInterval( float undefVal ) const
			{ return getFStepInterval(0,undefVal); }

    inline StepInterval<double> getDStepInterval( mDeclArgs(double) ) const
			{ return StepInterval<double>(getdValue(nr*3,undefVal),
						 getdValue(nr*3+1,undefVal),
						 getdValue(nr*3+2,undefVal) ); }
    inline StepInterval<double> getDStepInterval( double undefVal ) const
			{ return getDStepInterval(0,undefVal); }

    Coord		getCoord( mDeclArgs(double) ) const;
    BinID		getBinID( mDeclArgs(int) ) const;
    int			getTrcNr( mDeclArgs(int) ) const;
    float		getOffset( mDeclArgs(float) ) const;


#undef mDeclArgs    

    void		setText(const char*,int nr=0);
    void		setValue(int,int nr=0);
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
			{ setValue(b.inl,0); setValue(b.crl,1); }
    inline void		setValue( const BinIDValue& b )
			{ setValue(b.binid); setValue(b.value,2); }
    void		setValue(const Interval<int>&); //!< and StepIntv
    void		setValue(const Interval<float>&); //!< and StepIntv
    void		setValue(const Interval<double>&); //!< and StepIntv

    void		displayField(bool yn=true,int elemnr=-1,int fldnr=-1);
    void		setReadOnly( bool yn=true,int nr=-1);
    void		setSensitive(bool yn=true,int elemnr=-1,int fldnr=-1);
    void		clear(int nr=-1);

			//! returns 0 if not finalised.
    UserInputObj*	element(int idx); 
    uiObject*		rightObj();			//!< for attaching
    int			nrElements() const; 
    void		setElemSzPol( uiObject::SzPolicy p )	{ elemszpol=p; }
    uiObject::SzPolicy	elemSzPol() const		{ return elemszpol; }
    void		setToolTip(const char*,int ielem=0);

    virtual const char*	titleText();
    virtual void	setTitleText(const char*);

    void 		setChecked(bool yn);
    bool		isChecked();
    bool		isCheckable()
    			{ return cbox ? true : false; }

    virtual bool 	isSingleLine() const		{ return true; }

    void		setWithCheck( bool yn=true )	{ withchk = yn; }
    void		setWithSelect( bool yn=true ) 
			{ selText = yn ? "&Select" : "" ; }

    void		setNrDecimals(int nrdec,int fldnr=0);

    Notifier<uiGenInput> checked;
    Notifier<uiGenInput> valuechanging;
    Notifier<uiGenInput> valuechanged;
    Notifier<uiGenInput> updateRequested;

protected:

    ObjectSet<uiGenInputInputFld>	flds;
    TypeSet<uiGenInputFieldIdx>&	idxes;

    bool		finalised;

    BufferString	selText;
    bool		withchk;
    bool		withclr;

    uiLabel*		labl;
    uiCheckBox*		cbox;
    uiPushButton*	selbut;
    uiPushButton*	clrbut;

                        //! Select is pressed. Calls virtual doSelect
    void		doSelect_(CallBacker*);
                        //! Select is pressed. Called by doSelect_
    virtual void	doSelect(CallBacker*)   {}
    void		doClear(CallBacker*);

    void 		checkBoxSel(CallBacker*);

			//! DataInpField factory
    uiGenInputInputFld&	createInpFld(const DataInpSpec&);
    void		doFinalise(CallBacker*);
    inline DataInpSpec*	spec( int nr )
			{
			    return const_cast<DataInpSpec*>
				( ((const uiGenInput*)this)->dataInpSpec(nr) );
			}

private:

    bool		checked_;

    bool		rdonly_;
    bool		rdonlyset_;

    ObjectSet<DataInpSpec> inputs;

    uiObject::SzPolicy	elemszpol;

    DataInpSpec* 	getInputSpecAndIndex(const int,int&) const;
    uiGenInputInputFld*	getInputFldAndIndex(const int,int&) const;

};


#endif

