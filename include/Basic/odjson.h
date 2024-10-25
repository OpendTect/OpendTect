#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "filepath.h"
#include "od_iosfwd.h"
#include "typeset.h"
#include "uistringset.h"
#include "arraynd.h"

class DBKeySet;
class DBKey;
class SeparString;
class StringBuilder;
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
    Boolean, Number, String
};

typedef double NumberType;


/*! holds 'flat' value sets of each of the DataType's */

mExpClass(Basic) ValArr
{
public:

    typedef BoolTypeSet::size_type	size_type;
    typedef size_type			idx_type;
    typedef BoolTypeSet			BSet;
    typedef TypeSet<NumberType>		NSet;
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
    void		setFilePath(const FilePath&, idx_type idx);
    FilePath		getFilePath(idx_type idx) const;

    OD::Set&		odSet()			{ return *set_; }
    const OD::Set&	odSet() const		{ return *set_; }
    BSet&		bools()			{ return *((BSet*)set_); }
    const BSet&		bools() const		{ return *((BSet*)set_); }
    NSet&		vals()			{ return *((NSet*)set_); }
    const NSet&		vals() const		{ return *((NSet*)set_); }
    SSet&		strings()		{ return *((SSet*)set_); }
    const SSet&		strings() const		{ return *((SSet*)set_); }

    void		dumpJSon(BufferString&) const;
    void		dumpJSon(StringBuilder&) const;
    BufferString	dumpJSon() const;

protected:

    DataType		type_ = Boolean;
    OD::Set*		set_ = nullptr;

private:

			ValArr()	 = delete;

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

    virtual ValueType		valueType(idx_type) const;
    const BufferString& 	key(idx_type) const;
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

    uiRetVal			parseJSon(char* buf,int bufsz);
    static ValueSet*		getFromJSon(char* buf,int bufsz,uiRetVal&);
    void			dumpJSon(BufferString&,bool pretty=false) const;
    void			dumpJSon(StringBuilder&) const;
    BufferString		dumpJSon(bool pretty=false) const;

    uiRetVal			read(od_istream&);
    static ValueSet*		read(od_istream&,uiRetVal&);
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

    static ValueSet*		gtByParse(char*,int,uiRetVal&,ValueSet*);
    void			use(const GasonNode&);

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

			Array(bool objects,ValueSet* p=0);
			Array(DataType,ValueSet* p=0);
			Array(const Array&);
			~Array();

    Array&		operator=(const Array&)	= delete;
    Array*		clone() const override	{ return new Array(*this); }
    bool		isArray() const override	{ return true; }
    void		setEmpty() override;
    bool		isEmpty() const override;
    bool		isData() const;

    ValueType		valueType(idx_type) const override
			{ return valtype_; }
    ValueType		valType() const		{ return valtype_; }
    size_type		size() const override;

			// Only available if valType() == Data
    inline ValArr&	valArr()		{ return *valarr_; }
    inline const ValArr& valArr() const		{ return *valarr_; }

    Array*		add(Array*);
    Object*		add(Object*);

    bool		getBoolValue(idx_type) const override;
    od_int64		getIntValue(idx_type) const override;
    double		getDoubleValue(idx_type) const override;
    BufferString	getStringValue(idx_type) const override;
    FilePath		getFilePath(idx_type) const override;

			// only usable if valType() == Data
#   define		mDeclJSONArraySetFn( typ ) \
    Array&		set(typ)

#   define		mDeclJSONArrayAddAndSetFn( typ ) \
    Array&		add(typ); \
			mDeclJSONArraySetFn(typ)

#   define		mDeclJSONArraySetFns( typ ) \
			mDeclJSONArrayAddAndSetFn(typ); \
    Array&		set(const typ*,size_type); \
    Array&		set(const TypeSet<typ>&)

			mDeclJSONArrayAddAndSetFn(const char*);
			mDeclJSONArrayAddAndSetFn(const DBKey&);
			mDeclJSONArrayAddAndSetFn(const MultiID&);
			mDeclJSONArrayAddAndSetFn(const uiString&);
			mDeclJSONArrayAddAndSetFn(const OD::String&);
			mDeclJSONArrayAddAndSetFn(const FilePath&);
			mDeclJSONArrayAddAndSetFn(bool);

			mDeclJSONArraySetFn(const BufferStringSet&);
			mDeclJSONArraySetFn(const DBKeySet&);
			mDeclJSONArraySetFn(const uiStringSet&);
			mDeclJSONArraySetFn(const BoolTypeSet&);
    Array&		set(const bool*,size_type);
			mDeclJSONArraySetFns(od_int16);
			mDeclJSONArraySetFns(od_uint16);
			mDeclJSONArraySetFns(od_int32);
			mDeclJSONArraySetFns(od_uint32);
			mDeclJSONArraySetFns(od_int64);
			mDeclJSONArraySetFns(float);
			mDeclJSONArraySetFns(double);

protected:

    ValueType		valtype_;
    ValArr*		valarr_;

    template <class T>
    Array&		setVals(const TypeSet<T>&);
    template <class T>
    Array&		setVals(const T*,size_type);
    void		addVS(ValueSet*);

    friend class	ValueSet;

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
    bool		getStrings(const char*,BufferStringSet&) const;
    bool		getGeomID(const char*,Pos::GeomID&) const;
    MultiID		getMultiID(const char*) const;
    template <class T>
    bool		get(const char*,Interval<T>&) const;
    template <class T>
    bool		get(const char*,Array1D<T>&) const;
    template <class T>
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
    void		set( const char* ky, const OD::String& str )
			{ set( ky, str.str() ); }
    void		set(const char* ky,const FilePath&);
    void		set(const char* ky,const DBKey&);
    void		set(const char* ky,const MultiID&);
    void		set(const char* ky,const uiString&);
    template <class T>
    void		set(const char* ky,const Interval<T>&);
    template <class T>
    void		set(const char* ky,const Array1D<T>&);
    template <class T>
    void		set(const char* ky,const Array2D<T>&);

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
    template <class T>
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


template <class T>
inline bool OD::JSON::Object::get( const char* key, Interval<T>& intrvl ) const
{
    const auto* arr = getArray( key );
    if ( !arr )
	return false;
    const TypeSet<NumberType> intrvals = arr->valArr().vals();

    intrvl.start_ = intrvals[0];
    intrvl.stop_ = intrvals[1];

    if ( intrvl.hasStep() )
    {
	mDynamicCastGet(StepInterval<T>*,si,&intrvl)
	    si->step_ = intrvals[2];
    }
    return true;
}


template <class T>
inline void OD::JSON::Object::set( const char* key, const Interval<T>& intrvl )
{
    auto* arr = new Array(DataType::Number);
    TypeSet<NumberType> intrvals;
    intrvals.add( intrvl.start_ ).add( intrvl.stop_ );

    if ( intrvl.hasStep() )
    {
	mDynamicCastGet(const StepInterval<T>*,si,&intrvl)
	    intrvals.add( si->step_ );
    }

    arr->set( intrvals );
    set( key, arr );
}


template <class T>
inline bool OD::JSON::Object::get( const char* key, Array1D<T>& arr ) const
{
    const auto* jsarr = getArray( key );
    if ( !jsarr || !jsarr->isData() )
	return false;

    const Array1DInfoImpl info( jsarr->size() );
    if ( arr.info() != info && !arr.setInfo(info) )
	return false;

    const TypeSet<NumberType>& vals = jsarr->valArr().vals();
    for ( int idx=0; idx<vals.size(); idx++ )
	arr.set( idx, vals[idx] );

    return true;
}

template <class T>
inline void OD::JSON::Object::set( const char* key, const Array1D<T>& arr )
{
    auto* jsarr = new Array( DataType::Number );
    if ( arr.getData() && typeid(T)==typeid(NumberType) )
	jsarr->set( arr.getData(), arr.size() );
    else
    {
	TypeSet<NumberType> vals;
	for ( int idx=0; idx<arr.size(); idx++ )
	    vals.add( arr[idx] );

	jsarr->set( vals );
    }

    set( key, jsarr );
}


template <class T>
inline bool OD::JSON::Object::get( const char* key, Array2D<T>& arr ) const
{
    const auto* jsarr = getArray( key );
    if ( !jsarr )
	return false;

    const int nrows = jsarr->size();
    int ncols = 0;
    for ( int irow=0; irow<nrows; irow++ )
    {
	if ( !jsarr->isArrayChild(irow) || !jsarr->array(irow).isData() )
	    return false;

	ncols = mMAX( ncols, jsarr->array(irow).size() );
    }

    const Array2DInfoImpl info( nrows, ncols );
    if ( arr.info() != info && !arr.setInfo(info) )
	return false;

    for ( int irow=0; irow<nrows; irow++ )
    {
	const TypeSet<NumberType>& vals = jsarr->array(irow).valArr().vals();
	for ( int icol=0; icol<vals.size(); icol++ )
	    arr.set( irow, icol, vals[icol] );
    }

    return true;
}


template <class T>
inline void OD::JSON::Object::set( const char* key, const Array2D<T>& arr )
{
    auto* jsarr = new Array( true );
    for ( int irow=0; irow<arr.getSize(0); irow++ )
    {
	auto* col = new Array( DataType::Number );
	if ( arr.get2DData()[irow] && typeid(T)==typeid(NumberType) )
	    col->set( arr.get2DData()[irow], arr.getSize(1) );
	else
	{
	    TypeSet<NumberType> vals;
	    for ( int icol=0; icol<arr.getSize(1); icol++ )
		vals.add( arr.get(irow, icol) );

	    col->set( vals );
	}
	jsarr->add( col );
    }

    set( key, jsarr );
}

} // namespace JSON

} // namespace OD
