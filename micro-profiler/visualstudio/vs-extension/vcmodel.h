#pragma once

#include <functional>
#include <memory>
#include <string>

struct IDispatch;

namespace micro_profiler
{
	namespace vcmodel
	{
		struct compiler_tool;
		struct configuration;
		struct file;
		struct file_configuration;
		struct librarian_tool;
		struct linker_tool;
		struct project;
		struct tool;
		struct tool_visitor;

		typedef std::shared_ptr<compiler_tool> compiler_tool_ptr;
		typedef std::shared_ptr<configuration> configuration_ptr;
		typedef std::shared_ptr<file> file_ptr;
		typedef std::shared_ptr<file_configuration> file_configuration_ptr;
		typedef std::shared_ptr<project> project_ptr;
		typedef std::shared_ptr<tool> tool_ptr;

		struct tool
		{
			struct visitor;

			virtual void visit(const visitor &v) = 0;
		};

		struct tool::visitor
		{
			virtual void visit(compiler_tool &/*t*/) const { }
			virtual void visit(linker_tool &/*t*/) const { }
			virtual void visit(librarian_tool &/*t*/) const { }
		};

		struct compiler_tool : tool
		{
			enum pch_type { pch_none = 0, pch_create = 1, pch_use = 2, };

			virtual std::wstring additional_options() const = 0;
			virtual void additional_options(const std::wstring &value) = 0;

			virtual pch_type use_precompiled_header() const = 0;
			virtual void use_precompiled_header(pch_type value) = 0;
		};

		struct linker_tool : tool
		{
			virtual std::wstring additional_dependencies() const = 0;
			virtual void additional_dependencies(const std::wstring &value) = 0;
		};

		struct librarian_tool : tool
		{
			virtual std::wstring additional_dependencies() const = 0;
			virtual void additional_dependencies(const std::wstring &value) = 0;
		};

		struct configuration
		{
			virtual void enum_tools(const std::function<void (const tool_ptr &t)> &cb) const = 0;
		};

		struct file_configuration
		{
			virtual tool_ptr get_tool() const = 0;
		};

		struct file
		{
			virtual std::wstring unexpanded_relative_path() const = 0;
			virtual void enum_configurations(const std::function<void (const file_configuration_ptr &c)> &cb) const = 0;
			virtual void remove() = 0;
		};

		struct project
		{
			virtual configuration_ptr get_active_configuration() const = 0;
			virtual void enum_configurations(const std::function<void (const configuration_ptr &c)> &cb) const = 0;
			virtual void enum_files(const std::function<void (const file_ptr &c)> &cb) const = 0;
			virtual file_ptr add_file(const std::wstring &path) = 0;
		};

		std::shared_ptr<project> create(IDispatch *dte_project);
	}
}
