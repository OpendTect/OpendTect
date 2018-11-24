#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Sep 2001
________________________________________________________________________

-*/

#include "attribdescid.h"
#include "ranges.h"
#include "dbkey.h"
#include "geomid.h"
#include "bufstringset.h"
#include "typeset.h"
#include "datapack.h"

class NLAModel;
class BinDataDesc;
namespace ZDomain { class Info; }


namespace Attrib
{

/*!\brief specifies a full attribute selection (ID or output number of NN).

  When attrib sets and NLAs change, the IDs may be no longer valid. Thus, the
  user reference is stored, so you can try to get a valid ID in that situation.

  Object reference holds the NLA or attribute set name.
  discrSpec() specifies whether (if (0,0) interval not) and how a discrete
  output is to be expected.
*/

mExpClass(AttributeEngine) SelSpec
{
public:

			SelSpec( const char* r=0, DescID i=cAttribNotSelID(),
				 bool n=false, const char* objr=0 )
			: ref_(r), id_(i), isnla_(n)
			, objref_(objr)
			, is2d_(false)
			, defstring_(0)		{}

    const DescID&	id() const		{ return id_; }
    bool		isNLA() const		{ return isnla_; }
    bool		is2D() const		{ return is2d_; }
    const char*		userRef() const		{ return ref_; }
    const char*		objectRef() const	{ return objref_; }
    const char*		defString() const	{ return defstring_; }
    const char*		zDomainKey() const	{ return zdomainkey_; }

    bool		operator==(const SelSpec&) const;
    bool		operator!=(const SelSpec&) const;
    bool		isUsable() const;

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

    void		set( const char* r, DescID i, bool isnla,
			     const char* objr )
			{ ref_ = r; id_ = i; isnla_ = isnla; objref_ = objr;
		          defstring_ = ""; zdomainkey_ = ""; }

    void		set2D( bool yn = true )		{ is2d_ = yn; }
    void		setIDFromRef(const NLAModel&);
    void		setIDFromRef(const DescSet&);
    void		setRefFromID(const NLAModel&);
    void		setRefFromID(const DescSet&);

    const StepInterval<int>&	discrSpec() const	{ return discrspec_; }
    void		setDiscrSpec( const StepInterval<int>& ds )
			{ discrspec_ = ds; }

    void		fillPar(IOPar&) const;
    void		usePar(const IOPar&);

    bool		isStored(const DescSet*) const;
    bool		isZTransformed() const;

    const Desc*		getDesc(const DescSet* descset=0) const;
    const BinDataDesc*	getPreloadDataDesc(
				Pos::GeomID geomid=Pos::GeomID::get3D(),
				const DescSet* descset=0) const;

    static DescID	cNoAttribID()		{ return DescID(-1); }
    static DescID	cAttribNotSelID()	{ return DescID(-2); }
    static DescID	cOtherAttribID()	{ return DescID(-3); }
    static DescID	cExternalAttribID()	{ return DescID(-4); }

protected:

    BufferString	ref_;
    BufferString	objref_;
    BufferString	defstring_;
    BufferString	zdomainkey_;
    DescID		id_;
    bool		isnla_;
    StepInterval<int>	discrspec_;
    bool		is2d_;

    static const char*	sKeyRef();
    static const char*	sKeyObjRef();
    static const char*	sKeyID();
    static const char*	sKeyIsNLA();
    static const char*	sKeyDefStr();
    static const char*	sKeyIs2D();

    void		setDiscr(const DescSet&);
    void		setDiscr(const NLAModel&);
};


/*!\brief An ordered list of SelSpec's */

mExpClass(AttributeEngine) SelSpecList : public TypeSet<SelSpec>
{
public:
				SelSpecList()			{}
				SelSpecList( int nr, const SelSpec& ss )
				    : TypeSet<SelSpec>(nr,ss)	{}

    inline bool	is2D() const	{ return !isEmpty() && first().is2D(); }

};


/*!\brief All info needed to make attribute selections.

  On construction, the data is filled. If you need to re-read, fill with
  DataPack's (for synthetics, will discard 'stored'), or (re-)filter you can
  use the fillXX() functions.

 */

mExpClass(AttributeEngine) SelInfo
{
public:

			SelInfo(const DescSet&,
				const DescID& ignoreid=DescID(),
				const NLAModel* n=0,
				const ZDomain::Info* zi=0);
			SelInfo(const DescSet*,const DescID& ignoreid,
				const NLAModel* n=0,
				bool is2d=false,
				bool onlymulticomp=false, bool usehidden=false);
			SelInfo(const ZDomain::Info&,bool is2d);
				// Only stored

    BufferStringSet	ioobjnms_;
    DBKeySet		ioobjids_;
    BufferStringSet	steernms_;
    DBKeySet		steerids_;
    BufferStringSet	attrnms_;
    TypeSet<DescID>	attrids_;
    BufferStringSet	nlaoutnms_;

    void		fillStored(bool steerdata,const char* filter=0,
				    const ZDomain::Info* zi=0);
    void		fillSynthetic(bool steerdata,
				      const TypeSet<DataPack::FullID>&);
    void		fillNonStored(const DescSet&,const DescID& ignoreid,
				      const NLAModel*,bool usehidden=false);

			//!< 2D only
    static void		getAttrNames(const char* defstr_or_ioobjid,
				     BufferStringSet&,bool issteer=false,
				     bool onlymulticomp=false);

protected:

    bool		is2d_;
    bool		onlymulticomp_;

};


} // namespace Attrib
