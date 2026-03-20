#pragma once
namespace engine {

	/// CRTP 模板单例基类
	/// 使用方式: class MyClass : public Singleton<MyClass> { friend class Singleton<MyClass>; private: MyClass() = default; };
	template<typename T>
	class Singleton {
	public:
		static T& getInstance() {
			static T instance;
			return instance;
		}

		Singleton(const Singleton&) = delete;
		Singleton(Singleton&&) = delete;
		Singleton& operator=(const Singleton&) = delete;
		Singleton& operator=(Singleton&&) = delete;

	protected:
		Singleton() = default;
		~Singleton() = default;
	};

}