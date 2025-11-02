#pragma once

#if _DEBUG
#define BEGIN_EVENT(eventname) pushDebugGroup(eventname);
#define END_EVENT() popDebugGroup();
#else
#define BEGIN_EVENT(eventname) 
#define END_EVENT() 
#endif

namespace engine {
	// 设置调试组标记（兼容不同版本）
	void pushDebugGroup(const std::string& name);

	void popDebugGroup();
}// engine


