#ifndef attribdescset_h
#define attribdescset_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Kristofer Tingdahl
 Date:          07-10-1999
 RCS:           $Id: attribdescset.h,v 1.1 2005-01-26 09:15:22 kristofer Exp $
________________________________________________________________________

-*/

#include "sets.h"

class IOPar;

namespace Attrib 
{
class Desc;

class DescSet
{
public:
    DescSet*      clone() const;

    int		addDesc( Desc* );
		/*!<\returns id of the attrib */

    Desc*         getDesc(int id);
    const Desc*   getDesc(int id) const;

    int		getID(const Desc&) const;

    void	getIds( TypeSet<int>& ) const;

    void	removeDesc(int id);

    void	fillPar( const IOPar& );
    bool	usePar( const IOPar& );


protected:
    int			getFreeID() const;

    ObjectSet<Desc>	descs;
    TypeSet<int>	ids;
};

}; //Namespace

#endif

