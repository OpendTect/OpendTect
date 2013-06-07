#ifndef multidimstorage_h
#define multidimstorage_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K.Tingdahl
 Date:		Jan 2006
 RCS:		$Id$
________________________________________________________________________

-*/

#include "idxable.h"
#include "ranges.h"
#include "sets.h"
#include "errh.h"
#include <limits.h>

/*! Stores one or more values of type T that are associated with a discrete
position in a N dimenstional space. */


template <class T>
class MultiDimStorage
{
public:
				MultiDimStorage(int ndims,int nvals);
				MultiDimStorage(const MultiDimStorage<T>&);
    virtual			~MultiDimStorage();
    bool			isOK() const;
    MultiDimStorage<T>&		operator=(const MultiDimStorage<T>&);

    bool			getRange(int dim,Interval<int>&) const;
    int				nrDims() const;
    int				nrVals() const;
    void			setNrVals(int,bool keepdata);
    void			removeValue(int);

    bool			allowsDuplicates() const;
    				/*!<Set allowance for allowing multiple
				    instances at the same position. */
    bool			allowDuplicates(bool);
				/*!<\returns oldstatus. */

    bool			isEmpty() const;
    int				totalSize() const;
    				/*!<\returns the total number of instances. */
    int				size() const;
    				//!<\returns the size in highest dimension.
    MultiDimStorage<T>*		operator[](int);
    				//!<\returns zero if nrdims==1
    const MultiDimStorage<T>*	operator[](int) const;
    				//!<\returns zero if nrdims==1
    int				indexOf(int pos) const;
    				//!<\returns index in highest dimension
    int				getPos(int idx) const;
    				//!<\returns position in highest dimension


    template <class IDX> const T&	getRef(const IDX& index,int val) const;
    					/*!<\returns a reference to the value
					     poited out by index and val. */
    template <class IDX> T&		getRef(const IDX& index,int val);
    					/*!<\returns a reference to the value
					     poited out by index and val. */
    template <class V, class POS,class IDX>
    bool				add(const V& vals,const POS& pos,
					    IDX* index=0);
    					/*!<Adds values to stucture. */
    template <class V, class POS> bool	add(const V& vals,const POS& pos);
    					/*!<Adds values to stucture. */
    bool				append(const MultiDimStorage<T>&);
    template <class IDX> void		remove(const IDX& index);
    void				empty();
    					/*!<removes everything. */

    template <class POS,class IDX> bool	findFirst(const POS&,IDX&) const;
    template <class IDX> bool		getIndex(int global_pos,IDX&) const;
    template <class IDX,class POS> bool	getPos(const IDX&,POS&) const;
    template <class IDX> bool		next(IDX&,bool skipdups=false) const;
    template <class IDX> bool		prev(IDX&,bool skipdups=false) const;
    template <class IDX> bool		isValidPos(const IDX&) const;
    void				removeDuplicates();

    inline bool				divide(ObjectSet<int>&) const;
    					/*!<divides all items into chunks
					   of (about equal size). */
    template <class IDX> bool 		sort(TypeSet<IDX>& indices) const;
    					/*!<Sorts the array in order of
					    globalpos. */

    template <class POS> void		getIndicesInRange(const POS& start,
	    					   const POS& stop,
						   TypeSet<int>& res) const;
    					/*!<\returns all indices within
						   a given range. */

protected:
    void				getRangeI(int dim,Interval<int>&) const;
    template <class IDX> void		setToLastPos(IDX&) const;
    int					findFirstPos(int pos) const;
    template <class POS> void		getIndicesInRangeI(const POS& start,
	    					   const POS& stop,
						   TypeSet<int>& curpos,
						   TypeSet<int>& res) const;

    TypeSet<int>			positions_;
    ObjectSet<MultiDimStorage<T> >	lowerdimstorage_;
    TypeSet<T>				onedimstorage_;
    bool				allowduplicates_;

private:
    int					nrdims_;
    int					nrvals_;
};


template <class T> inline
MultiDimStorage<T>::MultiDimStorage( int nrdims, int nrvals )
    : nrdims_( nrdims )
    , nrvals_( nrvals )
    , allowduplicates_( false )
{}


template <class T> inline
MultiDimStorage<T>::MultiDimStorage( const MultiDimStorage<T>& templ )
{
    *this = templ;
}


