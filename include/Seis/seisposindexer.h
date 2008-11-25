#ifndef seisposindexer_h
#define seisposindexer_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Nov 2008
 RCS:           $Id: seisposindexer.h,v 1.1 2008-11-25 16:40:14 cvsbert Exp $
________________________________________________________________________


-*/

#include "seisposkey.h"
#include "sets.h"

namespace Seis
{

class PosKeyList
{
public:

    virtual od_uint64	size() const			= 0;
    virtual PosKey	key(od_int64) const		= 0;

};

/*!\brief builds an index of a list of positions, making it easy to find a
  specific position. */

class PosIndexer
{
public:

				PosIndexer(const PosKeyList&);
    virtual			~PosIndexer();

    od_int64			indexOf(const PosKey&,int offsnr=-1) const;
    				//!< -1 = inl not found
   				//!< -2 crl/trcnr not found
   				//!< -3 offset not found
    				//!< offsnr 0, 1, 2 overrules poskey's offset

    void			reIndex();

protected:

    const PosKeyList&		pkl_;
    bool			is2d_;
    bool			isps_;
    TypeSet<int>		inls_;
    ObjectSet< TypeSet<int> >	crlsets_;
    ObjectSet< ObjectSet< TypeSet<float> > >	offssets_;
    ObjectSet< ObjectSet< TypeSet<od_int64> > >	idxsets_;

    void			empty();
    void			add(const PosKey&,od_int64);
};



} // namespace

#endif
