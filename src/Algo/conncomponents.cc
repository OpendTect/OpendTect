/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * fault dip calculation based the pca analysis to the fault attributes
 * AUTHOR   : Bo Zhang/Yuancheng Liu
 * DATE     : July 2012
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "conncomponents.h"

#include "arrayndimpl.h"
#include "arrayndslice.h"
#include "executor.h"
#include "sorting.h"
#include "task.h"


class CC2DExtracor : public ParallelTask
{
public:

CC2DExtracor( const Array3D<bool>& inp )
    : input_(inp)
{
    const int slicesz = input_.info().getSize(2);
    for ( int idx=0; idx<slicesz; idx++ )
    {
	TypeSet<TypeSet<int> > comps;
	slicecomps_ += comps;
    }
}

const TypeSet<TypeSet<TypeSet<int> > >&	getCubeComps()	{ return slicecomps_; }

protected:
od_int64 nrIterations() const	{ return input_.info().getSize(2); }
const char* message() const	{ return "Computing 2D connected  components"; }

bool doWork( od_int64 start, od_int64 stop, int threadid )
{
    for ( int idx=mCast(int,start); idx<=stop && shouldContinue(); idx++ )
    {
	PtrMan<Array2DSlice<bool> > slice = new Array2DSlice<bool>( input_ );
	slice->setPos( 2, idx );
	slice->setDimMap( 0, 0 );
	slice->setDimMap( 1, 1 );
	slice->init();

	ConnComponents cc( *slice );
	cc.compute();

	TypeSet<TypeSet<int> >& slicecomp = slicecomps_[idx];
	for ( int idy=cc.nrComponents()-1; idy>=0; idy-- )
	{
	    const TypeSet<int>* comp = cc.getComponent(idy);
	    if ( comp )
		slicecomp += *comp;
	}

	addToNrDone(1);
    }

    return true;
}

const Array3D<bool>&			input_;
TypeSet<TypeSet<TypeSet<int> > >	slicecomps_;
};


ConnComponents::ConnComponents( const Array2D<bool>& input ) 
    : input_(input)
{
    label_ = new Array2DImpl<int>( input.info() );
}


ConnComponents::~ConnComponents()
{ delete label_; }


void ConnComponents::compute( TaskRunner* tr )
{
    label_->setAll(0);
    classifyMarks( *label_ );

    const int sz = mCast( int,input_.info().getTotalSz() );
    int* markers = label_->getData();
    components_.erase();
    TypeSet<int> labels;
    
    for ( int idx=0; idx<sz; idx++ )
    {
	if ( markers[idx]<1 )
	    continue;

	const int curidx = labels.indexOf(markers[idx]);
	if ( curidx==-1 )
	{
	    labels += markers[idx];
	     
	    TypeSet<int> ids;
	    ids += idx;
	    components_ += ids;
	}
	else
	{
	    components_[curidx] += idx;
	}
    }
    
    const int nrcomps = labels.size(); 
    TypeSet<int> nrnodes;
    sortedindex_.erase();
    for ( int idx=0; idx<nrcomps; idx++ )
    {
	nrnodes += components_[idx].size();
	sortedindex_ += idx;
    }

    sort_coupled( nrnodes.arr(), sortedindex_.arr(), nrcomps );
}


void ConnComponents::classifyMarks( Array2D<int>& mark )
{
    const int r = mark.info().getSize(0);
    const int c = mark.info().getSize(1);

    int index = 0;
    for ( int i=1; i<r-1; i++ )
    {
	for ( int j=1; j<c-1; j++ )
	{
	    if ( !input_.get(i,j) )
		continue;

	    if ( input_.get(i-1,j+1) )
	    {
		const int m = mark.get(i-1,j+1);
		if ( m>0 )
		{
		    mark.set(i,j,m);
		    
		    const int smij = mark.get(i,j-1);
		    const int nwmij = mark.get(i-1,j+1);
		    const int swmij = mark.get(i-1,j-1);
		    if ( smij>0 && smij!=nwmij )
		    {
			setMark( mark, mMAX(smij,nwmij), mMIN(smij,nwmij) );
		    }

		    if ( swmij>0 && swmij!=nwmij )
		    {
			setMark( mark, mMAX(swmij,nwmij), mMIN(swmij,nwmij) );
		    }
		}
		else
		{
		    index++;
		    mark.set(i,j,index);
		}
	    }
	    else if ( input_.get(i-1,j) )
	    {
		const int wmij = mark.get(i-1,j);
		if ( wmij>0 )
		    mark.set(i,j,wmij);
		else
		{
		    index++;
		    mark.set(i,j,index);
		}
	    }
	    else if ( input_.get(i-1,j-1) )
	    {
		const int swmij = mark.get(i-1,j-1);
		if ( swmij>0 )
		    mark.set(i,j,swmij);
		else
		{
		    index++;
		    mark.set(i,j,index);
		}
	    }
	    else if ( input_.get(i,j-1) )
	    {
		const int smij = mark.get(i,j-1);
		if ( smij>0 )
		    mark.set(i,j,smij);
		else
		{
		    index++;
		    mark.set(i,j,index);
		}
	    }
	    else
	    {
		index++;
		mark.set(i,j,index);
	    }
	}
    }
}