template <class T> inline MultiDimStorage<T>&
MultiDimStorage<T>::operator=( const MultiDimStorage<T>& templ )
{
    nrdims_ = templ.nrdims_;
    nrvals_ = templ.nrvals_;
    allowduplicates_ = templ.allowduplicates_;
    positions_ = templ.positions_;
    onedimstorage_ = templ.onedimstorage_;
    deepCopy( lowerdimstorage_, templ.lowerdimstorage_ );
    return *this;
}


template <class T> inline
MultiDimStorage<T>::~MultiDimStorage()
{
    empty();
}


template <class T> inline
bool MultiDimStorage<T>::isOK() const
{
    if ( nrdims_==1 )
    {
	if ( !positions_.arr() )
	    return false;

	return !nrvals_ || onedimstorage_.arr();
    }

    for ( int idx=lowerdimstorage_.size()-1; idx>=0; idx-- )
    {
	if ( !lowerdimstorage_[idx]->isOK() )
	    return false;
    }

    return true;
}

	
template <class T> inline
void MultiDimStorage<T>::empty()
{
    deepErase( lowerdimstorage_ );
    onedimstorage_.erase();
    positions_.erase();
}


template <class T> inline
bool MultiDimStorage<T>::append( const MultiDimStorage<T>& b )
{
    if ( b.nrdims_!=nrdims_ || b.nrvals_!=nrvals_ )
	return false;

    for ( int idx=0; idx<b.size(); idx++ )
    {
	const int pos = b.getPos( idx );
	const int targetidx = indexOf( pos );
	int index = findFirstPos( pos );

	const bool match = positions_.validIdx(index) && positions_[index]==pos;
	if ( nrdims_!=1 )
	{
	    if ( !match )
	    {	
		MultiDimStorage<T>* newstorage =
				    new MultiDimStorage<T>( *b[idx] );
		if ( index==lowerdimstorage_.size() )
		{
		    lowerdimstorage_ += newstorage;
		    positions_ += pos;
		}
		else
		{
		    if ( index==-1 )
			index = 0;

		    lowerdimstorage_.insertAt( newstorage, index );
		    positions_.insert( index, pos );
		}
	    }
	    else
	    {
		if ( !lowerdimstorage_[index]->append( *b[idx] ) )
		    return false;
	    }
	}
	else
	{
	    if ( !add( &b.onedimstorage_[idx], &pos ) )
		return false;
	}
    }

    return true;
}


template <class T> inline
bool MultiDimStorage<T>::allowsDuplicates() const
{ return allowduplicates_; }
	

template <class T> inline
bool MultiDimStorage<T>::allowDuplicates( bool nv )
{
    const bool res = allowduplicates_;
    allowduplicates_ = nv;
    for ( int idx=lowerdimstorage_.size()-1; idx>=0; idx-- )
	lowerdimstorage_[idx]->allowDuplicates( nv );

    if ( !nv && res )
	removeDuplicates();

    return res;
}


template <class T> inline
bool MultiDimStorage<T>::isEmpty() const
{
    if ( nrdims_==1 )
	return positions_.isEmpty();

    for ( int idx=lowerdimstorage_.size()-1; idx>=0; idx-- )
	if ( !lowerdimstorage_[idx]->isEmpty() )
	    return false;

    return true;
}

	
template <class T> inline
int MultiDimStorage<T>::totalSize() const
{
    if ( nrdims_==1 )
	return positions_.size();

    int sum = 0;
    for ( int idx=lowerdimstorage_.size()-1; idx>=0; idx-- )
	sum += lowerdimstorage_[idx]->totalSize();

    return sum;
}

	
template <class T> inline
int MultiDimStorage<T>::size() const
{
    return positions_.size();
}


template <class T> inline
MultiDimStorage<T>* MultiDimStorage<T>::operator[]( int idx )
{
    if ( nrdims_==1 ) return 0;
    return lowerdimstorage_[idx];
}


template <class T> inline
const MultiDimStorage<T>* MultiDimStorage<T>::operator[]( int idx ) const
{
    if ( nrdims_==1 ) return 0;
    return lowerdimstorage_[idx];
}


template <class T> inline
int MultiDimStorage<T>::indexOf( int pos ) const
{ return positions_.indexOf( pos ); }


template <class T> inline
int MultiDimStorage<T>::getPos( int idx ) const
{ return positions_[idx]; }



template <class T> inline
bool MultiDimStorage<T>::getRange( int dim, Interval<int>& rg ) const
{
    rg.stop = INT_MIN;
    rg.start = INT_MAX;

    if ( !size() )
	return false;

    getRangeI( dim, rg );

    return true;
}


