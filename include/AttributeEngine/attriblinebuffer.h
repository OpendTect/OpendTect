#ifndef attribbuffer_h
#define attribbuffer_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Kristofer Tingdahl
 Date:          07-10-1999
 RCS:           $Id: attriblinebuffer.h,v 1.3 2005-07-28 10:53:49 cvshelene Exp $
________________________________________________________________________

-*/

#include "sets.h"

class BinID;


namespace Attrib
{

class DataHolder;

class DataHolderLineBuffer
{
public:
    			DataHolderLineBuffer();
			~DataHolderLineBuffer();

    DataHolder*		createDataHolder( const BinID&, int t0, int nrsamples );
    DataHolder*		getDataHolder(const BinID&) const;
    void		removeDataHolder(const BinID&);
    void		removeBefore( const BinID&, const BinID& );

protected:
    void		removeInline( int lineidx );

    TypeSet<int>			inlines;	
    ObjectSet<ObjectSet<DataHolder> >	inlinedata;
    ObjectSet<TypeSet<int> >		crossliness;
};

}; //Namespace


#endif
