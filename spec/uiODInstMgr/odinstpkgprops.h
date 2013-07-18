#ifndef pkgprops_h
#define pkgprops_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          May 2010
 RCS:           $Id: odinstpkgprops.h 8009 2013-06-20 06:11:26Z kristofer.tingdahl@dgbes.com $
________________________________________________________________________

-*/

#include "uiodinstmgrmod.h"
#include "odinstpkgkey.h"
#include "odinstrel.h"
#include "bufstringset.h"
#include "separstr.h"
class IOPar;


namespace ODInst
{


mDefClass(uiODInstMgr) CreatorData
{
public:
			CreatorData( const char* nm, const char* url,
			             const char* odpg )
			: name_(nm), url_(url), odpage_(odpg)	{}

    BufferString	name_;
    BufferString	url_;
    BufferString	odpage_;

};
    
    
mDefClass(uiODInstMgr) PkgLabelData
{
public:
    			PkgLabelData( int id, const char* lbl, float sv )
			    : name_( lbl )
			    , id_( id )
			    , sortval_( sv ) {}
    int 		id_;
    BufferString	name_;
    float		sortval_;
};


/*!\brief Properties of package of a certain RelType */

mDefClass(uiODInstMgr) PkgProps
{
public:

			PkgProps()
			: internal_(false)
			, commercial_(false)
			, thirdparty_(false)
			, creator_(0)		{}

    bool		isDepOn(const PkgProps&) const;
    bool		isHidden() const	{ return internal_; }
    bool		isThirdParty() const	{ return thirdparty_; }
    bool		isPlfIndep(const Platform&) const;
    bool		isDoc() const;
    bool		isAvailableOn( const Platform& plf ) const
			{ return plfs_.indexOf(plf) >= 0; }
    PkgKey		pkgKey(const Platform&) const;
    BufferString	getURL() const;

    BufferString	pkgnm_;
    BufferString	usrname_;
    ObjectSet<const PkgLabelData> pkglabels_;
    const CreatorData*	creator_;
    bool		internal_;
    bool		commercial_;
    bool		thirdparty_;
    BufferString	webpage_;
    BufferStringSet	desc_;
    BufferString	shortdesc_;
    TypeSet<Platform>	plfs_;

    Version		ver_;
    BufferStringSet	deppkgnms_;

    const ObjectSet<const PkgProps>&	ownDeps() const	{ return deps_; }

protected:

    ObjectSet<const PkgProps>	deps_;

    void		addPlf( const Platform& plf )	{ plfs_ += plf; }
    void		getPlfsFromString(const char*);

    friend class	PkgGroupSet;

};


mDefClass(uiODInstMgr) PkgGroup : public ObjectSet<PkgProps>
{
public:

			PkgGroup( const char* nm )
			    : name_(nm)			{}
    			~PkgGroup()			{ deepErase(*this); }

    int			pkgIdx(const char*,bool usrnm) const;
    const PkgProps*	get( const char* s, bool usrnm ) const
			{ const int idx = pkgIdx(s,usrnm);
			  return idx < 0 ? 0 : (*this)[idx]; }

    BufferString	name_;

};


mDefClass(uiODInstMgr) PkgGroupSet : public ObjectSet<PkgGroup>
{
public:

			PkgGroupSet()		{}
			PkgGroupSet(const IOPar&);
    			~PkgGroupSet();
    void		set(const ObjectSet<IOPar>&,bool incl_comm=true);
    void		setVersions(const BufferStringSet&,const Platform&);

    int			groupIdx(const char*) const;
    int			pkgGroupIdx(const char*,bool usrnm) const;
    const PkgGroup*	getGroup( const char* s ) const
			{ const int idx = groupIdx(s);
			  return idx < 0 ? 0 : (*this)[idx]; }
    const PkgProps*	getPkg(const char* s,bool usrnm) const;
    const PkgLabelData*	getPkgLabel(int id) const;

    ObjectSet<CreatorData>		cd_;
    ObjectSet<PkgLabelData>		pkglabels_;

protected:

    void				determineDeps();

};


} // namespace

#endif