template <class T> inline
void MultiDimStorage<T>::getRangeI( int dim, Interval<int>& rg ) const
{
    if ( dim>=nrdims_ )
	return;

    if ( dim==nrdims_-1 )
    {
	const int sz = positions_.size();
	rg.include( positions_[0], false );
	if ( sz>1 )
	    rg.include( positions_[sz-1], false );
	return;
    }

    for ( int idx=0; idx<lowerdimstorage_.size(); idx++ )
	lowerdimstorage_[idx]->getRangeI( dim, rg );
}
    


template <class T> inline
int MultiDimStorage<T>::nrDims() const { return nrdims_; }


template <class T> inline
int MultiDimStorage<T>::nrVals() const { return nrvals_; }


template <class T> inline
void MultiDimStorage<T>::setNrVals( int nn, bool keepdata )
{
    if ( nn==nrvals_ )
	return;

    if ( nrdims_!=1 )
    {
	for ( int idx=lowerdimstorage_.size()-1; idx>=0; idx-- )
	    lowerdimstorage_[idx]->setNrVals( nn );

	nrvals_ = nn;
	return;
    }

    const int nrvalstocp = mMIN( nn, nrvals_ );
    const int nrpos = size();
    TypeSet<T> newstorage( nrpos*nn, mUdf(T) );
    if ( keepdata )
    {
	for ( int idx=0; idx<nrpos; idx++ )
	{
	    for ( int idy=0; idy<nrvalstocp; idy++ )
		newstorage[idx*nn+idy] = onedimstorage_[idx*nrvals_+idy];
	}
    }

    onedimstorage_ = newstorage;
    nrvals_ = nn;
}


template <class T> template <class IDX> inline
T& MultiDimStorage<T>::getRef( const IDX& indexarr, int val )
{
    const int index = indexarr[nrdims_-1];
    return nrdims_==1
	? onedimstorage_[index*nrvals_+val]
	: lowerdimstorage_[index]->getRef( indexarr, val );
}


template <class T> template <class IDX> inline
const T& MultiDimStorage<T>::getRef( const IDX& index, int val ) const
{ return const_cast<MultiDimStorage<T>* >(this)->getRef(index,val); }



template <class T> template <class V, class POS,class IDX> inline
bool MultiDimStorage<T>::add( const V& vals, const POS& posarr,
			      IDX* indexarr )
{
    const int dim = nrdims_-1;
    const int pos = posarr[dim];
    int index = findFirstPos( pos );

    const bool match = positions_.validIdx(index) && positions_[index]==pos;
    if ( !match ) index++;

    if ( indexarr ) (*indexarr)[dim] = index;

    if ( dim )
    {
	if ( !match )
	{
	    MultiDimStorage<T>* newstorage =
				new MultiDimStorage<T>( nrdims_-1, nrvals_ );
	    newstorage->allowDuplicates( allowduplicates_ );
	    if ( index==lowerdimstorage_.size() )
	    {
		lowerdimstorage_ += newstorage;
		positions_ += pos;
	    }
	    else
	    {
		if ( index==-1 )
		    index = 0;

		lowerdimstorage_.insertAt( newstorage, index );
		positions_.insert( index, pos );
	    }
	}
	
	if ( !lowerdimstorage_[index]->add( vals, posarr, indexarr ) )
	    return false;
    }
    else
    {
	if ( !match || (match && allowduplicates_) )
	{
	    for ( int idx=0; idx<nrvals_; idx++ )
	    {
		const int arrpos = index*nrvals_+idx;
		if ( arrpos>onedimstorage_.size() || arrpos<0 )
		    pErrMsg("Hmm");
		else if ( arrpos==onedimstorage_.size() )
		    onedimstorage_ += vals[idx];
		else
		    onedimstorage_.insert( index*nrvals_+idx, vals[idx] );
	    }

	    if ( index==positions_.size() )
		positions_ += pos;
	    else
		positions_.insert( index, pos );
	}
	else if ( match )
	{
	    for ( int idx=0; idx<nrvals_; idx++ )
	    	onedimstorage_[index*nrvals_+idx] = vals[idx];
	}
    }

    return true;
}


template <class T> template <class V, class POS> inline
bool MultiDimStorage<T>::add( const V& vals, const POS& posarr )
{
    return add<V,POS,int*>( vals, posarr, 0 );
}


