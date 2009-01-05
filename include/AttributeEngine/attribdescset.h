#ifndef attribdescset_h
#define attribdescset_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Kristofer Tingdahl
 Date:          07-10-1999
 RCS:           $Id: attribdescset.h,v 1.34 2009-01-05 09:49:43 cvsranojay Exp $
________________________________________________________________________

-*/

#include "callback.h"
#include "sets.h"
#include "multiid.h"
#include "attribdescid.h"

class BufferStringSet;
class IOPar;

namespace Attrib 
{
class Desc;

mClass DescSet : public CallBacker
{
public:
    				DescSet( bool is2d )
				    : is2d_(is2d)
			    	    , is2dset_(true)
			    	    , descToBeRemoved(this)   {}
    				DescSet( bool is2d, bool is2dset )
				    : is2d_(is2d)
			    	    , is2dset_(is2dset)
			    	    , descToBeRemoved(this)  {}
    				~DescSet() 		{ removeAll(); }

    DescSet*			clone() const;
    DescSet*			optimizeClone(const DescID& targetid) const;
    DescSet*      		optimizeClone(const TypeSet<DescID>&) const;
    DescSet*      		optimizeClone(const BufferStringSet&) const;
    				/*!< Only clones stuff needed to calculate
				     the attrib with the ids given */
    void			updateInputs();
    				/*!< Updates inputs for all descs in descset.
				     Necessary after cloning */

    DescID			addDesc(Desc*,DescID newid=DescID(-1,true));
				/*!< returns id of the attrib */
    
    DescID			insertDesc(Desc*,int,
	    				   DescID newid=DescID(-1,true));
				/*!< returns id of the attrib */

    int				nrDescs(bool inclstored=true,
	    				bool inclhidden=true) const;
    Desc*       		desc( int idx )		{ return descs_[idx]; }
    const Desc*       		desc( int idx ) const	{ return descs_[idx]; }
    Desc*       		getDesc(const DescID&);
    const Desc*			getDesc(const DescID&) const;
    DescID			getID(const Desc&) const;
    DescID			getID(int) const;
    DescID			getID(const char* ref,bool isusrref) const;
    void			getIds(TypeSet<DescID>&) const;
    void			getStoredIds(TypeSet<DescID>&) const;
    DescID			getStoredID(const char* lk,int selout=-1,
	    				    bool create=true);
    Desc* 			getFirstStored(bool usesteering=true) const;

    void			removeDesc(const DescID&);
    void                        moveDescUpDown(const DescID&,bool);
    void                        sortDescSet();
    void			removeAll();
    int                 	removeUnused(bool removestored=false);
				//!< Removes unused hidden attributes.
				//!< Removed stored attribs if not available
				//!< or if removestored flag is true;
				//!< Returns total removed.
    bool 			isAttribUsed(const DescID&) const;

    void			fillPar(IOPar&) const;
    bool			usePar(const IOPar&,BufferStringSet* errmsgs=0);
    bool 			useOldSteeringPar(IOPar&,ObjectSet<Desc>&,
	    					  BufferStringSet*);
    bool			createSteeringDesc(const IOPar&,BufferString,
						   ObjectSet<Desc>&, int& id, 
						   BufferStringSet* errmsgs=0);
    static Desc*		createDesc(const BufferString&, const IOPar&,
	    				  const BufferString&,BufferStringSet*);
    Desc* 			createDesc(const BufferString&, const IOPar&,
	    				   const BufferString& );
    DescID			createStoredDesc(const char*,int,
	    					 const BufferString&);
    bool 			setAllInputDescs(int, const IOPar&,
						 BufferStringSet*);
    void 			handleStorageOldFormat(IOPar&);
    void 			handleOldAttributes(BufferString&, IOPar&,
						    BufferString&);
    void                        handleReferenceInput(Desc*);

    				//!<will prepare strings for each desc, format :
    				//!<DescID`definition string
    void			fillInAttribColRefs(BufferStringSet&) const;

    bool                        is2DSet() const         { return is2dset_; }
    bool			is2D() const;
    const char*			errMsg() const;
    static const char*		highestIDStr()		{ return "MaxNrKeys"; }
    static const char*		definitionStr()		{ return "Definition"; }
    static const char*		userRefStr()		{ return "UserRef"; }
    static const char*		inputPrefixStr()	{ return "Input"; }
    static const char*		hiddenStr()		{ return "Hidden"; }

    CNotifier<DescSet,DescID>	descToBeRemoved;

protected:

    DescID			getFreeID() const;

    ObjectSet<Desc>		descs_;
    TypeSet<DescID>		ids_;
    BufferString		errmsg_;
    bool			is2d_;

private:

    bool			is2dset_;
    				DescSet( const DescSet& )
				    : descToBeRemoved(this) {}
				// crap. Don't even TRY to use. Use clone()!
};

} // namespace Attrib

#endif