void ConnComponents::setMark( Array2D<int>& mark, int source, int newval )
{
    const int sz = mCast( int, mark.info().getTotalSz() );
    int* vals = mark.getData();
    for ( int idx=0; idx<sz; idx++ )
    {
	if ( vals[idx]==source )
	    vals[idx] = newval;
    }
}


int ConnComponents::nrComponents() const
{ return components_.size(); }


const TypeSet<int>* ConnComponents::getComponent( int cidx )
{
    return  cidx<0 || cidx>=nrComponents() ? 0 : 
	&components_[sortedindex_[cidx]]; 
}


void ConnComponents::trimCompBranches( TypeSet<int>& comp )
{
    return trimCompBranches( comp, input_.info().getSize(1) );
}


void ConnComponents::getCompSticks( TypeSet<int>& comp, int sz1, 
	int allowgapsz, int minsticksz, TypeSet<TypeSet<int> >& sticks )
{
    const int compsz = comp.size();
    if ( compsz<minsticksz || compsz<2 )
	return;
    
    TypeSet<int> inls, crls;
    TypeSet<TypeSet<int> > inlcs, crlis;
    for ( int idx=0; idx<compsz; idx++ )
    {
     	const int inl = comp[idx]/sz1;
    	const int crl = comp[idx]%sz1;
	
	const int iidx = inls.indexOf(inl);
    	if ( iidx<0 )
    	{
	    inls += inl;
	    TypeSet<int> icrls; 
	    icrls += crl;
	    inlcs += icrls;
	}
    	else
	    inlcs[iidx] += crl;
	
    	const int cidx = crls.indexOf(crl);
    	if ( cidx<0 )
    	{
    	    crls += crl;
	    TypeSet<int> cinls; 
	    cinls += inl;
	    crlis += cinls;
	}
    	else
	    crlis[cidx] += inl;
    }

    const bool alonginl = inls.size() >= crls.size();
    const int sticksz = alonginl ? inls.size() : crls.size();
    TypeSet<int> ics = alonginl ? inls : crls;
    const TypeSet<TypeSet<int> >& nbs = alonginl ? inlcs : crlis;
    TypeSet<int> ids;
    for ( int k=0; k<sticksz; k++ )
	ids += k;

    sort_coupled( ics.arr(), ids.arr(), sticksz );

    TypeSet<int> stick;
    int prevpos = -1, sidx = mUdf(int);
    for ( int i=0; i<sticksz; i++ )
    {
	int res = -1;
	const int count = nbs[ids[i]].size();
	if ( prevpos<0 )
	    res = nbs[ids[i]][0];
	else
	{
	    if ( abs(ics[i]-ics[sidx])>allowgapsz )
		break;

	    if ( count==1 )
	    {
		const int diff = nbs[ids[i]][0] - prevpos;
		if ( abs(diff)<=allowgapsz )
		    res = nbs[ids[i]][0];
	    }
	    else
	    {
		int nearestpos = nbs[ids[i]][0];
		int mindist = abs(nearestpos-prevpos);
		for ( int k=1; k<count; k++ )
		{
		    const int dp = abs(nbs[ids[i]][k]-prevpos);
		    if ( dp<mindist )
		    {
			nearestpos = nbs[ids[i]][k];
			mindist = dp;
		    }
		}
		if ( mindist<=allowgapsz )
		    res = nearestpos;
	    }
	}
	
	if ( res>-1 )
	{
	    prevpos = res;
	    sidx = i;
	    stick +=  alonginl ? ics[i]*sz1+res : res*sz1+ics[i];
	}
	else if ( abs(ics[i]-ics[sidx])>allowgapsz )
	    break;
    }

    const int sz = stick.size();
    for ( int idx=0; idx<sz; idx++ )
	comp -= stick[idx];

    if ( sz>=minsticksz )
	sticks += stick;

    getCompSticks( comp, sz1, allowgapsz, minsticksz, sticks );
}


