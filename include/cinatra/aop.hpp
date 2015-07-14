#pragma once
#include "noncopyable.hpp"

#define HAS_MEMBER(member)\
	template<typename T, typename... Args>struct has_member_##member\
{\
private:\
	template<typename U> static auto Check(int) -> decltype(std::declval<U>().member(std::declval<Args>()...), std::true_type()); \
	template<typename U> static std::false_type Check(...); \
public:\
enum{ value = std::is_same<decltype(Check<T>(0)), std::true_type>::value }; \
}; \

HAS_MEMBER(Foo)
HAS_MEMBER(before)
HAS_MEMBER(after)

template<typename Func, typename... Args>
struct AOP : NonCopyable
{
	AOP(Func&& f) : m_func(std::forward<Func>(f))
	{
	}

	template<typename T>
	typename std::enable_if<has_member_before<T, Args...>::value&&has_member_after<T, Args...>::value>::type invoke(bool& result, Args&&... args, T&& aspect)
	{
		result = aspect.before(std::forward<Args>(args)...);//核心逻辑之前的切面逻辑
		if (result)
		{
			result = m_func(std::forward<Args>(args)...);//核心逻辑
			aspect.after(std::forward<Args>(args)...);//核心逻辑之后的切面逻辑
		}
	}

	template<typename T>
	typename std::enable_if<has_member_before<T, Args...>::value&&!has_member_after<T, Args...>::value>::type invoke(bool& result, Args&&... args, T&& aspect)
	{
		result = aspect.before(std::forward<Args>(args)...);//核心逻辑之前的切面逻辑
		if (result)
			result = m_func(std::forward<Args>(args)...);//核心逻辑
	}

	template<typename T>
	typename std::enable_if<!has_member_before<T, Args...>::value&&has_member_after<T, Args...>::value>::type invoke(bool& result, Args&&... args, T&& aspect)
	{
		result = m_func(std::forward<Args>(args)...);//核心逻辑
		aspect.after(std::forward<Args>(args)...);//核心逻辑之后的切面逻辑
	}

	template<typename T, typename Self>
	typename std::enable_if<has_member_before<T, Args...>::value&&has_member_after<T, Args...>::value>::type invoke_member(bool& result, Self* self, Args&&... args, T&& aspect)
	{
		result = aspect.before(std::forward<Args>(args)...);//核心逻辑之前的切面逻辑
		if (result)
		{
			result = (*self.*m_func)(std::forward<Args>(args)...);//核心逻辑
			aspect.after(std::forward<Args>(args)...);//核心逻辑之后的切面逻辑
		}
	}

	//template<typename Head, typename... Tail>
	//void invoke(bool& result, Args&&... args, Head&&headAspect, Tail&&... tailAspect)
	//{
	//	headAspect.before(std::forward<Args>(args)...);
	//	invoke(result, std::forward<Args>(args)..., std::forward<Tail>(tailAspect)...);
	//	headAspect.after(std::forward<Args>(args)...);
	//}

private:
	Func m_func; //被织入的函数
};
template<typename T> using identity_t = T;

//AOP的辅助函数，简化调用
template<typename... AP, typename... Args, typename Func>
typename std::enable_if<!std::is_member_function_pointer<Func>::value>::type invoke(bool& result, Func&&f, Args&&... args)
{
	AOP<Func, Args...> asp(std::forward<Func>(f));
	asp.invoke(result, std::forward<Args>(args)..., identity_t<AP>()...);
}

template<typename... AP, typename... Args, typename Func, typename Self>
typename std::enable_if<std::is_member_function_pointer<Func>::value>::type invoke(bool& result, Func&&f, Self* self, Args&&... args)
{
	AOP<Func, Args...> asp(std::forward<Func>(f));
	asp.invoke_member(result, self, std::forward<Args>(args)..., identity_t<AP>()...);
}