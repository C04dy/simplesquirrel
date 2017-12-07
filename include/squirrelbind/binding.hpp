#pragma once
#ifndef SQUIRREL_BIND_BINDING_HEADER_H
#define SQUIRREL_BIND_BINDING_HEADER_H

#include "allocators.hpp"
#include <functional>
#include <cstring>

namespace SquirrelBind {
    namespace detail {
		// function_traits and make_function credits by @tinlyx https://stackoverflow.com/a/21665705

		// For generic types that are functors, delegate to its 'operator()'
		template <typename T>
		struct function_traits
			: public function_traits<decltype(&T::operator())>
		{};

		// for pointers to member function
		template <typename ClassType, typename ReturnType, typename... Args>
		struct function_traits<ReturnType(ClassType::*)(Args...) const> {
			//enum { arity = sizeof...(Args) };
			typedef std::function<ReturnType (Args...)> f_type;
		};

		// for pointers to member function
		template <typename ClassType, typename ReturnType, typename... Args>
		struct function_traits<ReturnType(ClassType::*)(Args...) > {
			typedef std::function<ReturnType (Args...)> f_type;
		};

		// for function pointers
		template <typename ReturnType, typename... Args>
		struct function_traits<ReturnType (*)(Args...)>  {
			typedef std::function<ReturnType (Args...)> f_type;
		};

		template <typename L> 
		inline typename function_traits<L>::f_type make_function(L l){
			return static_cast<typename function_traits<L>::f_type>(l);
		}

        template <typename A>
        static void paramPackerType(char* ptr) {
            *ptr = '.';
        }

        template <typename ...B>
        static void paramPacker(char* ptr) {
            int _[] = { 0, (paramPackerType<B>(ptr++), 0)... };
            (void)_;
            *ptr = '\0';
        }

        template<typename Ret, typename... Args>
        static void bindUserData(HSQUIRRELVM vm, const std::function<Ret(Args...)>& func) {
            auto funcStruct = reinterpret_cast<detail::FuncPtr<Ret(Args...)>*>(sq_newuserdata(vm, sizeof(detail::FuncPtr<Ret(Args...)>)));
            funcStruct->ptr = new std::function<Ret(Args...)>(func);
            sq_setreleasehook(vm, -1, &detail::funcReleaseHook<Ret, Args...>);
        }

        template<typename T, typename... Args>
        static SqObject addClass(HSQUIRRELVM vm, const char* name, const std::function<T*(Args...)>& allocator) {
            static const std::size_t nparams = sizeof...(Args);

            SqObject clsObj(vm);
            
            sq_pushstring(vm, name, strlen(name));
            sq_newclass(vm, false);

            HSQOBJECT& obj = T::sqGetClass();
            sq_getstackobj(vm, -1, &obj);
            //sq_addref(vm, &obj);

            sq_getstackobj(vm, -1, &clsObj.get());
            sq_addref(vm, &clsObj.get());

            sq_settypetag(vm, -1, reinterpret_cast<SQUserPointer>(typeid(T).hash_code()));

            sq_pushstring(vm, "constructor", -1);
            bindUserData<T*>(vm, allocator);
            static char params[33];
            paramPacker<T*, Args...>(params);

            sq_newclosure(vm, &detail::classAllocator<T, Args...>, 1);

            sq_setparamscheck(vm, nparams + 1, params);
            sq_newslot(vm, -3, false); // Add the constructor method

            sq_newslot(vm, -3, SQFalse); // Add the class

            return clsObj;
        }

		template<int offet, typename R, typename... Args>
		struct func {
			static SQInteger global(HSQUIRRELVM vm) {
				try {
					static const std::size_t nparams = sizeof...(Args);

					FuncPtr<R(Args...)>* funcPtr;
					sq_getuserdata(vm, -1, reinterpret_cast<void**>(&funcPtr), nullptr);

					int index = nparams + offet;
					push(vm, std::forward<R>(funcPtr->ptr->operator()(pop<typename std::remove_reference<Args>::type>(vm, index--)...)));
					return 1;
				} catch (std::exception& e) {
					return sq_throwerror(vm, e.what());
				}
			}
		};

		template<int offet, typename... Args>
		struct func<offet, void, Args...> {
			static SQInteger global(HSQUIRRELVM vm) {
				try {
					static const std::size_t nparams = sizeof...(Args);

					FuncPtr<void(Args...)>* funcPtr;
					sq_getuserdata(vm, -1, reinterpret_cast<void**>(&funcPtr), nullptr);

					int index = nparams + offet;
					funcPtr->ptr->operator()(pop<typename std::remove_reference<Args>::type>(vm, index--)...);
					return 0;
				} catch (std::exception& e) {
					return sq_throwerror(vm, e.what());
				}
			}
		};

		template<typename R, typename... Args>
		static void addFunc(HSQUIRRELVM vm, const char* name, const std::function<R(Args...)>& func) {
			static const std::size_t nparams = sizeof...(Args);

			sq_pushstring(vm, name, strlen(name));

			bindUserData(vm, func);
			static char params[33];
			paramPacker<void, Args...>(params);

			sq_newclosure(vm, &detail::func<1, R, Args...>::global, 1);
			sq_setparamscheck(vm, nparams + 1, params);
			if(SQ_FAILED(sq_newslot(vm, -3, SQFalse))) {
				throw SqTypeException("Failed to bind function");
			}
		}

		template<typename R, typename... Args>
		static void addMemberFunc(HSQUIRRELVM vm, const char* name, const std::function<R(Args...)>& func) {
			static const std::size_t nparams = sizeof...(Args);

			sq_pushstring(vm, name, strlen(name));

			bindUserData(vm, func);
			static char params[33];
			paramPacker<Args...>(params);

			sq_newclosure(vm, &detail::func<0, R, Args...>::global, 1);
			sq_setparamscheck(vm, nparams, params);
			if(SQ_FAILED(sq_newslot(vm, -3, SQFalse))) {
				throw SqTypeException("Failed to bind member function");
			}
		}
    }
}

#endif