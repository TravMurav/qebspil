// SPDX-License-Identifier: GPL-2.0-only

#include <string.h>

#include "qebspil.h"

static const struct pil_type pil_types[] = {
	{
		.compatible = "qcom,sc7180-adsp-pas",
		.id[PIL_MAIN].full = 1
	},

	{
		.compatible = "qcom,sc8280xp-adsp-pas",
		.id[PIL_MAIN].full = 1,
	},
	{
		.compatible = "qcom,sc8280xp-slpi-pas",
		.id[PIL_MAIN].full = 12,
	},
	{
		.compatible = "qcom,sc8280xp-nsp0-pas",
		.id[PIL_MAIN].full = 18,
	},
	{
		.compatible = "qcom,sc8280xp-nsp1-pas",
		.id[PIL_MAIN].full = 30,
	},

	{
		.compatible = "qcom,x1e80100-adsp-pas",
		.id[PIL_MAIN] = {
			.full = 1,
			.lite = 31,
		},
		.id[PIL_DTB] = {
			.full = 36,
			.lite = 41,
		},
	},
	{
		.compatible = "qcom,x1e80100-cdsp-pas",
		.id[PIL_MAIN].full = 18,
		.id[PIL_DTB].full = 37,
	},
};

const struct pil_type *pil_types_find(const char *compatible)
{
	for (size_t i = 0; i < ARRAY_SIZE(pil_types); i++)
		if (strcmp(compatible, pil_types[i].compatible) == 0)
			return &pil_types[i];

	return NULL;
}
