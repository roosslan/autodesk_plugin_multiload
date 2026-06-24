#pragma once

#include "stdafx.h"

namespace bi_loader {
    ref class isolated_load_context : public AssemblyLoadContext {
    public:
        isolated_load_context(string context_name, string plugin_path);        
        IntPtr LoadUnmanagedDll(string unmanaged_dll_name) override;
        AssemblyDependencyResolver^ resolver;
        void on_unloading(System::Runtime::Loader::AssemblyLoadContext^ ctx);
        Assembly^ Load(AssemblyName^ assembly_name) override;
    };
}