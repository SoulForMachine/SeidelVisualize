#ifndef _BASE_LIB_INTERFACE_VAR_FREE_LIST_ALLOCATOR_H_
#define _BASE_LIB_INTERFACE_VAR_FREE_LIST_ALLOCATOR_H_

#include <cstdlib>
#include <cstdint>
#include <memory>
#include <type_traits>
#include <utility>
#include <cstdint>
#include <cassert>

#include "Common.h"


namespace BaseLib
{

//
// Memory pool for allocating single objects of variable sizes, up to some maximum size. Uses a linked list of free objects.
// Allocation and deallocation is allowed in any order.
//
template <size_t _MaxObjSize, size_t _Alignment>
class VarPoolAllocator
{
public:
	explicit VarPoolAllocator(size_t objectCount, void* mem = nullptr);
	VarPoolAllocator(const VarPoolAllocator&) = delete;
	VarPoolAllocator(VarPoolAllocator&&) = delete;
	~VarPoolAllocator();

	VarPoolAllocator& operator = (const VarPoolAllocator&) = delete;
	VarPoolAllocator& operator = (VarPoolAllocator&&) = delete;

	template <typename _Type>
	_Type* New();

	template <typename _Type, typename... _ArgTs>
	_Type* New(_ArgTs&&... args);

	template <typename _Type>
	void Delete(_Type* ptr);

	size_t GetFreeCount() { return _objectCount - _allocCount; }
	size_t GetAllocCount() { return _allocCount; }
	size_t TotalCapacity() { return _objectCount; }

	void Reset();

private:
	union FreeListBlock
	{
		std::aligned_storage_t<_MaxObjSize, _Alignment> blockStorage;
		FreeListBlock* next;
	};

	template <typename _Type>
	constexpr void CheckType()
	{
		static_assert(sizeof(_Type) <= _MaxObjSize, "Size of _Type must be at most _MaxObjSize.");
		static_assert(alignof(_Type) <= _Alignment, "Alignment of _Type must be at most _Alignment.");
	}

	void* AllocateOne();
	void ResetInternal();

	FreeListBlock* _buffer;
	FreeListBlock* _freeList;
	size_t _allocCount;
	size_t _objectCount;
	bool _ownBuffer;
	bool _onlyTrivialDestr;
};


template <size_t _MaxObjSize, size_t _Alignment, size_t _ObjCount>
class InplaceVarPoolAllocator : public VarPoolAllocator<_MaxObjSize, _Alignment>
{
public:
	static_assert(_ObjCount > 0, "Object count must be greater than 0.");

	InplaceVarPoolAllocator()
		: VarPoolAllocator<_MaxObjSize, _Alignment> { _ObjCount, _mem }
	{ }

	~InplaceVarPoolAllocator() = default;

private:
	std::aligned_storage_t<_MaxObjSize, _Alignment> _mem[_ObjCount];
};


template <size_t _MaxObjSize, size_t _Alignment>
inline VarPoolAllocator<_MaxObjSize, _Alignment>::VarPoolAllocator(size_t objectCount, void* mem)
	: _allocCount { 0 }
	, _objectCount { objectCount }
	, _onlyTrivialDestr { true }
{
	if (mem == nullptr)
	{
		_buffer = new FreeListBlock[objectCount];
		_ownBuffer = true;
	}
	else
	{
		assert(BaseLib::RoundUp(mem, _Alignment) == mem);
		_buffer = reinterpret_cast<FreeListBlock*>(mem);
		_ownBuffer = false;
	}

	ResetInternal();
}

template <size_t _MaxObjSize, size_t _Alignment>
inline VarPoolAllocator<_MaxObjSize, _Alignment>::~VarPoolAllocator()
{
	if (_ownBuffer)
		delete[] _buffer;
}

template <size_t _MaxObjSize, size_t _Alignment>
template <typename _Type>
inline _Type* VarPoolAllocator<_MaxObjSize, _Alignment>::New()
{
	CheckType<_Type>();

	if (std::is_trivially_destructible_v<_Type> == false)
		_onlyTrivialDestr = false;

	void* block = AllocateOne();
	_Type* obj = nullptr;

	if (block)
		obj = new(block) _Type;

	return obj;
}

template <size_t _MaxObjSize, size_t _Alignment>
template <typename _Type, typename... _ArgTs>
inline _Type* VarPoolAllocator<_MaxObjSize, _Alignment>::New(_ArgTs&&... args)
{
	CheckType<_Type>();

	if (std::is_trivially_destructible_v<_Type> == false)
		_onlyTrivialDestr = false;

	void* block = AllocateOne();
	_Type* obj = nullptr;

	if (block)
		obj = new(block) _Type { std::forward<_ArgTs>(args)... };

	return obj;
}

template <size_t _MaxObjSize, size_t _Alignment>
template <typename _Type>
inline void VarPoolAllocator<_MaxObjSize, _Alignment>::Delete(_Type* ptr)
{
	CheckType<_Type>();

	if (ptr != nullptr)
	{
		ptr->~_Type();
		auto unconst = const_cast<std::remove_const_t<_Type>*>(ptr);
		FreeListBlock* block = reinterpret_cast<FreeListBlock*>(unconst);
		block->next = _freeList;
		_freeList = block;
		--_allocCount;

		if (_allocCount == 0)
			_onlyTrivialDestr = true;
	}
}

// WARNING: this function quickly reclaims all allocated objects;
// it should be used only if objects with trivial destructors are
// allocated, after making sure they are not in use any more.
template <size_t _MaxObjSize, size_t _Alignment>
inline void VarPoolAllocator<_MaxObjSize, _Alignment>::Reset()
{
	assert(_onlyTrivialDestr == true);

	if (_allocCount > 0 && _onlyTrivialDestr)
		ResetInternal();
}

template <size_t _MaxObjSize, size_t _Alignment>
inline void* VarPoolAllocator<_MaxObjSize, _Alignment>::AllocateOne()
{
	if (_freeList)
	{
		++_allocCount;
		void* block = &_freeList->blockStorage;
		_freeList = _freeList->next;
		return block;
	}
	else
	{
		assert(!"Out of memory");
		return nullptr;
	}
}

template <size_t _MaxObjSize, size_t _Alignment>
void VarPoolAllocator<_MaxObjSize, _Alignment>::ResetInternal()
{
	_allocCount = 0;
	_freeList = &_buffer[0];

	for (size_t i = 0; i < _objectCount - 1; ++i)
		_freeList[i].next = &_freeList[i + 1];

	_freeList[_objectCount - 1].next = nullptr;
}

} // namespace BaseLib

#endif //_BASE_LIB_INTERFACE_VAR_FREE_LIST_ALLOCATOR_H_
