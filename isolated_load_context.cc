#include "isolated_load_context.h"

using namespace bi_loader;

isolated_load_context::isolated_load_context(string context_name, string plugin_path) : AssemblyLoadContext(context_name, true) /* isCollectible = true */
{
	resolver = gcnew AssemblyDependencyResolver(plugin_path);
    this->Unloading += gcnew System::Action<System::Runtime::Loader::AssemblyLoadContext^>(this, &bi_loader::isolated_load_context::on_unloading);
}

Assembly^ isolated_load_context::Load(AssemblyName^ assembly_name) {

    string assembly_path = resolver->ResolveAssemblyToPath(assembly_name);
    if (assembly_path != nullptr) {
        if (assembly_path->Contains("RevitAPI")) {
            return nullptr;
        }
        if (assembly_path->Contains("RevitAPIUI"))  {
            return nullptr;
        }
/*      Если зависимость плагина найдена, загружаем её в контекст плагина:
        auto stream = gcnew FileStream(assembly_path, FileMode::Open, FileAccess::Read);        
        return LoadFromStream(stream);
*/
        return LoadFromAssemblyPath(assembly_path);
    }

    /* Иначе возвращаем nullptr – пусть runtime попытается загрузить в контекст по умолчанию (fall back to default ALC) */
    return nullptr;
}

IntPtr isolated_load_context::LoadUnmanagedDll(string unmanaged_dll_name) {
    string library_path = resolver->ResolveUnmanagedDllToPath(unmanaged_dll_name);
    if (library_path != nullptr) {
        return LoadUnmanagedDllFromPath(library_path);
    }

    return IntPtr::Zero;
}

void bi_loader::isolated_load_context::on_unloading(System::Runtime::Loader::AssemblyLoadContext^ ctx) {
    ctx->Unload();    
}
