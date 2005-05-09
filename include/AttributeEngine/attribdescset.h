#ifndef attribdescset_h
#define attribdescset_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Kristofer Tingdahl
 Date:          07-10-1999
 RCS:           $Id: attribdescset.h,v 1.3 2005-05-09 14:40:01 cvshelene Exp $
________________________________________________________________________

-*/

#include "sets.h"

class BufferStringSet;
class IOPar;

namespace Attrib 
{
class Desc;

class DescSet
{
public:
    		~DescSet() { removeAll(); }
    DescSet*    clone() const;

    int		addDesc( Desc* );
		/*!<\returns id of the attrib */

    Desc*       getDesc(int id);
    const Desc* getDesc(int id) const;

    int		nrDescs() const;
    int		getID(const Desc&) const;
    void	getIds( TypeSet<int>& ) const;

    void	removeDesc(int id);
    void	removeAll();

    void	fillPar( IOPar& ) const;
    bool	usePar( const IOPar&, BufferStringSet* errmsgs = 0 );

    const ObjectSet<Desc>	getDescSet() const {return descs;}
    const char*         	getStoredID() { return storedid_; }

    const char*	errMsg() const;

protected:
    int			getFreeID() const;
    void		setStoredID( const BufferString& storedid ) 
    					{ storedid_ = storedid.buf(); }
    static const char*	highestIDStr() { return "MaxNrKeys"; }
    static const char*	definitionStr() { return "Definition"; }
    static const char*	userRefStr() { return "UserRef"; }
    static const char*	inputPrefixStr() { return "Input."; }



    ObjectSet<Desc>	descs;
    TypeSet<int>	ids;
    BufferString	errmsg;
    BufferString 	storedid_;
};

}; //Namespace

#endif

