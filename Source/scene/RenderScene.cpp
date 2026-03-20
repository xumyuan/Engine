#include "pch.h"
#include "RenderScene.h"

#include "scene/RenderableModel.h"
#include "graphics/renderer/ModelRenderer.h"

namespace engine {

	void RenderScene::submitModelsToRenderer() const {
		if (!modelRenderer || !renderableModels) return;

		for (auto* model : *renderableModels) {
			if (model->getTransparent()) {
				modelRenderer->submitTransparent(model);
			}
			else {
				modelRenderer->submitOpaque(model);
			}
		}
	}

}
