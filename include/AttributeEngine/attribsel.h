#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "attributeenginemod.h"
#include "ranges.h"
#include "multiid.h"
#include "bufstringset.h"
#include "attribdescid.h"
#include "typeset.h"

class NLAModel;
class BinDataDesc;

namespace ZDomain { class Info; }


namespace Attrib
{

class Desc;
class DescSet;

/*!
\brief Specifies an attribute selection (ID or output number of NN).

  When attrib sets and NLAs change, the IDs may be no longer valid. Thus, the
  user reference is stored, so you can try to get a valid ID in that situation.

  Object reference holds the NLA or attribute set name.
  discrSpec() specifies whether (if (0,0) interval not) and how a discrete
  output is to be expected.
*/

mExpClass(AttributeEngine) SelSpec
{
public:
			SelSpec(const char* userref=nullptr,
				DescID id=cAttribNotSel(),
				bool isnla=false,const char* objectref=nullptr);
			SelSpec(const SelSpec&);
    virtual		~SelSpec();

    const DescID&	id() const		{ return id_; }
    bool		isNLA() const		{ return isnla_; }
    bool		is2D() const		{ return is2d_; }
    const char*		userRef() const		{ return ref_; }
    const char*		objectRef() const	{ return objref_; }
    const char*		defString() const	{ return defstring_; }
    const char*		zDomainKey() const	{ return zdomainkey_; }
    const char*		zDomainUnit() const	{ return zunitstr_; }

    SelSpec&		operator=(const SelSpec&);
    bool		operator==(const SelSpec&) const;
    bool		operator!=(const SelSpec&) const;

    void		set(const Desc&);
    void		set(const NLAModel&,int);
    void		setUserRef( const char* ref )
			    { ref_ = ref; }
    void		setObjectRef( const char* objr )
			    { objref_ = objr; }
    void		setDefString( const char* def )
			    { defstring_ = def;}
    void		setZDomainKey( const char* key )
			    { zdomainkey_ = key; }
    void		setZDomainKey(const Desc&);
    void		setZDomainUnit( const char* unitstr )
			    { zunitstr_ = unitstr; }

    void		set( const char* r, const DescID& i, bool isnla,
			     const char* objr )
			{ ref_ = r; id_ = i; isnla_ = isnla; objref_ = objr;
			  defstring_ = ""; zdomainkey_ = ""; }

    void		setIDFromRef(const NLAModel&);
    void		setIDFromRef(const DescSet&);
    void		setRefFromID(const NLAModel&);
    void		setRefFromID(const DescSet&);
    void		set2DFlag( bool yn = true )	{ is2d_ = yn; }

    const StepInterval<int>&	discrSpec() const	{ return discrspec_; }
    void		setDiscrSpec( const StepInterval<int>& ds )
			{ discrspec_ = ds; }

    void		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);

    bool		isStored() const;
    bool		isZTransformed() const;

    const BinDataDesc*	getPreloadDataDesc(Pos::GeomID geomid) const;

    static const DescID& cNoAttrib();
    static const DescID& cAttribNotSel();
    static const DescID& cOtherAttrib();

protected:

    BufferString	ref_;
    BufferString	objref_;
    BufferString	defstring_;
    BufferString	zdomainkey_;
    BufferString	zunitstr_;
    DescID		id_;
    bool		isnla_			= false;
    StepInterval<int>	discrspec_;
    bool		is2d_			= false;

    static const char*	sKeyRef();
    static const char*	sKeyObjRef();
    static const char*	sKeyID();
    static const char*	sKeyIsNLA();
    static const char*	sKeyDefStr();
    static const char*	sKeyIs2D();
    static const char*	sKeyOnlyStoredData();

    void		setDiscr(const DescSet&);
    void		setDiscr(const NLAModel&);
};


/*
\brief Specifies current attribute choices (ID or output nr of NLA model).
*/

mExpClass(AttributeEngine) CurrentSel
{
public:
			CurrentSel();
    virtual		~CurrentSel();

    DescID		attrid_		= DescID(-1,true);
    MultiID		ioobjkey_;
    int			outputnr_	= -1; // For NLA or attribute nr in 2D

};


/*!
\brief Supplies lists of available attribute input.
*/

mExpClass(AttributeEngine) SelInfo
{
public:
			SelInfo(const DescSet*,const NLAModel* n=0,
				bool is2d=false,
				const DescID& ignoreid=DescID::undef(),
				bool usesteering=false,bool onlysteering=false,
				bool onlymulticomp=false, bool usehidden=false);
			SelInfo(const SelInfo&);
			~SelInfo();

    SelInfo&		operator=(const SelInfo&);

    BufferStringSet	ioobjnms_;
    TypeSet<MultiID>	ioobjids_;
    BufferStringSet	steernms_;
    TypeSet<MultiID>	steerids_;
    BufferStringSet	attrnms_;
    TypeSet<DescID>	attrids_;
    BufferStringSet	nlaoutnms_;

    void		fillStored(bool steerdata,const char* filter=0);
    static bool		is2D(const char* defstr_or_ioobjid);
    static void		getZDomainItems(const ZDomain::Info&,
					BufferStringSet& objnms);
    static void		getZDomainItems(const ZDomain::Info&,bool is2d,
					BufferStringSet& objnms);

			//!< 2D only
    static void		getAttrNames(const char* defstr,
				     BufferStringSet&,bool issteer=false,
				     bool onlymulticomp=false);
    static void		getAttrNames(const MultiID&,
				     BufferStringSet&,bool issteer=false,
				     bool onlymulticomp=false);

protected:

    bool		is2d_;
    bool		usesteering_;
    bool		onlysteering_;
    bool		onlymulticomp_;

};

} // namespace Attrib