template <class T> template<class IDX> inline
void MultiDimStorage<T>::remove( const IDX& indexarr )
{
    const int dim = nrdims_-1;
    const int index = indexarr[dim];
    
    if ( dim )
    {
	lowerdimstorage_[index]->remove( indexarr );
	if ( lowerdimstorage_[index]->size() )
	    return;

	delete lowerdimstorage_[index];
	lowerdimstorage_.remove( index );
	positions_.remove( index );
	return;
    }

    onedimstorage_.remove( index*nrvals_, index*nrvals_+nrvals_-1 );
    positions_.remove( index );
}


template <class T> template <class POS, class IDX> inline
bool MultiDimStorage<T>::findFirst( const POS& posarr, IDX& indexarr ) const
{
    const int dim = nrdims_-1;
    const int pos = posarr[dim];
    const int index = findFirstPos( pos );

    if ( index==-1 || positions_[index]!=pos )
	return false;

    indexarr[dim] = index;

    return dim ? lowerdimstorage_[index]->findFirst( posarr, indexarr ) : true;
}
    

template <class T> template <class IDX, class POS> inline
bool MultiDimStorage<T>::getPos( const IDX& indexarr, POS& pos ) const
{
    const int dim = nrdims_-1;
    int index = indexarr[dim];
    if ( index>=positions_.size() )
	return false;

    pos[dim] = positions_[index];
    return dim ? lowerdimstorage_[index]->getPos( indexarr, pos ) : true;
}


template <class T> template <class IDX> inline
bool MultiDimStorage<T>::getIndex( int globalpos, IDX& indexarr ) const
{
    const int dim = nrdims_-1;
    int& index = indexarr[dim];

    if ( dim )
    {
	for ( int idx=0; idx<lowerdimstorage_.size(); idx++ )
	{
	    const int localsum = lowerdimstorage_[idx]->totalSize();
	    if ( globalpos<localsum )
	    {
		index = idx;
		return lowerdimstorage_[idx]->getIndex( globalpos, indexarr );
	    }

	    globalpos -= localsum;
	}
	
	return false;
    }

    if ( globalpos<positions_.size() )
    {
	index = globalpos;
	return true;
    }

    return false;
}


template <class T> template <class IDX> inline
bool MultiDimStorage<T>::next( IDX& indexarr, bool skipduplicate ) const
{
    const int dim = nrdims_-1;
    int& index = indexarr[dim];

    if ( dim )
    {
        if ( index<0 )
	{
	    if ( lowerdimstorage_.size() )
	        index = 0;
	    else
	    	return false;
	}

	while ( !lowerdimstorage_[index]->next( indexarr, skipduplicate ) )
	{
	    index++;
	    if ( index>=lowerdimstorage_.size() )
	    {
		index = -1;
		return false;
	    }
	}

	return true;
    }

    if ( index<0 )
    {
	if ( positions_.size() )
	{
	    index = 0;
	    return true;
	}
	else
	    return false;
    }

    const int oldpos = positions_[index];
    do
    {
	index++;
	if ( index>=positions_.size() )
	{
	    index = -1;
	    return false;
	}
    } while ( skipduplicate && positions_[index]==oldpos );

    return true;
}


template <class T> template <class IDX> inline
bool MultiDimStorage<T>::prev( IDX& indexarr, bool skipduplicate ) const
{
    const int dim = nrdims_-1;
    int& index = indexarr[dim];
    if ( dim )
    {
	if ( lowerdimstorage_[index]->prev( indexarr, skipduplicate ) )
	    return true;

	index--;
	if ( index<0 )
	    return false;

	lowerdimstorage_[index]->setToLastPos( indexarr );
	return true;
    }

    return --index>=0;
}


template <class T> template<class IDX> inline 
bool MultiDimStorage<T>::isValidPos( const IDX& indexarr ) const
{
    const int dim = nrdims_-1;
    const int index = indexarr[dim];

    if ( index<0 )
	return false;

    if ( dim )
	return index<lowerdimstorage_.size() &&
	       lowerdimstorage_[index]->isValidPos( indexarr );

    return index<positions_.size();
}
    
    
template <class T> inline
int MultiDimStorage<T>::findFirstPos( int index ) const
{
    const bool mayhaveduplicates = allowduplicates_ && nrdims_==1;
    int res;
    if ( IdxAble::findPos(positions_,positions_.size(),index,-1,res) &&
	 mayhaveduplicates )
    {
	while ( res && positions_[res-1]==index )
	    res--;
    }
    
    return res;
}


