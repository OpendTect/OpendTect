#ifndef attriblinebuffer_h
#define attriblinebuffer_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Kristofer Tingdahl
 Date:          07-10-1999
 RCS:           $Id: attriblinebuffer.h,v 1.9 2010/08/04 14:49:36 cvsbert Exp $
________________________________________________________________________

-*/

#include "sets.h"

class BinID;


namespace Attrib
{

class DataHolder;

mClass DataHolderLineBuffer
{
public:
    			DataHolderLineBuffer();
			~DataHolderLineBuffer();

    DataHolder*		createDataHolder( const BinID&, int t0, int nrsamples );
    void		removeDataHolder(const BinID&);
    void		removeBefore( const BinID&, const BinID& );
    void		removeAllExcept( const BinID& );

    DataHolder*		getDataHolder( const BinID& b )
			{ return gtDataHolder(b); }
    const DataHolder*	getDataHolder( const BinID& b ) const
			{ return gtDataHolder(b); }

protected:
    void		removeInline( int lineidx );

    TypeSet<int>			inlines_;	
    ObjectSet<ObjectSet<DataHolder> >	inlinedata_;
    ObjectSet<TypeSet<int> >		crossliness_;
    DataHolder*		gtDataHolder(const BinID&) const;
};

}; //Namespace


#endif
