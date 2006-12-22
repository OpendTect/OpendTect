#ifndef tabledef_h
#define tabledef_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H.Bril
 Date:		Oct 2006
 RCS:		$Id: tabledef.h,v 1.11 2006-12-22 10:53:26 cvsbert Exp $
________________________________________________________________________

-*/

#include "sets.h"
#include "rowcol.h"
#include "namedobj.h"
#include "datainpspec.h"
#include "property.h"

class UnitOfMeasure;


namespace Table
{

    enum ReqSpec	{ Optional=0, Required=1 };

/*!\brief Description of a Logical piece of information.

 In most simple situations, you need to know the column of some data, or the
 row/col of a header. Then you just describe it as:
 Table::TargetInfo ti( "Number of samples", IntInpSpec() );

 In more complex situations, data can be present or offered in various ways.
 For example, an interval can be specified as start/stop or start/width. This
 would lead to the definition of multiple 'Form' objects.
 
 */

class TargetInfo : public NamedObject
{
public:

/*!\brief Specific form in which a piece of information can be found.

 In simple situations, the TargetInfo is defined by one form and you don't
 need to bother investigating this object.

 Alas, for example an interval can be specified as either start/stop or
 start/width. Then you'd have to specify:

 TargetInfo::Form* form = new TargetInfo::Form( "Start/Stop", FloatInpSpec() );
 form->add( FloatInpSpec() );
 Table::TargetInfo sampinfspec( "Sampling info", form );
 sampinfspec.add( form->duplicate( "Start/Width" ) );

 Example with units of measure, if the interval describes a Z range:

 TargetInfo::Form* form = new TargetInfo::Form( "Start/Stop", FloatInpSpec(),
 						PropertyRef::surveyZType() );
 form->add( FloatInpSpec(), PropertyRef::surveyZType() );
 Table::TargetInfo sampinfspec( "Sampling info", form );
 sampinfspec.add( form->duplicate( "Start/Width" ) );

*/

#   undef mPropArg
#   define mPropArg PropertyRef::StdType p=PropertyRef::Other

    struct Form : NamedObject
    {
	//!\brief One element of a Form
	struct Elem
	{
	    				Elem( DataInpSpec* s, mPropArg )
					    : inpspec_(s?s:new StringInpSpec)
					    , proptype_(p)		{}
	    				Elem( const DataInpSpec& s, mPropArg )
					    : inpspec_(s.clone())
					    , proptype_(p)		{}
					Elem( const Elem& e )
					    : inpspec_(e.inpspec_->clone())
					    , proptype_(e.proptype_)	{}
					~Elem()	{ delete inpspec_; }

	    DataInpSpec*		inpspec_;
	    PropertyRef::StdType	proptype_;

	};
			Form( const char* nm, DataInpSpec* spec=0, mPropArg )
			    : NamedObject(nm)
				{ add( spec, p ); }
			Form( const char* nm, const DataInpSpec& spec, mPropArg)
			    : NamedObject(nm)
				{ add( spec.clone(), p ); }

			~Form()	{ deepErase(elems_); }

	Form&		add( DataInpSpec* spec, mPropArg )
			    { elems_ += new Elem( spec, p ); return *this; }
	Form&		add( const DataInpSpec& spec, mPropArg )
			    { elems_ += new Elem( spec, p ); return *this; }

	Form*		duplicate( const char* nm ) const
	    		{
			    Form* ret = new Form( nm, *elems_[0]->inpspec_,
				    		  elems_[0]->proptype_ );
			    for ( int idx=0; idx<elems_.size(); idx++ )
				ret->elems_ += new Elem( *elems_[idx] );
			    return ret;
			}

	ObjectSet<Elem> elems_;
    };

    			TargetInfo( const char* nm, ReqSpec rs=Optional )
					//!< Single string
			    : NamedObject(nm)
			    , req_(rs)	{ add( nm ); }
    			TargetInfo( const char* nm, DataInpSpec* spec,
				  ReqSpec rs=Optional, mPropArg )
					//!< Single item
			    : NamedObject(nm)
			    , req_(rs)	{ add( nm, spec, p ); }
    			TargetInfo( const char* nm, const DataInpSpec& spec,
				  ReqSpec rs=Optional, mPropArg )
					//!< Single item
			    : NamedObject(nm)
			    , req_(rs)	{ add( nm, spec.clone(), p ); }

			~TargetInfo()		{ deepErase( forms_ ); }

    TargetInfo&		add( const char* nm, DataInpSpec* spec=0, mPropArg )
			    { forms_ += new Form( nm, spec, p ); return *this; }
    TargetInfo&		add( const char* nm, const DataInpSpec& spec, mPropArg )
			    { forms_ += new Form( nm, spec, p ); return *this; }
    TargetInfo&		add( const char* nm, Form* frm )
			    { forms_ += frm; return *this; }

    bool		isOptional() const	{ return req_ == Optional; }
    int			nrForms() const		{ return forms_.size(); }
    Form&		form( int idx )		{ return *forms_[idx]; }
    const Form&		form( int idx ) const	{ return *forms_[idx]; }

#   undef mPropArg

    /*!\brief Selected element/positioning
      This selects the specific form and where it can be found in the file,
      or explicit values for the form elements.
     */
    struct Selection
    {
			Selection() : form_(0)	{}

	int		form_;

	//!\brief holds the 'value' of a certain selection
	struct Elem
	{
	    				Elem()
					    : pos_(0,-1), unit_(0)	{}
	    				Elem( const RowCol& rc )
					    : pos_(rc), unit_(0)	{}
	    				Elem( const char* s )
					    : pos_(0,-1), val_(s), unit_(0) {}
	    bool			isInFile() const
					{ return pos_.c() >= 0; }
	    bool			isSpecified() const
					{ return !val_.isEmpty(); }
	    bool			isEmpty() const
	    				{ return !isInFile() && !isSpecified();}
	    bool			operator ==( const Elem& v ) const
					{ return pos_ == v.pos_
					      && val_ == v.val_; }

	    RowCol			pos_;
	    BufferString		val_;
	    const UnitOfMeasure*	unit_;
	};

	TypeSet<Elem>	elems_;

	bool		havePos( int ielem ) const
	    		{ return ielem < elems_.size()
			      && elems_[ielem].isInFile(); }
	const char*	getVal( int ielem ) const
	    		{ return ielem >= elems_.size() ? 0
			       : ((const char*)elems_[ielem].val_); }
	const UnitOfMeasure* getUnit( int ielem ) const
	    		{ return ielem >= elems_.size() ? 0
			       : elems_[ielem].unit_; }
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
