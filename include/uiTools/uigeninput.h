#ifndef uigeninput_h
#define uigeninput_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Bril
 Date:          Oct 2000
 RCS:           $Id: uigeninput.h,v 1.6 2001-05-02 16:35:35 bert Exp $
________________________________________________________________________

-*/

#include <uigroup.h>
#include <ranges.h>
#include <position.h>

class uiLineEdit;
class uiLabel;
class uiCheckBox;
class uiPushButton;

class uiDataInpField;
class DataInpSpec;

/*! \brief General Input Element

The General Input line consists of a number of input elements, 
optionally accompanied with either a label or a checkbox.

The uiGeninput is constructed with a number of DataInpSpec's, which determine
what kind of input field is going to be under the hood.

The usage of the DataInpSpec keeps the constructor for uiGenInput very simple
and also it's most common use. For example, to define a uiGenInput that inputs
a string, you'd do something like

\code
uiGenInput* xx = new uiGenInput( this,"Input name", StringInp("Initial"));
\endcode

For more complicated inputs, you'd have to explicitely construct a
DataInpSpec sub-class, and fill in it's details, like allowed range, etc.

When passed to the constructor of a uiGenInput, the DataInpSpec objects
are copied. You can use uiGenInput::getInpSpec to get a hold of them, but
only if the uiGenInput has not been finalised yet.


*/

class uiGenInput : public uiGroup
{
public:
			uiGenInput( uiObject* p, const char* disptxt ); 

			uiGenInput( uiObject* p, const char* disptxt,
			    const DataInpSpec& );

			uiGenInput( uiObject* p, const char* disptxt,
			    const DataInpSpec& ,const DataInpSpec& );

			uiGenInput( uiObject* p, const char* disptxt,
			    const DataInpSpec&, const DataInpSpec&,
			    const DataInpSpec& );

    void		addInput( const DataInpSpec& );

/*! \brief gives access to the input specs if not yet finalised.

It is preferred to construct and manipulate a DataInpSpec before
construction of a uiGenInput, because you probably have to dynamic
cast the DataInpSpec* to it's specific child when using getInput.
Don't use when already finalised (i.e. popped up).

*/
    DataInpSpec*	getInput( int nr );

    const char*		text(int nr=0) const;
    int			getIntValue(int nr=0) const;
    double		getValue(int nr=0) const;
    float		getfValue(int nr=0) const;
    bool		getBoolValue(int nr=0) const;

    inline Interval<int> getIInterval() const
			{ return Interval<int>(getIntValue(0),getIntValue(1)); }
    inline Interval<float> getFInterval() const
			{ return Interval<float>(getfValue(0),getfValue(1)); }
    inline Interval<double> getDInterval() const
			{ return Interval<double>(getValue(0),getValue(1)); }
    inline Coord	getCoord() const
			{ return Coord(getValue(0),getValue(1)); }
    inline BinID	getBinID() const
			{ return BinID(getIntValue(0),getIntValue(1)); }

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
    inline void		setValue( const Interval<int>& i )
			{ setValue(i.start,0); setValue(i.stop,1); }
    inline void		setValue( const Interval<float>& i )
			{ setValue(i.start,0); setValue(i.stop,1); }
    inline void		setValue( const Interval<double>& i )
			{ setValue(i.start,0); setValue(i.stop,1); }

    void		setReadOnly( bool yn=true, int nr=-1 );
    void		clear( int nr=-1 );

    virtual const char*	titleText();
    virtual void	setTitleText(const char*);

    void 		setChecked( bool yn );
    bool		isChecked();
    bool		isCheckable() { return cbox ? true : false; }
			//! returns false if no checkbox available
    bool 		notifyCheck( const CallBack& ); 

    virtual bool 	isSingleLine() const		{ return true; }

    void		setWithCheck( bool yn=true )	{ withchk = yn; }
    void		setWithSelect( bool yn=true ) 
			    { selText = yn ? "Select ..." : "" ; }

protected:

    ObjectSet<uiDataInpField> flds;

    BufferString	selText;
    bool		withchk;
    bool		withclr;

    uiLabel*		labl;
    uiCheckBox*		cbox;
    uiPushButton*	selbut;
    uiPushButton*	clrbut;

                        //! "Select ..." is pressed. Calls virtual doSelect
    void		doSelect_(CallBacker*);
                        //! "Select ..." is pressed. Called by doSelect_
    virtual void	doSelect(CallBacker*)   {}
    void		doClear(CallBacker*);

    void 		checkBoxSel(CallBacker*);

			//! DataInpField factory
    uiDataInpField& 	createInpField(uiObject*, const DataInpSpec* =0);
    virtual void	finalise_();

private:

    bool		checked;
    bool		ro;
    ObjectSet<DataInpSpec> inputs;

};


#endif
