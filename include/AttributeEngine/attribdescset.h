#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Kristofer Tingdahl
 Date:          07-10-1999
________________________________________________________________________

-*/

#include "attribdescid.h"
#include "notify.h"
#include "dbkey.h"
#include "uistring.h"

class BufferStringSet;
class DataPointSet;
class CtxtIOObj;
class IOObjContext;

namespace Attrib
{

class DescSetup;
class SelSpec;
class SelSpecList;
class DescSet_Standard_Manager;


/*!\brief Set of attribute descriptions.

  Every Attrib::Desc has a DescID, and ID that has no meaning outside the
  DescSet.

  For both 2D and 3D there is one global DescSet that is shared.

 */

mExpClass(AttributeEngine) DescSet : public CallBacker
{ mODTextTranslationClass( Attrib::DescSet )
public:

    static const DescSet&	global2D();
    static const DescSet&	global3D();
    static const DescSet&	empty2D();
    static const DescSet&	empty3D();
    static DescSet&		dummy2D();
    static DescSet&		dummy3D();

    inline static const DescSet& global( bool is2d )
			{ return is2d ? global2D() : global3D(); }
    inline static const DescSet& empty( bool is2d )
			{ return is2d ? empty2D() : empty3D(); }
    inline static const DescSet& dummy( bool is2d )
			{ return is2d ? dummy2D() : dummy3D(); }
    static bool		globalUsed(bool);

    explicit		DescSet(bool is2d);
			DescSet(const DescSet&);
			~DescSet();
    DescSet&		operator =(const DescSet&);
    uiRetVal		load(const DBKey&,uiRetVal* warns=0);
    uiRetVal		store(const DBKey&) const;
    uiRetVal		save() const		{ return store( dbky_ ); }
    DBKey		storeID() const		{ return dbky_; }
    BufferString	name() const;

    inline bool		isEmpty() const		{ return descs_.isEmpty(); }
    inline int		size() const		{ return descs_.size(); }
    int			indexOf(const char* nm,bool usrref=true) const;
    inline bool		isPresent( const char* nm, bool usrref=true ) const
						{ return indexOf(nm,usrref)>=0;}
    int			indexOf( const DescID& id ) const
						{ return ids_.indexOf( id ); }

    Desc*		desc( int idx )		{ return descs_[idx]; }
    const Desc*		desc( int idx ) const	{ return descs_[idx]; }

    DescSet*		optimizeClone(const DescID& targetid) const;
    DescSet*		optimizeClone(const TypeSet<DescID>&) const;
    DescSet*		optimizeClone(const BufferStringSet&) const;
			/*!< Only clones stuff needed to calculate
			     the attrib with the ids given */
    void		updateInputs();
			/*!< Updates inputs for all descs in descset.
			     Necessary after cloning */

    void		setEmpty();
    DescID		addDesc(Desc*,DescID newid=DescID());
    DescID		insertDesc(Desc*,int,DescID newid=DescID());
    void		createAndAddMultOutDescs(const DescID&,
						 const TypeSet<int>&,
						 const BufferStringSet&,
						 TypeSet<DescID>&);
			/*!< Make sure all descs needed to compute
			     attributes with multiple outputs
			     are created and added */

    Desc&		get( int idx )		{ return *desc(idx); }
    const Desc&		get( int idx ) const	{ return *desc(idx); }
    Desc&		operator[]( int idx )	{ return get(idx); }
    const Desc&		operator[]( int idx ) const { return get(idx); }
    int			nrDescs(bool inclstored=false,
				bool inclhidden=false) const;

    Desc*		getDesc( const DescID& id )
					{ return gtDesc(id); }
    const Desc*		getDesc( const DescID& id ) const
					{ return gtDesc(id); }
    static const Desc*	getGlobalDesc( bool is2d, const DescID& id )
					{ return global(is2d).getDesc( id ); }
    static const Desc*	getGlobalDesc(const SelSpec&);
    DescID		getID(const Desc&) const;
    DescID		getID(int) const;
    DescID		getID(const char* ref,bool isusrref,
			      bool mustbestored=false,
			      bool usestorinfo=false) const;
    DescID		getDefaultTargetID() const;
    void		getIds(TypeSet<DescID>&) const;
    void		getStoredIds(TypeSet<DescID>&) const;
    DescID		getStoredID(const DBKey&,int selout=-1,
				    bool add_if_absent=true,
				    bool blindcomp=false,
				    const char* blindcompnm=0) const;
    DescID		defStoredID() const;
    Desc*		getFirstStored(bool usesteering=true) const;
    bool		hasTrueAttribute() const;
    DBKey		getStoredKey(const DescID&) const;
    void		getStoredNames(BufferStringSet&) const;
    void		getAttribNames(BufferStringSet&,bool inclhidden) const;

    void		removeDesc(const DescID&);
    void		moveDescUpDown(const DescID&,bool);
    void		sortDescSet();
    int			removeUnused(bool removestored=false,
				     bool kpdefault=true);
			//!< Removes unused hidden attributes, stored attribs
			//!< if not available or if removestored flag is true.
			//!< Returns total removed.
    bool		isAttribUsed(const DescID&,BufferString&) const;
    bool		isAttribUsed(const DescID&) const;
    void		cleanUpDescsMissingInputs();

    uiRetVal		createSteeringDesc(const IOPar&,BufferString,
					   ObjectSet<Desc>&, int& id);
    static Desc*	createDesc(const BufferString&,const IOPar&,
				   const BufferString&,uiRetVal&);
    DescID		createStoredDesc(const DBKey&,int selout,
					 const BufferString& compnm);
    uiRetVal		setAllInputDescs(int, const IOPar&);
    void		handleStorageOldFormat(IOPar&);
    uiRetVal		handleOldAttributes(BufferString&,IOPar&,BufferString&,
					    int) const;
    uiRetVal		handleOldMathExpression(IOPar&,BufferString&,int) const;
    void		handleReferenceInput(Desc*);

			//!<will prepare strings for each desc, format :
			//!<DescID`definition string
    void		fillInAttribColRefs(BufferStringSet&) const;

			//!<will prepare strings for each desc, format :
			//!<Attrib,[stored], [{prestack}]
    void		fillInUIInputList(BufferStringSet&) const;
			//!<Counterpart: will decode the UI string
			//!<and return the corresponding Desc*
    Desc*		getDescFromUIListEntry(const StringPair&);

			//!<will create an empty DataPointSet
    DataPointSet*	createDataPointSet(DescSetup,
					   bool withstored=true) const;
    void		fillInSelSpecs(DescSetup,SelSpecList&) const;

    inline bool		is2D() const			{ return is2d_; }
    bool		hasStoredInMem() const;
    bool		needsSave() const		{ return ischanged_; }
    void		setSaved( bool yn=true ) const	{ ischanged_ = yn; }

    bool		exportToDot(const char* nm,const char* fnm) const;

    static const char*	highestIDStr()		{ return "MaxNrKeys"; }
    static const char*	definitionStr()		{ return "Definition"; }
    static const char*	userRefStr()		{ return "UserRef"; }
    static const char*	inputPrefixStr()	{ return "Input"; }
    static const char*	hiddenStr()		{ return "Hidden"; }
    static const char*	indexStr()		{ return "Index"; }

    CNotifier<DescSet,DescID>	descAdded;
    CNotifier<DescSet,DescID>	descUserRefChanged;
    CNotifier<DescSet,DescID>	descToBeRemoved;
    CNotifier<DescSet,DescID>	descRemoved;
    Notifier<DescSet>		aboutToBeDeleted;

    void		fillPar(IOPar&) const;
    uiRetVal		usePar(const IOPar&);
    uiRetVal		useOldSteeringPar(IOPar&,ObjectSet<Desc>&);

    static const char*	sKeyUseAutoAttrSet;
    static const char*	sKeyAuto2DAttrSetID;
    static const char*	sKeyAuto3DAttrSetID;

protected:

    DescID		getFreeID() const;

    const bool		is2d_;
    mutable bool	ischanged_;
    ObjectSet<Desc>	descs_;
    TypeSet<DescID>	ids_;
    DBKey		dbky_;

    void		usrRefChgCB(CallBacker*);

private:

    Desc*		gtDesc(const DescID&) const;
    friend class	DescSet_Standard_Manager;

public:

			// No detailed change management. Normally maintained
			// by UI only. If you change the global DescSet be
			// sure to call setIsChanged( true ).
    bool		isChanged() const		{ return ischanged_; }
    void		setIsChanged( bool yn ) const	{ ischanged_ = yn; }
    void		setStoreID( const DBKey& dbky )	{ dbky_ = dbky; }
    static DescSet&	global2D4Edit();
    static DescSet&	global3D4Edit();
    inline static DescSet& global4Edit( bool is2d )
			{ return is2d ? global2D4Edit() : global3D4Edit(); }
    static void		pushGlobal(bool,DescSet*);
    static DescSet*	popGlobal(bool);
    static void		initGlobalSet(bool);
    static const uiRetVal& autoLoadResult();

    DescID		ensureStoredPresent(const DBKey&,int compnr=-1) const;
    DescID		ensureDefStoredPresent() const;
    static uiString	sFactoryEntryNotFound(const char* attrnm);
    uiRetVal		load(const char* filenm,uiRetVal* warns=0);
    CtxtIOObj*		getCtxtIOObj(bool forread) const;
    static IOObjContext* getIOObjContext(bool forread);
    static IOObjContext* getIOObjContext(bool forread,bool is2d);

};

} // namespace Attrib
