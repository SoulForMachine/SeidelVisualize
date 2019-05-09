#ifndef _BASE_LIB_INTERFACE_POOL_ALLOCATOR_H_
#define _BASE_LIB_INTERFACE_POOL_ALLOCATOR_H_

#include <type_traits>
#include <cassert>

#include "VarPoolAllocator.h"


namespace BaseLib
{

//
// Memory pool for allocating single objects of same type. Uses a linked list of free objects.
// Allocation and deallocation is allowed in any order.
//
template <typename _Type>
class PoolAllocator : private VarPoolAllocator<sizeof(_Type), alignof(_Type)>
{
public:
	explicit PoolAllocator(size_t size, void* mem = nullptr)
		: VarPoolAllocator<_TypeSize, _Alignment> { size, mem }
	{ }

	PoolAllocator(const PoolAllocator&) = delete;
	PoolAllocator(PoolAllocator&&) = delete;
	~PoolAllocator() = default;

	PoolAllocator& operator = (const PoolAllocator&) = delete;
	PoolAllocator& operator = (PoolAllocator&&) = delete;

	_Type* New()	{ return VarPoolAllocator<_TypeSize, _Alignment>::New<_Type>(); }

	template <typename... _ArgTs>
	_Type* New(_ArgTs&&... args)	{ return VarPoolAllocator<_TypeSize, _Alignment>::New<_Type>(std::forward<_ArgTs>(args)...); }

	void Delete(_Type* ptr)	{ return VarPoolAllocator<_TypeSize, _Alignment>::Delete(ptr); }

	size_t GetFreeCount()	{ return VarPoolAllocator<_TypeSize, _Alignment>::GetFreeCount(); }
	size_t GetAllocCount()	{ return VarPoolAllocator<_TypeSize, _Alignment>::GetAllocCount(); }
	size_t TotalCapacity()	{ return VarPoolAllocator<_TypeSize, _Alignment>::TotalCapacity(); }

	// WARNING: this function quickly reclaims all allocated objects;
	// it should be used only if objects with trivial destructors are
	// allocated, after making sure they are not in use any more.
	void Reset()
	{
		assert(std::is_trivially_destructible_v<_Type>);
		return VarPoolAllocator<_TypeSize, _Alignment>::Reset();
	}

protected:
	static constexpr size_t _TypeSize = sizeof(_Type);
	static constexpr size_t _Alignment = alignof(_Type);
};


template <typename _Type, size_t _Size>
class InplacePoolAllocator : public PoolAllocator<_Type>
{
public:
	static_assert(_Size > 0, "Size must be greater than 0.");

	InplacePoolAllocator()
		: PoolAllocator<_Type> { _Size, _mem }
	{ }

	~InplacePoolAllocator() = default;

private:
	std::aligned_storage_t<_TypeSize, _Alignment> _mem[_Size];
};

} // namespace BaseLib

#endif //_BASE_LIB_INTERFACE_POOL_ALLOCATOR_H_
