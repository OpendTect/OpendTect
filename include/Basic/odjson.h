#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "arraynd.h"
#include "filepath.h"
#include "typeset.h"
#include "uistringset.h"

#include <climits>

class DBKeySet;
class DBKey;
class IdxPair;
class SeparString;
class StringBuilder;
class od_istream;
class od_ostream;
namespace Gason { struct JsonNode; }

namespace OD
{

namespace JSON
{

class Array;
class KeyedValue;
class Object;
class Value;

/*! data types you can find in a JSON file */

enum DataType
{
    Boolean, Number, INumber, String, Mixed
};

typedef bool BoolType;
typedef double NumberType;
typedef od_int64 INumberType;


/*! holds 'flat' value sets of each of the DataType's */

mClass(Basic) ValArr
{
public:

    typedef BoolTypeSet::size_type	size_type;
    typedef size_type			idx_type;
    typedef BoolTypeSet			BSet;
    typedef TypeSet<NumberType>		NSet;
    typedef TypeSet<INumberType>	INSet;
    typedef BufferStringSet		SSet;

			ValArr(DataType);
			ValArr(const ValArr&);
    virtual		~ValArr();
    ValArr&		operator=(const ValArr&)	= delete;

    DataType		dataType() const	{ return type_; }

    size_type		size() const
			{ return (size_type)set_->nrItems(); }
    bool		validIdx( idx_type idx ) const
			{ return set_->validIdx(idx); }
    bool		isEmpty() const		{ return set_->isEmpty(); }
    void		setEmpty()		{ set_->setEmpty(); }
    void		setFilePath(const FilePath&,idx_type);
    FilePath		getFilePath(idx_type) const;

    void		dumpJSon(BufferString&) const;
    void		dumpJSon(StringBuilder&) const;
    BufferString	dumpJSon() const;

private:
    friend class Array; // Only for the Array class

    OD::Set&		odSet()			{ return *set_; }
    const OD::Set&	odSet() const		{ return *set_; }
    BSet&		bools()			{ return *((BSet*)set_); }
    const BSet&		bools() const		{ return *((BSet*)set_); }
    NSet&		vals()			{ return *((NSet*)set_); }
    const NSet&		vals() const		{ return *((NSet*)set_); }
    INSet&		ivals()			{ return *((INSet*)set_); }
    const INSet&	ivals() const		{ return *((INSet*)set_); }
    SSet&		strings()		{ return *((SSet*)set_); }
    const SSet&		strings() const		{ return *((SSet*)set_); }

protected:

    DataType		type_ = Boolean;
    OD::Set*		set_ = nullptr;

private:

			ValArr()	 = delete;

public:
			// Only for the parser
    void		ensureNumber();

};


/*!\brief holds values and sets of values.
	    Is base class for either Array or Object. */

mExpClass(Basic) ValueSet
{ mODTextTranslationClass(OD::JSON::ValueSet)
public:

    typedef ValArr::size_type	size_type;
    typedef size_type		idx_type;
    enum ValueType		{ Data, SubArray, SubObject };
    typedef Gason::JsonNode	GasonNode;

    virtual			~ValueSet();

    ValueSet&			operator=(const ValueSet&)	= delete;
    virtual ValueSet*		clone() const			= 0;
    virtual bool		isArray() const			= 0;
    inline Array&		asArray();
    inline const Array&		asArray() const;
    inline Object&		asObject();
    inline const Object&	asObject() const;

    virtual size_type		size() const
				{ return (size_type)values_.size(); }
    virtual bool		isEmpty() const
				{ return values_.isEmpty(); }
    virtual void		setEmpty();
    virtual bool		validIdx( idx_type idx ) const
				{ return values_.validIdx(idx); }

    virtual ValueType		valueType(idx_type) const;
    virtual DataType		dType(idx_type) const;
				//!< Only for ValueType==Data
    const BufferString&		key(idx_type) const;
    inline bool			isPlainData( idx_type i ) const
				{ return valueType(i) == Data; }

    inline bool			isArrayChild( idx_type i ) const
				{ return valueType(i) == SubArray; }
    inline bool			isObjectChild( idx_type i ) const
				{ return valueType(i) == SubObject; }

    bool			isTop() const		{ return !parent_; }
    ValueSet*			top();
    const ValueSet*		top() const;

    inline ValueSet&		child( idx_type i )
				{ return *gtChildByIdx(i); }
    inline const ValueSet&	child( idx_type i ) const
				{ return *gtChildByIdx(i); }
    inline Array&		array( idx_type i )
				{ return *gtArrayByIdx(i); }
    inline const Array&		array( idx_type i ) const
				{ return *gtArrayByIdx(i); }
    inline Object&		object( idx_type i )
				{ return *gtObjectByIdx(i); }
    inline const Object&	object( idx_type i ) const
				{ return *gtObjectByIdx(i); }

    virtual bool		getBoolValue(idx_type) const;
    virtual od_int64		getIntValue(idx_type) const;
    virtual double		getDoubleValue(idx_type) const;
    virtual BufferString	getStringValue(idx_type) const;
    virtual FilePath		getFilePath(idx_type) const;

    uiRetVal			parseJSon(char* buf,int bufsz,
					  bool allowmixedarr=false);
    static ValueSet*		getFromJSon(char* buf,int bufsz,uiRetVal&,
					    bool allowmixedarr=false);
    void			dumpJSon(BufferString&,bool pretty=false) const;
    virtual void		dumpJSon(StringBuilder&) const;
    virtual BufferString	dumpJSon(bool pretty=false) const;

    uiRetVal			read(const char* fnm,bool allowmixedarr=false);
    static ValueSet*		read(const char* fnm,uiRetVal&,
				     bool allowmixedarr=false);
    uiRetVal			write(const char* fnm,bool pretty);

    uiRetVal			read(od_istream&,bool allowmixedarr=false);
    static ValueSet*		read(od_istream&,uiRetVal&,
				     bool allowmixedarr=false);
    uiRetVal			write(od_ostream&,bool pretty=false);
    uiRetVal			writePretty(od_ostream&);

protected:

				ValueSet(ValueSet* parent);
				ValueSet(const ValueSet&);

    ValueSet*			parent_;
    ObjectSet<Value>		values_;

    void			setParent( ValueSet* p )	{ parent_ = p; }

    ValueSet*			gtChildByIdx(idx_type) const;
    Array*			gtArrayByIdx(idx_type) const;
    Object*			gtObjectByIdx(idx_type) const;

    static ValueSet*		gtByParse(char*,int,bool allowmixed,
					  uiRetVal&,ValueSet*);
    void			use(const GasonNode&,bool allowmixed);

    friend class		Array;
    friend class		Object;
    friend class		Value;

};


/*!\brief ValueSet where the values and subsets have no key.

  If it holds plain data (valType()==Data), then you can only add
  plain values or set all at once. Otherwise, you can only add ValueSet's of
  the same type (either Array or Object).
 */

mExpClass(Basic) Array : public ValueSet
{
public:

			Array(bool objects,ValueSet* =nullptr);
			Array(DataType,ValueSet* =nullptr);
			Array(const Array&);
			~Array();

    Array&		operator=(const Array&)	= delete;
    Array*		clone() const override	{ return new Array(*this); }
    bool		isArray() const override	{ return true; }

    size_type		size() const override;
    bool		isEmpty() const override;
    void		setEmpty() override;
    bool		validIdx(idx_type) const override;

    bool		isData() const;
    bool		isMixed() const;

    ValueType		valueType(idx_type) const override
			{ return valtype_; }
    DataType		dType(idx_type) const override;
    ValueType		valType() const		{ return valtype_; }
    DataType		dataType() const;

    void		dumpJSon(StringBuilder&) const override;
    BufferString	dumpJSon(bool pretty=false) const override;

    Array*		add(Array*);
    Object*		add(Object*);

    bool		getBoolValue(idx_type) const override;
    od_int64		getIntValue(idx_type) const override;
    double		getDoubleValue(idx_type) const override;
    BufferString	getStringValue(idx_type) const override;
    FilePath		getFilePath(idx_type) const override;

    bool		get(::IdxPair&) const;
    bool		get(Coord&) const;
    bool		get(Coord3&) const;
    bool		get(TypeSet<Coord>&) const;
    bool		get(TypeSet<Coord3>&) const;
    bool		get(BufferStringSet&) const;
    bool		get(uiStringSet&) const;
    bool		get(TypeSet<MultiID>&) const;
    bool		get(DBKeySet&) const;
    bool		get(TypeSet<FilePath>&) const;
    mFloatIntegralNoBoolTemplate(T)
    bool		get(TypeSet<T>&) const;
    bool		get(BoolTypeSet&) const;

			// only usable if valType() == Data or Mixed
    Array&		set(const BufferStringSet&);
    Array&		set(const uiStringSet&);
    Array&		set(const TypeSet<MultiID>&);
    Array&		set(const DBKeySet&);
    Array&		set(const BoolTypeSet&);
    Array&		set(const TypeSet<FilePath>&);
    Array&		set(const bool*,size_type);

#   define		mDeclJSONArraySetAddFns( typ ) \
    Array&		set(const TypeSet<typ>&); \
    Array&		set(const typ*,size_type); \
    Array&		add(typ); \
    Array&		add(const TypeSet<typ>&); \
    Array&		add(const typ*,size_type);
			//!< Adding sets only for SubArray types

			mDeclJSONArraySetAddFns(od_int16);
			mDeclJSONArraySetAddFns(od_uint16);
			mDeclJSONArraySetAddFns(od_int32);
			mDeclJSONArraySetAddFns(od_uint32);
			mDeclJSONArraySetAddFns(od_int64);
			mDeclJSONArraySetAddFns(float);
			mDeclJSONArraySetAddFns(double);

    Array&		set(const FilePath&,idx_type);
    Array&		add(bool);
    Array&		add(Coord);
    Array&		add(Coord3);
    Array&		add(const TypeSet<Coord>&);
    Array&		add(const TypeSet<Coord3>&);

    Array&		add(const char*);
    Array&		add(const OD::String&);
    Array&		add(const uiString&);
    Array&		add(const FilePath&);
    Array&		add(const MultiID&);
    Array&		add(const DBKey&);

protected:

    ValueType		valtype_;
    ValArr*		valarr_		= nullptr;

			/*!< Only available if valType() == Data
			     and not using mixed-type arrays */
    inline ValArr&	valArr()		{ return *valarr_; }
    inline const ValArr& valArr() const		{ return *valarr_; }

    mFloatIntegralNoBoolTemplate(T)
    Array&		setVals(const TypeSet<T>&);
    mFloatIntegralTemplate(T)
    Array&		setVals(const T*,size_type);
    mFloatIntegralTemplate(T)
    Array&		addVal(T);
    mFloatIntegralNoBoolTemplate(T)
    Array&		addVals(const TypeSet<T>&);
    mFloatIntegralTemplate(T)
    Array&		addVals(const T*,size_type);

    void		addVS(ValueSet*);

    friend class	ValueSet;

private:
    void		ensureNumber();
    void		ensureMixed();
			//<! only for the parser

};


/*!\brief ValueSet where the values and subsets have a key. */

mExpClass(Basic) Object : public ValueSet
{
public:

			Object(ValueSet* p=nullptr);
			Object(const Object&);
			~Object();

    Object&		operator=(const Object&)	= delete;
    Object*		clone() const override	{ return new Object(*this); }
    bool		isArray() const override { return false; }

    idx_type		indexOf(const char*) const;
    bool		isPresent( const char* ky ) const
						{ return indexOf(ky) >= 0; }

    inline ValueSet*	getChild( const char* ky )
			{ return gtChildByKey(ky); }
    inline const ValueSet* getChild( const char* ky ) const
			{ return gtChildByKey(ky); }
    inline Array*	getArray( const char* ky )
			{ return gtArrayByKey(ky); }
    inline const Array* getArray( const char* ky ) const
			{ return gtArrayByKey(ky); }
    inline Object*	getObject( const char* ky )
			{ return gtObjectByKey(ky); }
    inline const Object* getObject( const char* ky ) const
			{ return gtObjectByKey(ky); }

    void		getSubObjKeys(BufferStringSet&) const;
    inline ValueSet*	getChild( const BufferStringSet& bskey )
				{ return gtChildByKeys( bskey ); }
    inline const ValueSet* getChild( const BufferStringSet& bskey ) const
				{ return gtChildByKeys( bskey ); }
    inline Array*	getArray( const BufferStringSet& bskey )
				{ return gtArrayByKeys( bskey ); }
    inline const Array* getArray( const BufferStringSet& bskey ) const
				{ return gtArrayByKeys( bskey ); }
    inline Object*	getObject( const BufferStringSet& bskey )
				{ return gtObjectByKeys( bskey ); }
    inline const Object* getObject( const BufferStringSet& bskey ) const
				{ return gtObjectByKeys( bskey ); }

    od_int64		getIntValue( idx_type idx ) const override
			{ return ValueSet::getIntValue( idx ); }
    bool		getBoolValue( idx_type idx ) const override
			{ return ValueSet::getBoolValue( idx ); }
    double		getDoubleValue( idx_type idx ) const override
			{ return ValueSet::getDoubleValue( idx ); }
    BufferString	getStringValue( idx_type idx ) const override
			{ return ValueSet::getStringValue( idx ); }
    FilePath		getFilePath( idx_type idx ) const override
			{ return ValueSet::getFilePath( idx ); }

    bool		getBoolValue(const char*) const;
    od_int64		getIntValue(const char*) const;
    double		getDoubleValue(const char*) const;
    BufferString	getStringValue(const char*) const;
    FilePath		getFilePath(const char*) const;
    bool		getGeomID(const char*,Pos::GeomID&) const;
    MultiID		getMultiID(const char*) const;
    bool		get(const char*,::IdxPair&) const;
    bool		get(const char*,Coord&) const;
    bool		get(const char*,Coord3&) const;
    bool		get(const char*,BufferStringSet&) const;
    bool		get(const char*,uiStringSet&) const;
    bool		get(const char*,TypeSet<MultiID>&) const;
    bool		get(const char*,DBKeySet&) const;

    mFloatIntegralNoBoolTemplate(T)
    bool		get(const char*,Interval<T>&) const;
    mFloatIntegralNoBoolTemplate(T)
    bool		get(const char*,TypeSet<T>&) const;
    bool		get(const char*,BoolTypeSet&) const;
    mFloatIntegralTemplate(T)
    bool		get(const char*,Array1D<T>&) const;
    mFloatIntegralTemplate(T)
    bool		get(const char*,Array2D<T>&) const;

    bool		getBoolValue( const OD::String& str ) const
			{ return getBoolValue( str.buf() ); }
    od_int64		getIntValue( const OD::String& str ) const
			{ return getIntValue( str.buf() ); }
    double		getDoubleValue( const OD::String& str ) const
			{ return getDoubleValue( str.buf() ); }
    BufferString	getStringValue( const OD::String& str ) const
			{ return getStringValue( str.buf() ); }
    FilePath		getFilePath( const OD::String& str ) const
			{ return getFilePath( str.buf() ); }

    Array*		set(const char* ky,Array*);
    Object*		set(const char* ky,Object*);

    void		set(const char* ky,bool);
    void		set(const char* ky,od_int16);
    void		set(const char* ky,od_uint16);
    void		set(const char* ky,od_int32);
    void		set(const char* ky,od_uint32);
    void		set(const char* ky,od_int64);
    void		set(const char* ky,float);
    void		set(const char* ky,double);
    void		set(const char* ky,const char*);
    void		set(const char* ky,const OD::String&);
    void		set(const char* ky,const uiString&);
    void		set(const char* ky,const FilePath&);
    void		set(const char* ky,const MultiID&);
    void		set(const char* ky,const DBKey&);
    void		set(const char* ky,const ::IdxPair&);
    void		set(const char* ky,const Coord&);
    void		set(const char* ky,const Coord3&);
    mFloatIntegralNoBoolTemplate(T)
    void		set(const char* ky,const Interval<T>&);
    mFloatIntegralNoBoolTemplate(T)
    void		set(const char* ky,const TypeSet<T>&);
    void		set(const char* ky,const TypeSet<Coord>&);
    void		set(const char* ky,const TypeSet<Coord3>&);
    void		set(const char* ky,const BoolTypeSet&);
    mFloatIntegralTemplate(T)
    void		set(const char* ky,const Array1D<T>&);
    mFloatIntegralTemplate(T)
    void		set(const char* ky,const Array2D<T>&);
    void		set(const char* ky,const BufferStringSet&);
    void		set(const char* ky,const uiStringSet&);
    void		set(const char* ky,const TypeSet<MultiID>&);
    void		set(const char* ky,const DBKeySet&);

    void		remove(const char*);

protected:

    ValueSet*		gtChildByKey(const char*) const;
    Array*		gtArrayByKey(const char*) const;
    Object*		gtObjectByKey(const char*) const;
    ValueSet*		gtChildByKeys(const BufferStringSet&) const;
    Array*		gtArrayByKeys(const BufferStringSet&) const;
    Object*		gtObjectByKeys(const BufferStringSet&) const;

    void		set(KeyedValue*);
    void		setVS(const char*,ValueSet*);
    mFloatIntegralTemplate(T)
    void		setVal(const char*,T);

    friend class	ValueSet;

};


inline Array& ValueSet::asArray()
{ return *static_cast<Array*>( this ); }
inline const Array& ValueSet::asArray() const
{ return *static_cast<const Array*>( this ); }
inline Object& ValueSet::asObject()
{ return *static_cast<Object*>( this ); }
inline const Object& ValueSet::asObject() const
{ return *static_cast<const Object*>( this ); }


template <class T, typename Enable>
Array& Array::setVals( const TypeSet<T>& vals )
{
    return setVals( vals.arr(), vals.size() );
}


template <class T, typename Enable>
Array& Array::setVals( const T* vals, size_type sz )
{
    setEmpty();
    valtype_ = Data;
    delete valarr_;
    if constexpr ( std::is_floating_point<T>::value )
    {
	valarr_ = new ValArr( Number );
	valarr_->vals().setSize( sz );
	if ( typeid(T)==typeid(NumberType) )
	    OD::memCopy( valarr_->vals().arr(), vals, sz*sizeof(T) );
	else
	{
	    for ( int idx=0; idx<sz; idx++ )
	    {
		const NumberType val = mIsUdf(vals[idx])
				     ? mUdf(NumberType)
				     : (NumberType) vals[idx];
		valarr_->vals()[idx] = val;
	    }
	}
    }
    else if constexpr (std::is_same_v<T, BoolType>)
    {
	valarr_ = new ValArr( Boolean );
	valarr_->bools().setSize( sz );
	for ( auto idx=0; idx<sz; idx++ )
	    valarr_->bools()[idx] = vals[idx];
    }
    else if constexpr ( std::is_integral<T>::value )
    {
	valarr_ = new ValArr( INumber );
	valarr_->ivals().setSize( sz );
	if ( typeid(T)==typeid(INumberType) )
	    OD::memCopy( valarr_->ivals().arr(), vals, sz*sizeof(T) );
	else
	{
	    for ( int idx=0; idx<sz; idx++ )
	    {
		const INumberType ival = mIsUdf(vals[idx])
				       ? mUdf(INumberType)
				       : (INumberType) vals[idx];
		valarr_->ivals()[idx] = ival;
	    }
	}
    }
    else
	valarr_ = nullptr;

    return *this;
}


template <class T, typename Enable>
bool Array::get( TypeSet<T>& arr ) const
{
    if ( !isData() && !isMixed() )
	return false;

    const int sz = size();
    if ( !arr.setSize(sz) )
	return false;

    if constexpr ( std::is_floating_point<T>::value )
    {
	for ( int idx=0; idx<sz; idx++ )
	{
	    const double val = getDoubleValue( idx );
	    arr[idx] = mIsUdf(val) ? mUdf(T) : mCast(T,val);
	}
    }
    else if constexpr ( std::is_integral<T>::value )
    {
	for ( int idx=0; idx<sz; idx++ )
	{
	    const od_int64 val = getIntValue( idx );
	    arr[idx] = mIsUdf(val) ? mUdf(T) : mCast(T,val);
	}
    }

    return true;
}


template <class T, typename Enable>
Array& Array::addVals( const TypeSet<T>& vals )
{
    addVals( vals.arr(), vals.size() );
    return *this;
}


template <class T, typename Enable>
Array& Array::addVals( const T* vals, size_type sz )
{
    if ( valType() != SubArray )
	return *this;

    Array* subarr = nullptr;
    if constexpr ( std::is_floating_point<T>::value )
	subarr = new Array( Number );
    else if constexpr (std::is_same_v<T, BoolType>)
	subarr = new Array( Boolean );
    else if constexpr ( std::is_integral<T>::value )
	subarr = new Array( INumber );
    else
	return *this;

    subarr->set( vals, sz );
    add( subarr );

    return *this;
}


template <class T, typename Enable>
bool Object::get( const char* key, Interval<T>& intrvl ) const
{
    const auto* arr = getArray( key );
    const int reqsz = intrvl.hasStep() ? 3 : 2;
    if ( !arr || arr->size() < reqsz )
	return false;

    TypeSet<T> vals;
    if ( !arr->get(vals) || vals.size() < reqsz )
	return false;

    intrvl.set( vals[0], vals[1] );
    if ( reqsz > 2 )
	sCast(StepInterval<T>&,intrvl).step_ = vals[2];

    return true;
}


template <class T, typename Enable>
void Object::set( const char* key, const Interval<T>& intrvl )
{
    Array* arr = nullptr;
    if constexpr ( std::is_floating_point<T>::value )
    {
	arr = new Array( DataType::Number );
	arr->add( intrvl.start_ ).add( intrvl.stop_ );
	if ( intrvl.hasStep() )
	    arr->add( sCast(const StepInterval<T>&,intrvl).step_ );
    }
    else if constexpr ( std::is_integral<T>::value )
    {
	arr = new Array( DataType::INumber );
	arr->add( intrvl.start_ ).add( intrvl.stop_ );
	if ( intrvl.hasStep() )
	    arr->add( sCast(const StepInterval<T>&,intrvl).step_ );
    }

    set( key, arr );
}


template <class T, typename Enable>
bool Object::get( const char* key, TypeSet<T>& arr ) const
{
    const auto* jsarr = getArray( key );
    return jsarr ? jsarr->get( arr ) : false;
}


template <class T, typename Enable>
void Object::set( const char* key, const TypeSet<T>& arr )
{
    Array* jsarr = nullptr;
    if constexpr ( std::is_floating_point<T>::value )
	jsarr = new Array( DataType::Number );
    else if constexpr ( std::is_integral<T>::value )
	jsarr = new Array( DataType::INumber );
    else
	return;

    jsarr->set( arr );
    set( key, jsarr );
}


template <class T, typename Enable>
bool Object::get( const char* key, Array1D<T>& arr ) const
{
    const auto* jsarr = getArray( key );
    if ( !jsarr || (!jsarr->isData() && !jsarr->isMixed()) )
	return false;

    const Array1DInfoImpl info( jsarr->size() );
    if ( arr.info() != info && !arr.setInfo(info) )
	return false;

    const int sz = arr.size();
    if constexpr ( std::is_floating_point<T>::value )
    {
	for ( int idx=0; idx<sz; idx++ )
	{
	    const double val = jsarr->getDoubleValue( idx );
	    if ( mIsUdf(val) )
		arr.set( idx, mUdf(T) );
	    else
		arr.set( idx, mCast(T,val) );
	}
    }
    else if constexpr (std::is_same_v<T, BoolType>)
    {
	for ( int idx=0; idx<sz; idx++ )
	    arr.set( idx, jsarr->getBoolValue(idx) );
    }
    else if constexpr ( std::is_integral<T>::value )
    {
	for ( int idx=0; idx<sz; idx++ )
	{
	    const od_int64 val = jsarr->getIntValue( idx );
	    if ( mIsUdf(val) )
		arr.set( idx, mUdf(T) );
	    else
		arr.set( idx, mCast(T,val) );
	}
    }

    return true;
}


template <class T, typename Enable>
void Object::set( const char* key, const Array1D<T>& arr )
{
    const od_int64 totsz = arr.size();
    if ( totsz > INT_MAX )
	return;

    const int sz = mCast(int,totsz);
    Array* jsarr = nullptr;
    if constexpr ( std::is_floating_point<T>::value )
    {
	jsarr = new Array( DataType::Number );
	if ( arr.getData() && typeid(T)==typeid(NumberType) )
	    jsarr->set( arr.getData(), sz );
	else
	{
	    TypeSet<NumberType> vals( sz );
	    for ( int idx=0; idx<sz; idx++ )
		vals[idx] = arr[idx];
	    jsarr->set( vals );
	}
    }
    else if constexpr (std::is_same_v<T, BoolType>)
    {
	jsarr = new Array( DataType::Boolean );
	if ( arr.getData() && typeid(T)==typeid(BoolType) )
	    jsarr->set( arr.getData(), sz );
	else
	{
	    BoolTypeSet vals;
	    vals.setSize( sz );
	    for ( int idx=0; idx<sz; idx++ )
		vals[idx] = arr[idx];
	    jsarr->set( vals );
	}
    }
    else if constexpr ( std::is_integral<T>::value )
    {
	jsarr = new Array( DataType::INumber );
	if ( arr.getData() && typeid(T)==typeid(INumberType) )
	    jsarr->set( arr.getData(), sz );
	else
	{
	    TypeSet<INumberType> vals( sz );
	    for ( int idx=0; idx<sz; idx++ )
		vals[idx] = arr[idx];
	    jsarr->set( vals );
	}
    }

    set( key, jsarr );
}


template <class T, typename Enable>
bool Object::get( const char* key, Array2D<T>& arr ) const
{
    const auto* jsarr = getArray( key );
    if ( !jsarr || !jsarr->isArray() )
	return false;

    const int nrows = jsarr->size();
    int ncols = 0;
    for ( int irow=0; irow<nrows; irow++ )
    {
	if ( !jsarr->isArrayChild(irow) ||
	     (!jsarr->array(irow).isData() && !jsarr->array(irow).isMixed()) )
	    return false;

	ncols = mMAX( ncols, jsarr->array(irow).size() );
    }

    const Array2DInfoImpl info( nrows, ncols );
    if ( arr.info() != info && !arr.setInfo(info) )
	return false;

    for ( int irow=0; irow<nrows; irow++ )
    {
	const Array& rowjsarr = jsarr->array( irow );
	const int nrcols = rowjsarr.size();
	if constexpr ( std::is_floating_point<T>::value )
	{
	    for ( int icol=0; icol<nrcols; icol++ )
	    {
		const double val = rowjsarr.getDoubleValue( icol );
		if ( mIsUdf(val) )
		    arr.set( irow, icol, mUdf(T) );
		else
		    arr.set( irow, icol, mCast(T,val) );
	    }
	}
	else if constexpr (std::is_same_v<T, BoolType>)
	{
	    for ( int icol=0; icol<nrcols; icol++ )
		arr.set( irow, icol, rowjsarr.getBoolValue(icol) );
	}
	else if constexpr ( std::is_integral<T>::value )
	{
	    for ( int icol=0; icol<nrcols; icol++ )
	    {
		const od_int64 val = rowjsarr.getIntValue( icol );
		if ( mIsUdf(val) )
		    arr.set( irow, icol, mUdf(T) );
		else
		    arr.set( irow, icol, mCast(T,val) );
	    }
	}
    }

    return true;
}

template <class T, typename Enable>
void Object::set( const char* key, const Array2D<T>& arr )
{
    auto* jsarr = new Array( false );
    const int sz1 = arr.getSize(0);
    const int sz2 = arr.getSize(1);
    for ( int irow=0; irow<sz1; irow++ )
    {
	Array* col = nullptr;
	if constexpr ( std::is_floating_point<T>::value )
	{
	    col = new Array( DataType::Number );
	    if ( arr.get2DData()[irow] && typeid(T)==typeid(NumberType) )
		col->set( arr.get2DData()[irow], sz2 );
	    else
	    {
		TypeSet<NumberType> vals( sz2 );
		for ( int icol=0; icol<sz2; icol++ )
		    vals[icol] = arr.get(irow, icol);
		col->set( vals );
	    }
	}
	else if constexpr (std::is_same_v<T, BoolType>)
	{
	    col = new Array( DataType::Boolean );
	    if ( arr.get2DData()[irow] && typeid(T)==typeid(BoolType) )
		col->set( arr.get2DData()[irow], sz2 );
	    else
	    {
		BoolTypeSet vals;
		vals.setSize( sz2 );
		for ( int icol=0; icol<sz2; icol++ )
		    vals[icol] = arr.get(irow, icol);
		col->set( vals );
	    }
	}
	else if constexpr ( std::is_integral<T>::value )
	{
	    col = new Array( DataType::INumber );
	    if ( arr.get2DData()[irow] && typeid(T)==typeid(INumberType) )
		col->set( arr.get2DData()[irow], sz2 );
	    else
	    {
		TypeSet<INumberType> vals( sz2 );
		for ( int icol=0; icol<sz2; icol++ )
		    vals[icol] = arr.get(irow, icol);
		col->set( vals );
	    }
	}

	jsarr->add( col );
    }

    set( key, jsarr );
}

} // namespace JSON

} // namespace OD