void ConnComponents::trimCompBranches( TypeSet<int>& comp, int dimsz1 )
{
    BoolTypeSet toremove;
    for ( int idx=comp.size()-1; idx>=0; idx-- )
	toremove += true;

    for ( int idx=comp.size()-1; idx>=0; idx-- )
    {
	if ( !toremove[idx] ) 
	    continue;

	int nbs[4] = { comp[idx]+1, comp[idx]-1, comp[idx]+dimsz1, 
	    comp[idx]-dimsz1 };
	for ( int k=0; k<4; k++ )
	{
    	    int pidx = comp.indexOf( nbs[k] );
    	    if ( pidx!=-1 )
    	    {
    		toremove[idx] = false;
    		toremove[pidx] = false;
    		break;
    	    } 
	}
    }

    for ( int idx=comp.size()-1; idx>=0; idx-- )
    {
	if ( toremove[idx] ) 
	    comp -= comp[idx];
    }

    const int compsz = comp.size();
    if ( compsz<4 ) return;

    TypeSet<int> endids;
    TypeSet<TypeSet<int> > nbs;
    TypeSet<int> endrows, endcols;

    for ( int idx=0; idx<compsz; idx++ )
    {
	const int curid = comp[idx];

	TypeSet<int> conns;
	for ( int i=-1; i<2; i++ )
	{
	    for ( int j=-1; j<2; j++ )
	    {
		if ( !i && !j )
		    continue;

		const int nbid = curid+i+j*dimsz1;
		if ( comp.indexOf(nbid)!=-1 )
		    conns += nbid;
	    }
	}

	if ( conns.size()<2 )
	{
	    endids += idx;
	    endrows += curid/dimsz1;
	    endcols += curid%dimsz1;
	}

	nbs += conns;
    }

    const int nrends = endids.size();
    if ( nrends<3 ) return;

    int startidx, stopidx, maxdist=0;
    for ( int i=0; i<nrends; i++ )
    {
	for ( int j=i+1; j<nrends; j++ )
	{
	    int rd = endrows[i]-endrows[j];
	    int cd = endcols[i]-endcols[j];
	    int sqdist = rd*rd+cd*cd;
	    if ( sqdist>maxdist )
	    {
		startidx = endids[i];
		stopidx = endids[j];
		maxdist = sqdist;
	    }
	}
    }

    endids -= startidx;
    endids -= stopidx;

    toremove.erase();
    for ( int idx=0; idx<comp.size(); idx++ )
	toremove += false;

    for ( int idx=endids.size()-1; idx>=0; idx-- )
    {
	int curidx = endids[idx];
	toremove[curidx] = true;
	int previdx = curidx;
	curidx = comp.indexOf(nbs[previdx][0]);
	int jointsz = nbs[curidx].size();

	while ( jointsz==2 && !toremove[curidx] )
	{
	    toremove[curidx] = true;
	    const bool isfirst = comp[previdx]==nbs[curidx][0];

	    previdx = curidx;
	    curidx = comp.indexOf(isfirst ? nbs[curidx][1] : nbs[curidx][0]);
	    jointsz = nbs[curidx].size();
	}

	if ( jointsz>2 )
	{
	    bool hasvhnb = false;
	    for ( int idy=0; idy<jointsz; idy++ )
	    {
		if ( nbs[curidx][idy]==comp[previdx] )
		    continue;

		int dist = abs(comp[curidx]-nbs[curidx][idy]);
		if ( dist==1 || dist==dimsz1 )
		{
		    hasvhnb = true;
		    break;
		}
	    }

	    if ( !hasvhnb )
    		toremove[curidx] = true;
	}
    }

    for ( int idx=compsz-1; idx>=0; idx-- )
    {
	if ( toremove[idx] ) 
	    comp -= comp[idx];
    }

    trimCompBranches( comp, dimsz1 );
}


