#ifndef uigeninput_h
#define uigeninput_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Bril
 Date:          Oct 2000
 RCS:           $Id: uigeninput.h,v 1.3 2001-02-16 17:01:55 arend Exp $
________________________________________________________________________

-*/

#include <uigroup.h>
//#include <ranges.h>

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

*/

class uiGenInput : public uiGroup
{
public:
			uiGenInput( uiObject* p, const char* disptxt ); 

			uiGenInput( uiObject* p, const char* disptxt 
			    ,const DataInpSpec& );

			uiGenInput( uiObject* p, const char* disptxt 
			    ,const DataInpSpec& ,const DataInpSpec& );

			uiGenInput( uiObject* p, const char* disptxt 
			    ,const DataInpSpec&, const DataInpSpec& 
			    ,const DataInpSpec& );

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

    void		setText(const char*,int nr=0);
    void		setValue(int,int nr=0);
    void		setValue(double,int nr=0);
    void		setValue(float,int nr=0);
    void		setValue(bool,int nr=0);

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
			    { selText = yn ? "Select..." : "" ; }

protected:

    ObjectSet<uiDataInpField> flds;

    BufferString	selText;
    bool		withchk;
    bool		withclr;

    uiLabel*		labl;
    uiCheckBox*		cbox;
    uiPushButton*	selbut;
    uiPushButton*	clrbut;

                        //! "Select.." is pressed. Calls virtual doSelect
    void		doSelect_(CallBacker*);
                        //! "Select.." is pressed. Called by doSelect_
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
