#include "sptlib-stdafx.hpp"

#include "DetoursUtils.hpp"
#include <cassert>
namespace DetoursUtils
{
	using namespace std;

	static map<void*, void*> tramp_to_original;
	static mutex tramp_to_original_mutex;

	void AttachDetours(const wstring& moduleName, size_t n, const pair<void**, void*> funcPairs[])
	{
		if (!g_pHooks)
		{
			g_pHooks = funchook_create();
		}

		size_t hook_count = 0;
		for (size_t i = 0; i < n; ++i)
		{
			void** target = funcPairs[i].first;
			void* detour = funcPairs[i].second;
			assert(target);

			if (*target && detour)
			{
				void* original = *target;
				auto status = funchook_prepare(g_pHooks, target, original);
				detour = original;
				if (status != FUNCHOOK_ERROR_SUCCESS)
				{
					continue;
				}

				{
					lock_guard<mutex> lock(tramp_to_original_mutex);
					tramp_to_original[*target] = original;
				}
				hook_count++;
			}
		}

		funchook_install(g_pHooks, 0);

		if (hook_count == 0)
		{
			return;
		}
	}

	void DetachDetours(const wstring& moduleName, size_t n, void** const functions[])
	{
		size_t hook_count = 0;
		for (size_t i = 0; i < n; ++i)
		{
			void** tramp = functions[i];
			assert(tramp);

			if (*tramp)
			{
				void* original;
				{
					lock_guard<mutex> lock(tramp_to_original_mutex);
					original = tramp_to_original[*tramp];
					tramp_to_original.erase(*tramp);
				}

				*tramp = original;
				hook_count++;
			}
		}

		funchook_uninstall(g_pHooks, 0);

		if (!hook_count)
		{
			return;
		}
	}
}