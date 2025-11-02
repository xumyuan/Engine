#pragma once
namespace engine {

	class Singleton {
	public:
		Singleton(const Singleton& singleton) = delete;
		Singleton(const Singleton&& singleton) = delete;

		Singleton& operator=(const Singleton& singleton) = delete;
		Singleton& operator=(const Singleton&& singleton) = delete;
	protected:
		Singleton() = default;
		virtual ~Singleton() = default;
	};

}