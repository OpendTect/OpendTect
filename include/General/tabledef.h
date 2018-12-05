#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Oct 2006
________________________________________________________________________

-*/

#include "generalmod.h"
#include "datainpspec.h"
#include "namedobj.h"
#include "propertyref.h"
#include "rowcol.h"
#include "sets.h"

class UnitOfMeasure;
namespace Coords { class CoordSystem; }


namespace Table
{

    enum ReqSpec	{ Optional=0, Required=1, Hidden=2 };

/*!\brief Description of a Logical piece of information.

 In most simple situations, you need to know the column of some data, or the
 row/col of a header. Then you just describe it as:
 Table::TargetInfo ti( "Number of samples", IntInpSpec(), Table::Required );

 In more complex situations, data can be present or offered in various ways.
 For example, an interval can be specified as start/stop or start/width. This
 would lead to the definition of multiple 'Form' objects.

 */

mExpClass(General) TargetInfo : public NamedObject
{
public:

/*!\brief Specific form in which a piece of information can be found.

 In simple situations, the TargetInfo is defined by one form and you don't
 need to bother investigating this object.

 Alas, for example an interval can be specified as either start/stop or
 start/width. Then you'd have to specify:

 Table::TargetInfo sampinfspec( "Sampling info", form, Table::Required,
				PropertyRef::surveyZType() );
 TargetInfo::Form* form = new TargetInfo::Form( "Start/Stop", FloatInpSpec() );
 form->add( FloatInpSpec() );
 sampinfspec.add( form->duplicate( "Start/Width" ) );

*/

    struct Form : NamedObject
    {
			Form( const uiString& dispnm, DataInpSpec* spec )
			    : NamedObject(toString(dispnm))
			    , dispnm_(dispnm)
				{ add( spec ); }
			Form( const uiString& dispnm, const DataInpSpec& spec )
			    : NamedObject(toString(dispnm))
			    , dispnm_(dispnm)
				{ add( spec.clone() ); }

			~Form()	{ deepErase(specs_); }

	Form&		add( const DataInpSpec& spec )
			    { specs_ += spec.clone(); return *this; }
	Form&		add( DataInpSpec* spec )
			    { specs_ += spec ? spec : new StringInpSpec;
			      return *this; }

	Form*		duplicate( const uiString& nm ) const
			{
			    Form* ret = new Form( nm, *specs_[0] );
			    for ( int idx=1; idx<specs_.size(); idx++ )
				ret->specs_ += specs_[idx]->clone();
			    return ret;
			}

	ObjectSet<DataInpSpec>	specs_;

	uiString	dispnm_;
    };


			TargetInfo( const uiString& nm, ReqSpec rs=Optional )
					//!< Single string
			    : NamedObject(toString(nm))
			    , req_(rs)
			    , dispnm_(nm)
			    , proptype_(PropertyRef::Other)
				{ add( nm ); }
			TargetInfo( const uiString& nm, DataInpSpec* spec,
				  ReqSpec rs=Optional,
				  PropertyRef::StdType p=PropertyRef::Other )
			    : NamedObject(toString(nm))
			    , req_(rs)
			    , proptype_(p)
			    , dispnm_(nm)
				{ add( nm, spec ); }
			TargetInfo( const uiString& nm, const DataInpSpec& spec,
				  ReqSpec rs=Optional,
				  PropertyRef::StdType p=PropertyRef::Other )
			    : NamedObject(toString(nm))
			    , req_(rs)
			    , proptype_(p)
			    , dispnm_(nm)
				{ add( nm, spec ); }

			~TargetInfo()		{ deepErase( forms_ ); }

    TargetInfo&		add( const uiString& nm, DataInpSpec* spec=0 )
			    { forms_ += new Form( nm, spec ); return *this; }
    TargetInfo&		add( const uiString& nm, const DataInpSpec& spec )
			    { forms_ += new Form( nm, spec ); return *this; }
    TargetInfo&		add( Form* frm )
			    { forms_ += frm; return *this; }

    bool		isOptional() const	{ return req_ != Required; }
    bool		isHidden() const	{ return req_ == Hidden; }
    PropertyRef::StdType propertyType() const	{ return proptype_; }
    void		setPropertyType( PropertyRef::StdType p )
			    { proptype_ = p; }
    int			nrForms() const		{ return forms_.size(); }
    Form&		form( int idx )		{ return *forms_[idx]; }
    const Form&		form( int idx ) const	{ return *forms_[idx]; }
    int			formNr( const char* formnm ) const
			{
			    for ( int idx=0; idx<forms_.size(); idx++ )
				if ( forms_[idx]->hasName(formnm) )
				    return idx;
			    return -1;
			}

    uiString		dispnm_;

    /*!\brief Selected element/positioning
      This selects the specific form and where/how it can be found in the file,
      or explicit values for the form elements.

     */
    struct Selection
    {
	//!\brief holds the 'value' of a certain selection
	struct Elem
	{
				Elem()
				    : pos_(0,-1)		{}
				Elem( const RowCol& rc, const char* kw=0 )
				    : pos_(rc), keyword_(kw)	{}
				Elem( const char* s )
				    : pos_(0,-1), val_(s)	{}

	    bool		isInFile() const
				    { return pos_.col() >= 0; }
	    bool		isKeyworded() const
				    { return isInFile() && !keyword_.isEmpty();}
	    bool		isSpecified() const
				    { return !val_.isEmpty(); }
	    bool		isEmpty() const
				    { return !isInFile() && !isSpecified();}
	    bool		operator ==( const Elem& v ) const
				    { return pos_ == v.pos_ && val_ == v.val_
					  && keyword_ == v.keyword_; }

	    RowCol		pos_;
	    BufferString	keyword_;
	    BufferString	val_;
	};

	int			form_;
	TypeSet<Elem>		elems_;
	const UnitOfMeasure*	unit_;
	ConstRefMan<Coords::CoordSystem>	coordsys_;

			Selection()
				: form_(0), unit_(0), coordsys_(0)	{}

	bool		havePos( int ielem ) const
			    { return ielem < elems_.size()
				  && elems_[ielem].isInFile(); }
	bool		isKeyworded( int ielem ) const
			    { return ielem < elems_.size()
				  && elems_[ielem].isKeyworded(); }
	bool		isInFile( int ielem=0 ) const
			    { return ielem < elems_.size()
				  && elems_[ielem].isInFile(); }
	const char*	getVal( int ielem ) const
			    { return ielem >= elems_.size() ? 0
				   : elems_[ielem].val_.buf(); }
	bool		isFilled() const
			{ return elems_.size() > 0 && !elems_[0].isEmpty(); }
    };

    mutable Selection	selection_;

    void		fillPar(IOPar&) const;
    void		usePar(const IOPar&);

    static TargetInfo*	mkHorPosition(bool isreq,bool wic=true,bool wll=false,
				      bool wcrs=true);
			//!< form(0)=(X,Y), form(1)=inl/crl, form(1/2)=long/lat
    static TargetInfo*	mkZPosition( bool isreq, bool withunits=true )
				{ return mkZPos(isreq,withunits); }
    static TargetInfo*	mkDepthPosition( bool isreq, bool withunits=true )
				{ return mkZPos(isreq,withunits,1); }
    static TargetInfo*	mkTimePosition( bool isreq, bool withunits=true )
				{ return mkZPos(isreq,withunits,-1); }
    bool		needsConversion() const;
    Coord		convert(const Coord&) const;

protected:

    ReqSpec		req_;
    PropertyRef::StdType proptype_;
    ObjectSet<Form>	forms_;
    static TargetInfo*	mkZPos(bool,bool wu=false,int zopt=0);

};


/*!\brief description of input our output data content */

mExpClass(General) FormatDesc : public NamedObject
{
public:
			FormatDesc( const char* nm )
			    : NamedObject(nm)
			    , nrhdrlines_(0)
			    , eohtokencol_(-1)		{}
			~FormatDesc()
			{ deepErase( headerinfos_ ); deepErase( bodyinfos_ ); }

    ObjectSet<TargetInfo> headerinfos_;
    ObjectSet<TargetInfo> bodyinfos_;

    int			nrhdrlines_;	//!< if < 0 eohtoken will be used
    BufferString	eohtoken_;	//!< end-of-header token
    int			eohtokencol_;	//!< if < 0 eohtoken can be in any col
    BufferString	eobtoken_;	//!< end-of-body: no more data

    bool		needEOHToken() const
			{ return nrhdrlines_ < 0 && !eohtoken_.isEmpty(); }
    int			nrHdrLines() const
			{ return needEOHToken() ? mUdf(int)
			       : nrhdrlines_ > 0 ? nrhdrlines_ : 0; }
    bool		haveEOBToken() const
			{ return !eobtoken_.isEmpty(); }

    bool		isGood() const;
    bool		bodyUsesCol(int) const;

    void		fillPar(IOPar&) const;
    void		usePar(const IOPar&);

    void		clear()
			{
			    nrhdrlines_ = eohtokencol_ = 0;
			    eohtoken_.setEmpty(); eobtoken_.setEmpty();
			}
};

}; // namespace Table
