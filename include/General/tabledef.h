#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "generalmod.h"

#include "coordsystem.h"
#include "datainpspec.h"
#include "mnemonics.h"
#include "namedobj.h"
#include "rowcol.h"
#include "sets.h"

class UnitOfMeasure;

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

 <pre>
 Table::TargetInfo sampinfspec( "Sampling info", form, Table::Required,
				Mnemonic::surveyZType() );
 TargetInfo::Form* form = new TargetInfo::Form( "Start/Stop", FloatInpSpec() );
 form->add( FloatInpSpec() );
 sampinfspec.add( form->duplicate( "Start/Width" ) );
 </pre>

*/

    struct Form : NamedObject
    {
			Form( const char* nm, DataInpSpec* spec )
			    : NamedObject(nm)
				{ add( spec ); }
			Form( const char* nm, const DataInpSpec& spec )
			    : NamedObject(nm)
				{ add( spec.clone() ); }

			~Form()	{ deepErase(specs_); }

	Form&		add( const DataInpSpec& spec )
			    { specs_ += spec.clone(); return *this; }
	Form&		add( DataInpSpec* spec )
			    { specs_ += spec ? spec : new StringInpSpec;
			      return *this; }

	Form*		duplicate( const char* nm ) const
			{
			    Form* ret = new Form( nm, *specs_[0] );
			    for ( int idx=1; idx<specs_.size(); idx++ )
				ret->specs_ += specs_[idx]->clone();
			    return ret;
			}

	ObjectSet<DataInpSpec>	specs_;
    };


			TargetInfo( const char* nm, ReqSpec rs=Optional )
			    : NamedObject(nm), req_(rs)
			    , proptype_(Mnemonic::Other)
				{ add( nm ); }
			TargetInfo( const char* nm, DataInpSpec* spec,
				  ReqSpec rs=Optional,
				  Mnemonic::StdType p=Mnemonic::Other )
			    : NamedObject(nm), req_(rs), proptype_(p)
				{ add( nm, spec ); }
			TargetInfo( const char* nm, const DataInpSpec& spec,
				  ReqSpec rs=Optional,
				  Mnemonic::StdType p=Mnemonic::Other )
			    : NamedObject(nm), req_(rs), proptype_(p)
				{ add( nm, spec ); }

			~TargetInfo()		{ deepErase( forms_ ); }

    TargetInfo&		add( const char* nm, DataInpSpec* spec=0 )
			    { forms_ += new Form( nm, spec ); return *this; }
    TargetInfo&		add( const char* nm, const DataInpSpec& spec )
			    { forms_ += new Form( nm, spec ); return *this; }
    TargetInfo&		add( Form* frm )
			    { forms_ += frm; return *this; }

    bool		isOptional() const	{ return req_ != Required; }
    bool		isHidden() const	{ return req_ == Hidden; }
    Mnemonic::StdType	propertyType() const	{ return proptype_; }
    void		setPropertyType( Mnemonic::StdType p )
			    { proptype_ = p; }
    int			nrForms() const		{ return forms_.size(); }
    Form&		form( int idx )		{ return *forms_[idx]; }
    const Form&		form( int idx ) const	{ return *forms_[idx]; }
    int			formNr( const char* formnm ) const
			{
			    for ( int idx=0; idx<forms_.size(); idx++ )
				if ( forms_[idx]->name() == formnm )
				    return idx;
			    return -1;
			}

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

	int			form_				= 0;
	TypeSet<Elem>		elems_;
	const UnitOfMeasure*	unit_				= nullptr;
	ConstRefMan<Coords::CoordSystem>	coordsys_	= nullptr;
	ConstRefMan<Coords::CoordSystem>	llsys_		= nullptr;

			Selection()	{}

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
				      bool wcrs=true,
				      ConstRefMan<Coords::CoordSystem> crs=
				      nullptr);
			//!< form(0)=(X,Y), form(1)=inl/crl, form(1/2)=long/lat
    static TargetInfo*	mk2DHorPosition(bool isreq,
				ConstRefMan<Coords::CoordSystem> crs=
				nullptr);
    static TargetInfo*	mk2DHorPosition(bool isreq,bool withsp=false,
					bool withll=false,
				ConstRefMan<Coords::CoordSystem> crs=
				nullptr);
    static TargetInfo*	mkZPosition( bool isreq, bool withunits=true )
				{ return mkZPos(isreq,withunits); }
    static TargetInfo*	mkDepthPosition( bool isreq, bool withunits=true )
				{ return mkZPos(isreq,withunits,1); }
    static TargetInfo*	mkTimePosition( bool isreq, bool withunits=true )
				{ return mkZPos(isreq,withunits,-1); }
    bool		needsConversion() const;
    Coord		convert(const Coord&) const;

    static const char*	sKeyXY();
    static const char*	sKeyInlCrl();
    static const char*	sKeyLatLong();

protected:

    ReqSpec		req_;
    Mnemonic::StdType	proptype_;
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
			    , eohtokencol_(-1)		{ init(); }
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

private:
    void		init();
};

} // namespace Table
