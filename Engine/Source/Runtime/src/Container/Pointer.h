#pragma once
#include <memory>

namespace Gleam {

template<class T>
class UniquePtr
{
public:

	template<class T, class ... Args>
	static constexpr UniquePtr<T> Create(Args&& ... args)
	{
		return std::make_unique<T>(std::forward<Args>(args)...);
	}

	T* Raw() const noexcept
	{
		return m_Ptr.get();
	}

	operator std::unique_ptr<T>() const noexcept
	{
		return m_Ptr;
	}

private:

	std::unique_ptr<T> m_Ptr;
};


template<class T>
class SharedPtr
{
public:

	template<class T, class ... Args>
	static constexpr SharedPtr<T> Create(Args&& ... args)
	{
		return std::make_shared<T>(std::forward<Args>(args)...);
	}

	T* Raw() const noexcept
	{
		return m_Ptr.get();
	}

	operator std::shared_ptr<T>() const noexcept
	{
		return m_Ptr;
	}

private:

	std::shared_ptr<T> m_Ptr;
};

template<class T>
class WeakPtr
{
public:

	T* Raw() const noexcept
	{
		return m_Ptr.get();
	}

	operator std::weak_ptr<T>() const noexcept
	{
		return m_Ptr;
	}

private:

	std::weak_ptr<T> m_Ptr;
};

}