float ConnComponents::overLapRate( int componentidx )
{
    const TypeSet<int>* comp = getComponent( componentidx );
    if ( !comp )
	return 1;

    const int csz = comp->size();
    const int ysz = input_.info().getSize(1);

    TypeSet<int> idxs, idys;
    for ( int idx=0; idx<csz; idx++ )
    {
	const int row = (*comp)[idx]/ysz;
	const int col = (*comp)[idx]%ysz;
	if ( idxs.indexOf(row)<0 )
	    idxs += row;

	if ( idys.indexOf(col)<0 )
	    idys += col;
    }

    const float xrate = 1 - ((float)idxs.size())/((float)csz);
    const float yrate = 1 - ((float)idys.size())/((float)csz);
    return mMIN(xrate,yrate);
}


ConnComponents3D::ConnComponents3D( const Array3D<bool>& input ) 
    : input_(input)
{}


ConnComponents3D::~ConnComponents3D()
{ deepErase(components_); }


int ConnComponents3D::nrComponents() const
{ return components_.size(); }


const ObjectSet<ConnComponents3D::VPos>* ConnComponents3D::getComponent( int ci)
{ return  ci<0 || ci>=nrComponents() ? 0 : components_[sortedindex_[ci]]; }


void ConnComponents3D::compute( TaskRunner* tr )
{
    CC2DExtracor cc( input_ );
    bool extraced = TaskRunner::execute( tr, cc );
    if ( !extraced )
	return;

    const TypeSet<TypeSet<TypeSet<int> > >& comps = cc.getCubeComps();
    const int nrslices = comps.size();
    
    TypeSet<TypeSet<unsigned char> > usedcomps;
    for ( int idx=0; idx<nrslices; idx++ )
    {
	const TypeSet<TypeSet<int> >& slicecomps = comps[idx];
	const int slicecompsz = slicecomps.size();
	
	TypeSet<unsigned char> used;
	for ( int idy=0; idy<slicecompsz; idy++ )
	    used += 0;

	usedcomps += used;
    }

    deepErase( components_ );
    TypeSet<int> nrcomps;
    for ( int idx=0; idx<nrslices; idx++ )
    {
	const TypeSet<TypeSet<int> >& slicecomps = comps[idx];
	const int slicecompsz = slicecomps.size();
	
	for ( int idy=0; idy<slicecompsz; idy++ )
	{
	    if ( usedcomps[idx][idy] )
		continue;

	    ObjectSet<VPos>  ccomp;
	    addToComponent( comps, idx, idy, usedcomps, ccomp );

	    const int compsz = ccomp.size();
	    if ( compsz )
	    {
		components_ += &ccomp;
		nrcomps += compsz;
	    }
	}
    }

    const int totalcompsz = components_.size();
    sortedindex_.erase();
    for ( int idx=0; idx<totalcompsz; idx++ )
	sortedindex_ += idx;

    sort_coupled( nrcomps.arr(), sortedindex_.arr(), totalcompsz );
}


void ConnComponents3D::addToComponent( 
	const TypeSet<TypeSet<TypeSet<int> > >& comps, int sliceidx, 
	int compidx, TypeSet<TypeSet<unsigned char> >& usedcomps,
        ObjectSet<VPos>& rescomp )
{
    const int slicesz = comps.size();
    if ( sliceidx<0 || sliceidx>=slicesz || usedcomps[sliceidx][compidx] )
	return;

    const TypeSet<TypeSet<int> >& slicecomps = comps[sliceidx];
    if ( compidx<0 || compidx>=slicecomps.size() )
	return;

    const TypeSet<int>& startcomp = slicecomps[compidx];
    const int compsz = startcomp.size();
    const int nrcrls = input_.info().getSize(1);
    for ( int idx=0; idx<compsz; idx++ )
    {
	VPos* v = new VPos();
	v->i = startcomp[idx]/nrcrls;
	v->j = startcomp[idx]%nrcrls;
	v->k = sliceidx;
	rescomp += v;
    }

    usedcomps[sliceidx][compidx] = 1;

    const int nextslice = sliceidx+1;
    if ( nextslice>=slicesz )
	return;

    const TypeSet<TypeSet<int> >& nextslicecomps = comps[nextslice];
    const int nextcompsz = nextslicecomps.size();
    for ( int idx=0; idx<nextcompsz; idx++ )
    {
	const TypeSet<int>& curcomp = nextslicecomps[idx];
	bool doconnect = false;
	for ( int idy=0; idy<curcomp.size(); idy++ )
	{
	    if ( startcomp.indexOf(curcomp[idy])!=-1 )
	    {
		doconnect = true;
		break;
	    }
	}

	if ( doconnect )
	    addToComponent( comps, nextslice, idx, usedcomps, rescomp );
    }
}

