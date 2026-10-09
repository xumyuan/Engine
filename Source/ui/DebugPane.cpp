#include "pch.h"
#include "DebugPane.h"

#include <algorithm>

namespace engine {

	namespace {

		struct DebugSection {
			int id = 0;
			int order = 0;
			bool openByDefault = false;
			std::string name;
			DebugPane::DrawFn draw;
		};

		std::vector<DebugSection> g_Sections;
		int g_NextSectionId = 0;

	}

	bool DebugPane::s_WireframeMode = false;
	bool DebugPane::s_LightMarkersEnabled = true;

	DebugPane::DebugPane(const glm::vec2& panePosition) : Pane(std::string("Debug Controls"), panePosition)
	{
	}

	int DebugPane::addSection(const char* name, DrawFn draw, int order, bool openByDefault) {
		DebugSection section;
		section.id = ++g_NextSectionId;
		section.order = order;
		section.openByDefault = openByDefault;
		section.name = name ? name : "";
		section.draw = std::move(draw);
		const int id = section.id;

		auto it = std::upper_bound(g_Sections.begin(), g_Sections.end(), order,
			[](int value, const DebugSection& existing) { return value < existing.order; });
		g_Sections.insert(it, std::move(section));
		return id;
	}

	void DebugPane::removeSection(int id) {
		if (id == 0)
			return;
		std::erase_if(g_Sections, [id](const DebugSection& section) { return section.id == id; });
	}

	void DebugPane::setupPaneObjects() {
		if (ImGui::CollapsingHeader("View", ImGuiTreeNodeFlags_DefaultOpen)) {
			ImGui::Checkbox("Light markers", &s_LightMarkersEnabled);
#if DEBUG_ENABLED
			ImGui::Checkbox("Wireframe Mode", &s_WireframeMode);
			ImGui::Text("Hit \"P\" to show/hide the cursor");
#endif
		}

		for (const DebugSection& section : g_Sections) {
			ImGui::PushID(section.id);
			const ImGuiTreeNodeFlags flags = section.openByDefault ? ImGuiTreeNodeFlags_DefaultOpen : 0;
			if (ImGui::CollapsingHeader(section.name.c_str(), flags) && section.draw)
				section.draw();
			ImGui::PopID();
		}
	}

}
