#ifndef attribdescset_h
#define attribdescset_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Kristofer Tingdahl
 Date:          07-10-1999
 RCS:           $Id: attribdescset.h,v 1.45 2010-11-18 13:03:16 cvshelene Exp $
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
    				DescSet(bool is2d);
    				DescSet(const DescSet&);
    				~DescSet() 		{ removeAll( false ); }
    DescSet&			operator =(const DescSet&);
    bool			isEmpty() const	{ return descs_.isEmpty(); }

    DescSet*			optimizeClone(const DescID& targetid) const;
    DescSet*      		optimizeClone(const TypeSet<DescID>&) const;
    DescSet*      		optimizeClone(const BufferStringSet&) const;
    				/*!< Only clones stuff needed to calculate
				     the attrib with the ids given */
    void			updateInputs();
    				/*!< Updates inputs for all descs in descset.
				     Necessary after cloning */

    DescID			addDesc(Desc*,DescID newid=DescID());
				/*!< returns id of the attrib */
    
    DescID			insertDesc(Desc*,int,DescID newid=DescID());
				/*!< returns id of the attrib */

    void			createAndAddMultOutDescs(const DescID&,
	    						 const TypeSet<int>&,
							 const BufferStringSet&,
							 TypeSet<DescID>&);
				/*!< Make sure all descs needed to compute
				     attributes with multiple outputs
				     are created and added */

    int				nrDescs(bool inclstored=true,
	    				bool inclhidden=true) const;
    Desc*       		desc( int idx )		{ return descs_[idx]; }
    const Desc*       		desc( int idx ) const	{ return descs_[idx]; }
    Desc*       		getDesc( const DescID& id )
    							{ return gtDesc(id); }
    const Desc*       		getDesc( const DescID& id ) const
    							{ return gtDesc(id); }
    DescID			getID(const Desc&) const;
    DescID			getID(int) const;
    DescID			getID(const char* ref,bool isusrref,
	    			      bool isdescstored =false,
				      bool usestorinfo =false) const;
    void			getIds(TypeSet<DescID>&) const;
    void			getStoredIds(TypeSet<DescID>&) const;
    DescID			getStoredID(const char* lk,int selout=-1,
	    				    bool create=true,
					    bool blindcomp=false,
					    const char* blindcompnm=0);
    Desc* 			getFirstStored(bool usesteering=true) const;

    void			removeDesc(const DescID&);
    void                        moveDescUpDown(const DescID&,bool);
    void                        sortDescSet();
    void			removeAll(bool kpdefault);
    int                 	removeUnused(bool removestored=false,
	    				     bool kpdefault=true);
				//!< Removes unused hidden attributes.
				//!< Removed stored attribs if not available
				//!< or if removestored flag is true;
				//!< Returns total removed.
    bool 			isAttribUsed(const DescID&) const;

    void			fillPar(IOPar&) const;
    bool			usePar(const IOPar&,float,
	    			       BufferStringSet* errmsgs=0);
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
    void 			handleOldAttributes(BufferString&,IOPar&,
						    BufferString&,float) const;
    void			handleOldMathExpression(IOPar&,
	    						BufferString&) const;
    void                        handleReferenceInput(Desc*);

    				//!<will prepare strings for each desc, format :
    				//!<DescID`definition string
    void			fillInAttribColRefs(BufferStringSet&) const;

    void			setContainStoredDescOnly(bool yn);
    inline bool			containsStoredDescOnly() const
    				{ return storedattronly_; }

    inline bool			is2D() const		{ return is2d_; }
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
    bool			is2d_;
    bool			storedattronly_;
    BufferString		errmsg_;
    mutable BufferString	defidstr_;
    mutable DescID		defattribid_;

private:

    Desc*			gtDesc(const DescID&) const;

public:

    DescID			ensureDefStoredPresent() const;

};

} // namespace Attrib

#endif

