#pragma once

#include <patcher/interface.h>

namespace micro_profiler
{
	namespace tests
	{
		namespace mocks
		{
			class patch_manager : public micro_profiler::patch_manager
			{
			public:
				std::function<void (std::vector<unsigned int /*rva of installed*/> &result, unsigned int persistent_id)> on_query;
				std::function<void (std::vector<unsigned int /*rva*/> &failures, unsigned int persistent_id, void *base,
					std::shared_ptr<void> lock, range<const unsigned int /*rva*/, size_t> functions)> on_apply;
				std::function<void (std::vector<unsigned int /*rva*/> &failures, unsigned int persistent_id,
					range<const unsigned int /*rva*/, size_t> functions)> on_revert;

			private:
				virtual void query(std::vector<unsigned int /*rva of installed*/> &result, unsigned int persistent_id) override;
				virtual void apply(std::vector<unsigned int /*rva*/> &failures, unsigned int persistent_id, void *base,
					std::shared_ptr<void> lock, range<const unsigned int /*rva*/, size_t> functions) override;
				virtual void revert(std::vector<unsigned int /*rva*/> &failures, unsigned int persistent_id,
					range<const unsigned int /*rva*/, size_t> functions) override;
			};



			inline void patch_manager::query(std::vector<unsigned int> &result, unsigned int persistent_id)
			{	on_query(result, persistent_id);	}

			inline void patch_manager::apply(std::vector<unsigned int> &failures, unsigned int persistent_id, void *base,
				std::shared_ptr<void> lock, range<const unsigned int, size_t> functions)
			{	on_apply(failures, persistent_id, base, lock, functions);	}

			inline void patch_manager::revert(std::vector<unsigned int> &failures, unsigned int persistent_id,
				range<const unsigned int, size_t> functions)
			{	on_revert(failures, persistent_id, functions);	}
		}
	}
}