/*!Gives all indices within a position range.
   \param start array with the interval starts for each dimension
   \param stop  array with the interval stops for each dimension
   \param res   the output. The indexes are multiplexed in, so the first
   		ndims samples are the indices for first position, the
		second ndim samples are the indices for the second postion and
		so forth. */

template <class T> template <class POS> inline
void MultiDimStorage<T>::getIndicesInRange(const POS& start, const POS& stop,
    TypeSet<int>& res) const
{
    TypeSet<int> curpos( nrdims_, -1 );
    getIndicesInRangeI( start, stop, curpos, res );
}


template <class T> template <class POS> inline
void MultiDimStorage<T>::getIndicesInRangeI(const POS& startarr,
	const POS& stoparr, TypeSet<int>& curpos, TypeSet<int>& res) const
{
    const int dim = nrdims_-1;
    const int start = startarr[dim];
    const int stop = stoparr[dim];
    const int sz = positions_.size();
    if ( !sz || stop<positions_[0] || start>positions_[sz-1] )
	return;

    int idx = findFirstPos(start);
    if ( idx<0 ) idx = 0;
    if ( positions_[idx]<start ) idx++;
    for ( ; idx<sz; idx++ )
    {
	if ( positions_[idx]>stop )
	    break;

	curpos[dim] = idx;
	if ( !dim )
	    res.append( curpos );
	else
	    lowerdimstorage_[idx]->getIndicesInRangeI(startarr,stoparr,
		    				      curpos,res);
    }
}


template <class T> template <class IDX> inline
void MultiDimStorage<T>::setToLastPos( IDX& indexarr ) const
{
    const int dim = nrdims_-1;
    const int index = indexarr[dim] = size()-1;
    if ( dim )
	lowerdimstorage_[index]->setToLastPos( indexarr );
}
	
	
template <class T> inline
void MultiDimStorage<T>::removeDuplicates()
{
    if ( nrdims_!=1 ) return;

    for ( int idx=1; idx<positions_.size(); idx++ )	
    {
	if ( positions_[idx-1]==positions_[idx] )
	{
	    positions_.remove( idx );
	    for ( int idy=0; idy<nrvals_; idy++ )
		onedimstorage_.remove( idx );

	    idx--;
	}
    }
}
template <class T> inline
bool MultiDimStorage<T>::divide( ObjectSet<int>& res ) const
{
    const int nrchunks = res.size();

    if ( !nrchunks )
	return false;

    const int totalsize = totalSize();
    if ( !totalsize )
	return -1;


    const int nrperchunk = mNINT32((float) totalsize / nrchunks );

    int allocidx[nrdims_];
    int* idxs = allocidx;
    for ( int idx=0; idx<nrdims_; idx++ )
	idxs[idx] = -1;

    int idx=0;
    int prevchunk = -1;
    while ( next(idxs) )
    {
	int chunk = idx/nrperchunk;
	if ( chunk>=nrchunks )
	    chunk = nrchunks-1;

	idx++;

	if ( chunk==prevchunk )
	    continue;

	for ( int idy=0; idy<nrdims_; idy++ )
	    res[chunk][idy] = idxs[idy];

	prevchunk = chunk;
    }

    return true;
}


template <class T> template <class IDXS> inline
bool MultiDimStorage<T>::sort( TypeSet<IDXS>& indices ) const
{
    if ( !size() )
	return false;

    TypeSet<long long> dimsizes;

    long long multiplicator = 1;
    for ( int dim=0; dim<nrdims_; dim++ )
    {
	dimsizes += multiplicator;

	Interval<int> rg;
	if ( getRange( dim, rg ) )
	    multiplicator *= ( rg.width()+1 );
    }

    TypeSet<long long> sortkeys;
    TypeSet<long long> idxs;

    for ( int idx=0; idx<indices.size(); idx++ )
    {
	long long sortkey = 0;
	for ( int dim=0; dim<nrdims_; dim++ )
	    sortkey += indices[idx][dim] * dimsizes[dim];

	sortkeys += sortkey;
	idxs += idx;
    }

    sort_coupled( sortkeys.arr(), idxs.arr(), idxs.size() );

    TypeSet<IDXS> copy = indices;
    for ( int idx=idxs.size()-1; idx>=0; idx-- )
	indices[idx] = copy[idxs[idx]];

    return true;
}


#endif
