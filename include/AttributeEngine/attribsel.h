#ifndef attribsel_h
#define attribsel_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert Bril
 Date:          Sep 2001
 RCS:           $Id: attribsel.h,v 1.1 2005-05-20 15:39:03 cvsnanne Exp $
________________________________________________________________________

-*/

#include "ranges.h"
#include "multiid.h"
#include "bufstringset.h"

class IOPar;
class NLAModel;

/*!\brief specifies an attribute selection (ID or output number of NN).

  When attrib sets and NLAs change, the IDs may be no longer valid. Thus, the
  user reference is stored, so you can try to get a valid ID in that situation.

  Object reference holds the NLA or attribute set name.
  discrSpec() specifies whether (if (0,0) interval not) and how a discrete
  output is to be expected.
 
 */

namespace Attrib 
{

class Desc;
class DescSet;

class SelSpec
{
public:
			SelSpec( const char* r=0,
				 int i=attribNotSel, bool n=false,
				 const char* objr=0 )
			: ref_(r), id_(i), isnla_(n)
			, objref_(objr)			{}

    int			id() const			{ return id_; }
    bool		isNLA() const			{ return isnla_; }
    const char*		userRef() const			{ return ref_; }
    const char*		objectRef() const		{ return objref_; }

    bool		operator==( const SelSpec& ) const;
    bool		operator!=( const SelSpec& ) const;


    void		set(const Desc&);
    void		set(const NLAModel&,int);
    void		setObjectRef(const char* objr)	{ objref_ = objr; }
    void		set( const char* r, int i, bool isnla, const char* objr)
			{ ref_ = r; id_ = i; isnla_ = isnla; objref_ = objr; }

    void		setIDFromRef(const NLAModel&);
    void		setIDFromRef(const DescSet&);
    void		setRefFromID(const NLAModel&);
    void		setRefFromID(const DescSet&);

    const StepInterval<int>&	discrSpec() const	{ return discrspec_; }
    void		setDiscrSpec( const StepInterval<int>& ds )
			{ discrspec_ = ds; }

    void		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);

    static const int	noAttrib;
    static const int	attribNotSel;
    static const int	otherAttrib;

protected:

    BufferString	ref_;
    BufferString	objref_;
    int			id_;
    bool		isnla_;
    StepInterval<int>	discrspec_;

    static const char*	refstr;
    static const char*	objrefstr;
    static const char*	idstr;
    static const char*	isnlastr;

    void		setDiscr(const DescSet&);
    void		setDiscr(const NLAModel&);
};


/*!\brief specifies the attribute selection for colortable specification and
	the property selected (e.g. transparency,whiteness, etc. */

class ColorSelSpec
{
public:
			ColorSelSpec(const char* r=0,int i=-2,bool n=false,
				     const char* objr=0)
			: as(r,i,n,objr) 
			, datatype(0)
			, reverse(false)
			, useclip(true)
			, range(Interval<float>(0,0))
			, cliprate0(0.025)
			, cliprate1(0.025) {}

    SelSpec		as;
    int			datatype;

    float		cliprate0;
    float 		cliprate1;
    Interval<float>	range;
    bool		useclip;
    bool		reverse;

    void		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);

protected:
    static const char*	refstr;
    static const char*	idstr;
    static const char*	isnlastr;
    static const char*	datatypestr;
};



/*!\brief specifies current attribute choices (ID or output nr of NLA model). */

class CurrentSel
{
public:
			CurrentSel()
			: attrid(-1), outputnr(-1)	{}

    int			attrid;
    MultiID		ioobjkey;
    int			outputnr; // For NLA or attribute nr in 2D

};


/*!\brief supplies lists of available attribute input */

class SelInfo
{
public:

			SelInfo(const DescSet*,const NLAModel* n=0,
				      Pol2D pol=Both2DAnd3D,int ignoreid=-1);
			SelInfo(const SelInfo&);
    SelInfo&		operator=(const SelInfo&);

    BufferStringSet	ioobjnms;
    BufferStringSet	ioobjids;
    BufferStringSet	attrnms;
    TypeSet<int>	attrids;
    BufferStringSet	nlaoutnms;

    static bool		is2D(const char* defstr_or_ioobjid);
    static void		getAttrNames(const char* defstr_or_,BufferStringSet&);
    			//!< 2D only
    void		fillStored(const char* filter=0);

protected:

    Pol2D		pol2d_;

};

}; // namespace Attrib

#endif
