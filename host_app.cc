#include "host_app.h"
#include "launcher_button.h"

using namespace System::IO;
using namespace System::Diagnostics;
using namespace System::Runtime::CompilerServices;

using namespace bi_loader;

host_app::host_app() {
    /* The first thing we do: clean temporary files from the last session */
    
    auto di = gcnew DirectoryInfo(temp_path + "\\revit_addin");
    if (Directory::Exists(temp_path + "\\revit_addin"))
	try {
        for each(DirectoryInfo^ dir in di->EnumerateDirectories()) {
            dir->Delete(true);
        }
	}
	catch (exception ex) {
        UI::TaskDialog::Show("Cannot delete all files in temporary directories!", ex->Message + "\n=============\n" + ex->StackTrace);
	}

    assembly_location = Assembly::GetExecutingAssembly()->Location;
};

/* virtual */
Result host_app::OnStartup(UIControlledApplication^ ui_app) {
    DllMain(ui_app);
    return (ui::Result::Succeeded);
}

/* Точка входа в плагин, запускается при старте Revit */
Result host_app::DllMain(UIControlledApplication^ hinst_dll) {

    uic_application = hinst_dll;
    uic_application->ControlledApplication->ApplicationInitialized += gcnew EventHandler<ApplicationInitializedEventArgs^>(this, &host_app::delegate_on_application_initialized);

    api_wrapper_ = gcnew wrapper_into_context(uic_application);
    external_export_event_ = ExternalEvent::Create(api_wrapper_);

    /* Get the path to the actual (original source) DLL */
    const auto vendor_path = gcnew String(vendor_name);
    const auto vendor_directory = gcnew String(app_directory);
    string original_addin_path = Environment::ExpandEnvironmentVariables("%AppData%") + vendor_directory + "\\" + vendor_path + "26\\addin_2026\\revit_addin26.dll";

    string assembly_directory = Path::GetDirectoryName(original_addin_path);
    string actual_dll_path = Path::Combine(assembly_directory, main_addin_dll_name);

    /* Create a unique temp directory for this session */
    string temp_directory = temp_path + "\\revit_addin\\app\\" + Guid::NewGuid().ToString();
    Directory::CreateDirectory(temp_directory);

    /* Copy all files to temp directory */
    copy_directory(assembly_directory, temp_directory);

    create_ribbon_buttons();

    return ui::Result::Succeeded;
}

void host_app::delegate_on_application_initialized(object sender, Autodesk::Revit::DB::Events::ApplicationInitializedEventArgs^ e) {
    auto app = reinterpret_cast<Autodesk::Revit::ApplicationServices::Application^>(sender);
    ext_ui_application_ = gcnew Autodesk::Revit::UI::UIApplication(app);
}

void host_app::create_ribbon_buttons() {    
/*  ComponentManager::UIElementActivated += gcnew EventHandler<UIElementActivatedEventArgs^>(this, &host_app::delegate_component_manager_ui_element_activated);  */

    RibbonControl^ ribbon = ComponentManager::Ribbon;

    const auto tab_name = gcnew String(wtab_name);
    RibbonTab^ bim_tab = ribbon->FindTab(tab_name);
    if (bim_tab == nullptr)
        uic_application->CreateRibbonTab(tab_name);

    string panel_update = "Обновление";

    auto btn_update_dll = gcnew PushButtonData("ID_UPDATE26_BUTTON", "Доставить\nобновления", assembly_location, "bi_loader.btn_click");

    btn_update_dll->AvailabilityClassName = "bi_loader.btn_availability";    

    UI::RibbonPanel^ ribbon_panel = nullptr;
    try {
        ribbon_panel = uic_application->CreateRibbonPanel(tab_name, panel_update);
    }
    catch (Autodesk::Revit::Exceptions::ArgumentException^ e) {
        (void)e; // Explicitly mark 'e' as unused
        /* The panel with same name already exists! */
    }

    btn_update_dll->LargeImage = gcnew BitmapImage(gcnew Uri(host_app::biloader_dll_directory + "\\resources\\update_service_32px.png"));
    btn_update_dll->Image = gcnew BitmapImage(gcnew Uri(host_app::biloader_dll_directory + "\\resources\\update_service_16px.png"));

    auto push_unload_button = dynamic_cast<PushButton^>(ribbon_panel->AddItem(btn_update_dll));    
}

/* virtual */
Result host_app::OnShutdown(UIControlledApplication^ ui_app) {
    ui_app = nullptr;
    return Result::Succeeded;
}