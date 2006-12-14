#ifndef tabledef_h
#define tabledef_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H.Bril
 Date:		Oct 2006
 RCS:		$Id: tabledef.h,v 1.10 2006-12-14 18:34:37 cvsbert Exp $
________________________________________________________________________

-*/

#include "sets.h"
#include "rowcol.h"
#include "namedobj.h"
#include "datainpspec.h"
#include "bufstringset.h"
#include <iostream>

namespace Table
{

    enum ReqSpec	{ Optional=0, Required=1 };

/*!\brief Description of a Logical piece of information.

 In most simple situations, you need to know the column of some data, or the
 row/col of a header. Then you just describe it as:
 Table::TargetInfo ti( "Sample rate", FloatInpSpec() );

 In more complex situations, data can be present or offered in various ways.
 For example, an interval can be specified as start/stop or start/width. This
 would lead to the definition of multiple 'Form's.
 
 */

class TargetInfo : public NamedObject
{
public:

/*!\brief Specific form in which a piece of information can be found.

 In simple situations, there is only one form and you don;t need to bother.
 If not, for example an interval, which can be specified as start/stop or
 start/width, you'd have to specify:

 TargetInfo::Form* form = new TargetInfo::Form( "Start/Stop", FloatInpSpec() );
 form->add( FloatInpSpec() );
 Table::TargetInfo infspec( "Sampling info", form );
 infspec.add( form->duplicate( "Start/Width" ) );

*/

    struct Form : NamedObject
    {
			Form( const char* nm, DataInpSpec* spec=0 )
					//!< Single item
			    : NamedObject(nm)
				{ add( spec ); }
			Form( const char* nm, const DataInpSpec& spec )
					//!< Single item
			    : NamedObject(nm)
				{ add( spec.clone() ); }
			Form( const char* nm, ObjectSet<DataInpSpec>& specs )
					//!< Multi items
			    : NamedObject(nm)
				{ specs_.copy(specs); specs.erase(); }
			~Form()	{ deepErase(specs_); }

	Form&		add( DataInpSpec* spec )
			{
			    specs_ += spec ? spec : new StringInpSpec;
			    return *this;
			}
	Form&		add( const DataInpSpec& spec )
				{ specs_ += spec.clone(); return *this; }

	Form*		duplicate( const char* nm ) const
	    		{
			    Form* newform = new Form( nm, specs_[0]->clone() );
			    for ( int idx=0; idx<specs_.size(); idx++ )
				newform->add( specs_[idx]->clone() );
			    return newform;
			}

	ObjectSet<DataInpSpec>	specs_;
    };

    			TargetInfo( const char* nm, ReqSpec rs=Optional )
					//!< Single string
			    : NamedObject(nm)
			    , req_(rs)	{ add( nm ); }
    			TargetInfo( const char* nm, DataInpSpec* spec,
				  ReqSpec rs=Optional )
					//!< Single item
			    : NamedObject(nm)
			    , req_(rs)	{ add( nm, spec ); }
    			TargetInfo( const char* nm, const DataInpSpec& spec,
				  ReqSpec rs=Optional )
					//!< Single item
			    : NamedObject(nm)
			    , req_(rs)	{ add( nm, spec.clone() ); }
    			TargetInfo( const char* nm,
				    ObjectSet<DataInpSpec>& specs,
				    ReqSpec rs=Optional )
					//!< Multi items, in one form
			    : NamedObject(nm)
			    , req_(rs)	{ add( nm, specs ); }
    			TargetInfo( const char* nm, ObjectSet<Form>& forms,
				    ReqSpec rs=Optional )
					//!< Multi items, in multiple form
			    : NamedObject(nm)
			    , req_(rs)
					{ forms_.copy(forms); forms.erase(); }

			~TargetInfo()		{ deepErase( forms_ ); }

    TargetInfo&		add( const char* nm, DataInpSpec* spec=0 )
			    { forms_ += new Form( nm, spec ); return *this; }
    TargetInfo&		add( const char* nm, const DataInpSpec& spec )
			    { forms_ += new Form( nm, spec ); return *this; }
    TargetInfo&		add( const char* nm, Form* frm )
			    { forms_ += frm; return *this; }
    TargetInfo&		add( const char* nm, ObjectSet<DataInpSpec>& specs )
			    { forms_ += new Form( nm, specs ); return *this; }

    bool		isOptional() const	{ return req_ == Optional; }
    int			nrForms() const		{ return forms_.size(); }
    Form&		form( int idx )		{ return *forms_[idx]; }
    const Form&		form( int idx ) const	{ return *forms_[idx]; }

    /*!\brief Selected element/positioning
      This selects the specific form and where it can be found in the file,
      or explicit values for the form elements.
     */
    struct Selection
    {
			Selection() : form_(0)	{}

	int		form_;
	TypeSet<RowCol>	pos_;
	BufferStringSet	vals_;	//!< when !havePos(isub)

	bool		havePos( int ispec ) const
	    		{ return ispec < pos_.size() && pos_[ispec].c() >= 0; }
	const char*	getVal( int ispec ) const
	    		{ return ispec >= vals_.size() ? 0
			       : ((const char*)vals_.get(ispec).buf()); }

    };

    mutable Selection	selection_;

protected:

    ReqSpec		req_;
    ObjectSet<Form>	forms_;

};


/*!\brief description of input our output data content */

class FormatDesc : public NamedObject
{
public:
    			FormatDesc( const char* nm )
			    : NamedObject(nm)
			    , nrhdrlines_(0)
			    , tokencol_(-1)		{}
			~FormatDesc()
			{
			    deepErase( headerinfos_ );
			    deepErase( bodyinfos_ );
			}

    ObjectSet<TargetInfo> headerinfos_;
    ObjectSet<TargetInfo> bodyinfos_;

    int			nrhdrlines_;	//!< if < 0 token will be used
    BufferString	token_;
    int			tokencol_;	//!< if < 0 token can be in any col

    bool		needToken() const
    			{ return nrhdrlines_ < 0 && !token_.isEmpty(); }
    int			nrHdrLines() const
			{ return needToken() ? mUdf(int)
			       : nrhdrlines_ > 0 ? nrhdrlines_ : 0; }
};

}; // namespace Table


#endif
